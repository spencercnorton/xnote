// archive.go — deterministic tar of the xnote config directory.
//
// Produces a canonical tar archive:
//   - stable file order (sorted by path)
//   - fixed mode 0600 for files, 0700 for dirs (not the actual filesystem mode)
//   - uid/gid = 0, uname/gname = ""
//   - mtime = zero (Unix epoch) for determinism
//   - only allowlisted paths: content-*, info-*, default-style, *.conf
//   - excludes: server socket, symlinks, devices, paths containing ".."
//
// The determinism property is tested in archive_test.go.

package main

import (
	"archive/tar"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"
)

var zeroTime = time.Unix(0, 0)

const maxArchiveFileSize = 100 * 1024 * 1024

// isAllowlisted returns true if name matches the backup allowlist.
// Only content-*, info-*, default-style, and *.conf are backed up.
func isAllowlisted(name string) bool {
	// No directory traversal — enforce at caller too
	if strings.Contains(name, "/") || strings.Contains(name, "..") {
		return false
	}
	if strings.HasPrefix(name, "content-") {
		return true
	}
	if strings.HasPrefix(name, "info-") {
		return true
	}
	if name == "default-style" {
		return true
	}
	if strings.HasSuffix(name, ".conf") {
		return true
	}
	return false
}

func validateBackupSource(configDir string) error {
	entries, err := os.ReadDir(configDir)
	if err != nil {
		return fmt.Errorf("read XNote config source %s: %w", configDir, err)
	}
	for _, entry := range entries {
		if !strings.HasPrefix(entry.Name(), "content-") || len(entry.Name()) == len("content-") {
			continue
		}
		if entry.Type()&os.ModeSymlink != 0 || !entry.Type().IsRegular() {
			continue
		}
		info, err := os.Lstat(filepath.Join(configDir, entry.Name()))
		if err != nil {
			return fmt.Errorf("stat note source %s: %w", entry.Name(), err)
		}
		if info.Mode().IsRegular() {
			if info.Size() > maxArchiveFileSize {
				return fmt.Errorf("note source %s is too large to restore safely (%d bytes)", entry.Name(), info.Size())
			}
			return nil
		}
	}
	return fmt.Errorf("refusing backup: no regular content-* note files in %s", configDir)
}

// WriteTar writes a deterministic tar of configDir to w.
// Returns the number of files written.
func WriteTar(w io.Writer, configDir string) (int, error) {
	files, _, err := WriteTarWithNoteCount(w, configDir)
	return files, err
}

// WriteTarWithNoteCount also reports how many real content-* note files made it
// into the archive. Backup uses this as the final source-presence guard so an
// info/default-style/cloud-config-only directory cannot become remote "latest".
func WriteTarWithNoteCount(w io.Writer, configDir string) (files, noteFiles int, err error) {
	entries, err := os.ReadDir(configDir)
	if err != nil {
		return 0, 0, fmt.Errorf("readdir %s: %w", configDir, err)
	}

	// Collect and sort allowlisted regular files
	var names []string
	for _, e := range entries {
		if e.IsDir() || e.Type()&os.ModeSymlink != 0 || e.Type()&os.ModeDevice != 0 || e.Type()&os.ModeSocket != 0 {
			continue
		}
		if !e.Type().IsRegular() {
			continue
		}
		if !isAllowlisted(e.Name()) {
			continue
		}
		names = append(names, e.Name())
	}
	sort.Strings(names)

	tw := tar.NewWriter(w)
	noteFiles = 0
	for _, name := range names {
		fpath := filepath.Join(configDir, name)
		fi, err := os.Lstat(fpath)
		if err != nil {
			return 0, 0, fmt.Errorf("stat %s: %w", name, err)
		}
		// Double-check: reject symlinks even if readdir missed them
		if fi.Mode()&os.ModeSymlink != 0 {
			continue
		}
		if !fi.Mode().IsRegular() {
			continue
		}
		if fi.Size() > maxArchiveFileSize {
			return 0, 0, fmt.Errorf("refusing oversized backup file %s (%d bytes)", name, fi.Size())
		}

		hdr := &tar.Header{
			Name:     name,
			Mode:     0600,
			Uid:      0,
			Gid:      0,
			Size:     fi.Size(),
			ModTime:  zeroTime,
			Typeflag: tar.TypeReg,
			Format:   tar.FormatGNU,
		}
		if err := tw.WriteHeader(hdr); err != nil {
			return 0, 0, fmt.Errorf("tar header %s: %w", name, err)
		}
		f, err := os.Open(fpath)
		if err != nil {
			return 0, 0, fmt.Errorf("open %s: %w", name, err)
		}
		_, copyErr := io.Copy(tw, f)
		f.Close()
		if copyErr != nil {
			return 0, 0, fmt.Errorf("tar body %s: %w", name, copyErr)
		}
		if strings.HasPrefix(name, "content-") && len(name) > len("content-") {
			noteFiles++
		}
	}
	if err := tw.Close(); err != nil {
		return 0, 0, fmt.Errorf("tar close: %w", err)
	}
	return len(names), noteFiles, nil
}

// ExtractTar extracts a tar from r into destDir.
// Security: rejects paths with "..", symlinks, sockets, devices, non-regular files,
// and files not matching the allowlist.
func ExtractTar(r io.Reader, destDir string) error {
	tr := tar.NewReader(r)
	for {
		hdr, err := tr.Next()
		if err == io.EOF {
			break
		}
		if err != nil {
			return fmt.Errorf("tar next: %w", err)
		}

		name := hdr.Name
		// Reject path traversal
		if strings.Contains(name, "/") || strings.Contains(name, "\\") || strings.Contains(name, "..") {
			return fmt.Errorf("security: rejected path %q (directory traversal)", name)
		}
		// Reject symlinks, devices, sockets, directories
		switch hdr.Typeflag {
		case tar.TypeReg, tar.TypeRegA:
			// ok
		case tar.TypeSymlink, tar.TypeLink:
			return fmt.Errorf("security: rejected symlink/hardlink %q in archive", name)
		default:
			return fmt.Errorf("security: rejected non-regular entry %q (typeflag %d)", name, hdr.Typeflag)
		}
		// Reject non-allowlisted files
		if !isAllowlisted(name) {
			return fmt.Errorf("security: rejected non-allowlisted file %q in archive", name)
		}
		// Size sanity (100 MiB per file)
		if hdr.Size < 0 || hdr.Size > maxArchiveFileSize {
			return fmt.Errorf("unreasonable file size %d for %q", hdr.Size, name)
		}

		dest := filepath.Join(destDir, name)
		f, err := os.OpenFile(dest, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0600)
		if err != nil {
			return fmt.Errorf("create %s: %w", name, err)
		}
		_, copyErr := io.Copy(f, tr)
		f.Close()
		if copyErr != nil {
			return fmt.Errorf("write %s: %w", name, copyErr)
		}
	}
	return nil
}
