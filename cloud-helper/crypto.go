// crypto.go — pinned cryptographic primitives for xnote-cloud-backup.
//
// Key hierarchy (IMPLEMENT EXACTLY — do not change defaults):
//   master_key   32 random bytes, generated once at init.
//   KEK          Argon2id(passphrase, salt, time=4, mem=1GiB, threads=4, len=32)
//   wrapped_mk   XChaCha20-Poly1305(KEK, nonce||master_key) in keyfile.json
//   DEK          32 random bytes per archive
//   wrap_key     HKDF-SHA256(master_key, salt=nil, info="xnote archive wrap v1"), 32 bytes
//   wrapped_dek  XChaCha20-Poly1305(wrap_key, nonce||DEK) in snapshot header
//
// Stream encryption: 64 KiB plaintext chunks, each sealed with XChaCha20-Poly1305.
// Nonce = base_nonce (24 bytes) XOR big-endian chunk index (last 8 bytes).
// Final chunk's AAD includes "final" marker to detect truncation.
// Header is authenticated as AAD of the first chunk.

package main

import (
	"bytes"
	"crypto/hmac"
	"crypto/rand"
	"crypto/sha256"
	"encoding/binary"
	"fmt"
	"io"

	"golang.org/x/crypto/argon2"
	"golang.org/x/crypto/chacha20poly1305"
	"golang.org/x/crypto/hkdf"
)

const (
	// Argon2id parameters — PINNED, stored in keyfile so they can be upgraded.
	argon2Time    = 4
	argon2Memory  = 1048576 // 1 GiB in KiB
	argon2Threads = 4
	argon2KeyLen  = 32

	// Salt sizes
	argon2SaltLen = 16 // 16 bytes per slot
	xchacha20NonceLen = 24 // XChaCha20-Poly1305 nonce size

	// Chunk size for stream encryption
	chunkSize = 64 * 1024 // 64 KiB plaintext per chunk

	// HKDF info string — fixed, do not change
	hkdfInfoArchiveWrap = "xnote archive wrap v1"

	// Snapshot header magic — identifies file format
	snapMagic = "XNOTE-SNAP-1"
)

// randBytes generates n cryptographically random bytes.
func randBytes(n int) ([]byte, error) {
	b := make([]byte, n)
	if _, err := io.ReadFull(rand.Reader, b); err != nil {
		return nil, fmt.Errorf("rand: %w", err)
	}
	return b, nil
}

// deriveKEK runs Argon2id with the pinned parameters to derive a KEK from a passphrase.
func deriveKEK(passphrase []byte, salt []byte) []byte {
	return argon2.IDKey(passphrase, salt, argon2Time, argon2Memory, argon2Threads, argon2KeyLen)
}

// deriveKEKWithParams runs Argon2id with caller-supplied parameters (from stored keyfile).
// Use this for unwrap operations so stored param upgrades take effect.
func deriveKEKWithParams(passphrase, salt []byte, t, m uint32, threads uint8, keyLen uint32) []byte {
	return argon2.IDKey(passphrase, salt, t, m, threads, keyLen)
}

// wrapKey wraps plaintext (e.g. master_key or DEK) under key using XChaCha20-Poly1305.
// Returns nonce||ciphertext.
func wrapKey(key, plaintext []byte) ([]byte, error) {
	aead, err := chacha20poly1305.NewX(key)
	if err != nil {
		return nil, fmt.Errorf("newX: %w", err)
	}
	nonce, err := randBytes(xchacha20NonceLen)
	if err != nil {
		return nil, err
	}
	ct := aead.Seal(nil, nonce, plaintext, nil)
	return append(nonce, ct...), nil
}

// unwrapKey unwraps nonce||ciphertext under key using XChaCha20-Poly1305.
func unwrapKey(key, blob []byte) ([]byte, error) {
	if len(blob) < xchacha20NonceLen {
		return nil, fmt.Errorf("wrapped key too short")
	}
	aead, err := chacha20poly1305.NewX(key)
	if err != nil {
		return nil, fmt.Errorf("newX: %w", err)
	}
	nonce := blob[:xchacha20NonceLen]
	ct := blob[xchacha20NonceLen:]
	pt, err := aead.Open(nil, nonce, ct, nil)
	if err != nil {
		return nil, fmt.Errorf("AEAD open failed (wrong key or corrupted data): %w", err)
	}
	return pt, nil
}

// deriveArchiveWrapKey derives the DEK wrapping key from master_key via HKDF-SHA256.
func deriveArchiveWrapKey(masterKey []byte) ([]byte, error) {
	h := hkdf.New(sha256.New, masterKey, nil, []byte(hkdfInfoArchiveWrap))
	key := make([]byte, 32)
	if _, err := io.ReadFull(h, key); err != nil {
		return nil, fmt.Errorf("hkdf: %w", err)
	}
	return key, nil
}

// generateDEK generates a random 32-byte DEK and wraps it under the archive wrap key.
// Returns (dek, wrappedDEK, error).
func generateDEK(masterKey []byte) ([]byte, []byte, error) {
	dek, err := randBytes(32)
	if err != nil {
		return nil, nil, err
	}
	archiveWrapKey, err := deriveArchiveWrapKey(masterKey)
	if err != nil {
		return nil, nil, err
	}
	defer zero(archiveWrapKey)
	wrapped, err := wrapKey(archiveWrapKey, dek)
	if err != nil {
		return nil, nil, err
	}
	return dek, wrapped, nil
}

// unwrapDEK unwraps a wrapped DEK blob using master_key.
func unwrapDEK(masterKey, wrappedDEK []byte) ([]byte, error) {
	wk, err := deriveArchiveWrapKey(masterKey)
	if err != nil {
		return nil, err
	}
	defer zero(wk)
	return unwrapKey(wk, wrappedDEK)
}

// chunkNonce computes the nonce for chunk i by XORing the base nonce with
// the big-endian chunk counter in the last 8 bytes.
// base must be exactly xchacha20NonceLen (24) bytes.
func chunkNonce(base []byte, i uint64) []byte {
	n := make([]byte, xchacha20NonceLen)
	copy(n, base)
	// XOR last 8 bytes with big-endian counter
	ctr := make([]byte, 8)
	binary.BigEndian.PutUint64(ctr, i)
	for j := 0; j < 8; j++ {
		n[xchacha20NonceLen-8+j] ^= ctr[j]
	}
	return n
}

// chunkAAD returns the AAD for chunk i. The header bytes are AAD for chunk 0.
// The final chunk always gets the "final" tag appended.
// isFinal marks the last chunk so truncation is detected.
func chunkAAD(headerAAD []byte, i uint64, isFinal bool) []byte {
	// Build: [chunk_index_be8] [header if i==0] ["final" if isFinal]
	aad := make([]byte, 8)
	binary.BigEndian.PutUint64(aad, i)
	if i == 0 && len(headerAAD) > 0 {
		aad = append(aad, headerAAD...)
	}
	if isFinal {
		aad = append(aad, []byte("final")...)
	}
	return aad
}

// EncryptStream encrypts plaintext from r to w using DEK (XChaCha20-Poly1305 chunks).
// headerAAD is authenticated as part of chunk 0's AAD (the snapshot header).
// Returns the (randomly generated) base nonce used — caller stores it in the header.
func EncryptStream(w io.Writer, r io.Reader, dek []byte, headerAAD []byte) ([]byte, error) {
	baseNonce, err := randBytes(xchacha20NonceLen)
	if err != nil {
		return nil, err
	}
	return EncryptStreamWithNonce(w, r, dek, headerAAD, baseNonce)
}

// EncryptStreamWithNonce is like EncryptStream but uses a caller-supplied nonce.
// Use this when the nonce must be committed to a header before encryption.
func EncryptStreamWithNonce(w io.Writer, r io.Reader, dek []byte, headerAAD []byte, baseNonce []byte) ([]byte, error) {
	aead, err := chacha20poly1305.NewX(dek)
	if err != nil {
		return nil, fmt.Errorf("DEK AEAD init: %w", err)
	}
	if len(baseNonce) != xchacha20NonceLen {
		return nil, fmt.Errorf("baseNonce must be %d bytes", xchacha20NonceLen)
	}

	buf := make([]byte, chunkSize)
	var chunkIdx uint64
	wroteFinal := false
	for {
		n, readErr := io.ReadFull(r, buf)
		if n == 0 && readErr == io.EOF {
			break
		}
		if readErr != nil && readErr != io.ErrUnexpectedEOF && readErr != io.EOF {
			return nil, fmt.Errorf("read chunk %d: %w", chunkIdx, readErr)
		}
		if chunkIdx == ^uint64(0) {
			return nil, fmt.Errorf("chunk counter overflow")
		}
		isFinal := readErr == io.ErrUnexpectedEOF || readErr == io.EOF
		plainChunk := buf[:n]
		nonce := chunkNonce(baseNonce, chunkIdx)
		aad := chunkAAD(headerAAD, chunkIdx, isFinal)
		ct := aead.Seal(nil, nonce, plainChunk, aad)

		// Write length-prefixed ciphertext: uint32 big-endian length then bytes
		var lenBuf [4]byte
		binary.BigEndian.PutUint32(lenBuf[:], uint32(len(ct)))
		if _, err := w.Write(lenBuf[:]); err != nil {
			return nil, fmt.Errorf("write chunk len: %w", err)
		}
		if _, err := w.Write(ct); err != nil {
			return nil, fmt.Errorf("write chunk: %w", err)
		}
		chunkIdx++
		if isFinal {
			wroteFinal = true
			break
		}
	}
	// If input was empty or an exact multiple of chunkSize, emit an empty final chunk.
	// For the exact-multiple case: the loop exits via the n==0 && EOF branch without
	// ever setting isFinal=true, so wroteFinal remains false and we must emit here.
	if !wroteFinal {
		nonce := chunkNonce(baseNonce, chunkIdx)
		aad := chunkAAD(headerAAD, chunkIdx, true)
		ct := aead.Seal(nil, nonce, []byte{}, aad)
		var lenBuf [4]byte
		binary.BigEndian.PutUint32(lenBuf[:], uint32(len(ct)))
		if _, err := w.Write(lenBuf[:]); err != nil {
			return nil, err
		}
		if _, err := w.Write(ct); err != nil {
			return nil, err
		}
	}
	return baseNonce, nil
}

// DecryptStream decrypts a stream produced by EncryptStream.
// dek is the archive DEK, baseNonce is from the snapshot header, headerAAD is the same header bytes.
// Writes plaintext to w ONLY after the entire stream has been authenticated as complete.
// On any error, w receives nothing (staging buffer is discarded).
func DecryptStream(w io.Writer, r io.Reader, dek []byte, baseNonce []byte, headerAAD []byte) error {
	aead, err := chacha20poly1305.NewX(dek)
	if err != nil {
		return fmt.Errorf("DEK AEAD init: %w", err)
	}

	// Decrypt all chunks into a staging buffer first.
	// Only copy to w after foundFinal AND post-final EOF verified.
	var staging bytes.Buffer
	var chunkIdx uint64
	var lenBuf [4]byte
	foundFinal := false
	for {
		_, err := io.ReadFull(r, lenBuf[:])
		if err == io.EOF || err == io.ErrUnexpectedEOF {
			break
		}
		if err != nil {
			return fmt.Errorf("read chunk len: %w", err)
		}
		ctLen := binary.BigEndian.Uint32(lenBuf[:])
		if ctLen > uint32(chunkSize)+uint32(aead.Overhead())+64 { // sanity: max chunk + poly tag + small margin
			return fmt.Errorf("chunk %d: ciphertext length %d is unreasonably large", chunkIdx, ctLen)
		}
		ct := make([]byte, ctLen)
		if _, err := io.ReadFull(r, ct); err != nil {
			return fmt.Errorf("read chunk %d body: %w", chunkIdx, err)
		}

		// Try with isFinal=false first, then isFinal=true
		nonce := chunkNonce(baseNonce, chunkIdx)
		aadNoFinal := chunkAAD(headerAAD, chunkIdx, false)
		aadFinal := chunkAAD(headerAAD, chunkIdx, true)

		pt, errNoFinal := aead.Open(nil, nonce, ct, aadNoFinal)
		if errNoFinal != nil {
			// Try as final chunk
			var errFinal error
			pt, errFinal = aead.Open(nil, nonce, ct, aadFinal)
			if errFinal != nil {
				return fmt.Errorf("chunk %d: AEAD authentication failed (tamper or truncation)", chunkIdx)
			}
			foundFinal = true
		}

		staging.Write(pt)

		if foundFinal {
			// Verify no more data after final chunk. io.ReadFull (not a bare
			// Read, which may legally return (0,nil)) reliably reports whether
			// any trailing byte exists.
			extra := make([]byte, 1)
			n, _ := io.ReadFull(r, extra)
			if n > 0 {
				return fmt.Errorf("data found after final chunk — stream integrity violation")
			}
			break
		}
		chunkIdx++
	}
	if !foundFinal {
		return fmt.Errorf("stream truncated: final chunk marker never seen")
	}
	// Stream fully authenticated — now write to caller
	if _, err := io.Copy(w, &staging); err != nil {
		return fmt.Errorf("write plaintext: %w", err)
	}
	return nil
}

// hashBytes returns SHA-256 of input (for parent_cipher_hash).
func hashBytes(b []byte) [32]byte {
	return sha256.Sum256(b)
}

// hmacEqual is a constant-time comparison for []byte.
func hmacEqual(a, b []byte) bool {
	return hmac.Equal(a, b)
}

// zero overwrites a byte slice with zeros to clear sensitive key material.
func zero(b []byte) {
	for i := range b {
		b[i] = 0
	}
}
