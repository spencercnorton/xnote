package main

import (
	"bytes"
	"strings"
	"testing"
)

// The recovery-code unlock CLI path (cmdUnlock --recovery) chains
// RecoveryCodeToBytes -> UnwrapMasterKeyFromRecovery on the code that init
// printed. Verify that exact chain recovers the master key end to end (the
// feature was previously unreachable, so this guards against it regressing back
// to dead code).
func TestRecoveryCodeUnlockPath(t *testing.T) {
	masterKey := mustRandBytes(t, 32)
	recoveryBytes := mustRandBytes(t, 32)

	kf, err := BuildKeyfile(masterKey, []byte("pw"), recoveryBytes)
	if err != nil {
		t.Fatalf("BuildKeyfile: %v", err)
	}

	code := RecoveryBytesToCode(recoveryBytes) // what init prints to the user
	decoded, err := RecoveryCodeToBytes(code)  // what cmdUnlock --recovery parses
	if err != nil {
		t.Fatalf("RecoveryCodeToBytes: %v", err)
	}
	got, err := UnwrapMasterKeyFromRecovery(kf, decoded)
	if err != nil {
		t.Fatalf("UnwrapMasterKeyFromRecovery: %v", err)
	}
	if !bytes.Equal(got, masterKey) {
		t.Fatalf("recovered master key mismatch via the recovery-code path")
	}
}

// SnapshotID must zero-pad the generation so ListSnapshots' lexical sort matches
// numeric order (unpadded, "-g10" sorted before "-g9").
func TestSnapshotIDZeroPaddedOrder(t *testing.T) {
	if !strings.HasSuffix(SnapshotID(9), "-g00000000000000000009") {
		t.Fatalf("generation not zero-padded: %s", SnapshotID(9))
	}
	gen9 := strings.SplitN(SnapshotID(9), "-g", 2)[1]
	gen10 := strings.SplitN(SnapshotID(10), "-g", 2)[1]
	if !(gen9 < gen10) {
		t.Fatalf("generation 9 (%s) should sort before 10 (%s)", gen9, gen10)
	}
}
