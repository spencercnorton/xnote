package main

import (
	"errors"
	"io"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestCopyDirStrictPropagatesWalkError(t *testing.T) {
	wantErr := errors.New("injected walk failure")
	walk := func(root string, walkFn filepath.WalkFunc) error {
		return walkFn(filepath.Join(root, "unreadable"), nil, wantErr)
	}

	err := copyDirStrictWith("source", "destination", walk, copyFileStrict)
	if !errors.Is(err, wantErr) {
		t.Fatalf("copyDirStrictWith error = %v, want %v", err, wantErr)
	}
}

func TestCopyDirStrictPropagatesCopyError(t *testing.T) {
	src := t.TempDir()
	if err := os.WriteFile(filepath.Join(src, "content-note"), []byte("note"), 0600); err != nil {
		t.Fatal(err)
	}
	wantErr := errors.New("injected copy failure")
	copyRegular := func(string, string) error { return wantErr }

	err := copyDirStrictWith(src, filepath.Join(t.TempDir(), "copy"), filepath.Walk, copyRegular)
	if !errors.Is(err, wantErr) {
		t.Fatalf("copyDirStrictWith error = %v, want %v", err, wantErr)
	}
}

func TestCopyDirStrictRejectsNonRegularEntry(t *testing.T) {
	src := t.TempDir()
	if err := os.Symlink("content-note", filepath.Join(src, "note-link")); err != nil {
		t.Fatal(err)
	}

	err := copyDirStrict(src, filepath.Join(t.TempDir(), "copy"))
	if err == nil || !strings.Contains(err.Error(), "non-regular entry") {
		t.Fatalf("copyDirStrict error = %v, want non-regular-entry rejection", err)
	}
}

type errorReadCloser struct {
	readErr    error
	closeErr   error
	closeCalls int
}

func (r *errorReadCloser) Read([]byte) (int, error) { return 0, r.readErr }
func (r *errorReadCloser) Close() error {
	r.closeCalls++
	return r.closeErr
}

type errorWriteCloser struct {
	writeErr   error
	closeErr   error
	closeCalls int
}

func (w *errorWriteCloser) Write(p []byte) (int, error) {
	if w.writeErr != nil {
		return 0, w.writeErr
	}
	return len(p), nil
}
func (w *errorWriteCloser) Close() error {
	w.closeCalls++
	return w.closeErr
}

func TestCopyContentsStrictReportsReadWriteAndCloseErrors(t *testing.T) {
	readErr := errors.New("read failed")
	readSource := &errorReadCloser{readErr: readErr}
	readDest := &errorWriteCloser{}
	if err := copyContentsStrict(readDest, readSource); !errors.Is(err, readErr) {
		t.Fatalf("read error = %v, want %v", err, readErr)
	}
	if readSource.closeCalls != 1 || readDest.closeCalls != 1 {
		t.Fatalf("read-error close calls: source=%d destination=%d, want exactly 1 each", readSource.closeCalls, readDest.closeCalls)
	}

	writeErr := errors.New("write failed")
	writeSource := &countingReadCloser{Reader: strings.NewReader("note")}
	writeDest := &errorWriteCloser{writeErr: writeErr}
	if err := copyContentsStrict(writeDest, writeSource); !errors.Is(err, writeErr) {
		t.Fatalf("write error = %v, want %v", err, writeErr)
	}
	if writeSource.closeCalls != 1 || writeDest.closeCalls != 1 {
		t.Fatalf("write-error close calls: source=%d destination=%d, want exactly 1 each", writeSource.closeCalls, writeDest.closeCalls)
	}

	sourceCloseErr := errors.New("source close failed")
	destCloseErr := errors.New("destination close failed")
	closeSource := &countingReadCloser{Reader: strings.NewReader("note"), closeErr: sourceCloseErr}
	closeDest := &errorWriteCloser{closeErr: destCloseErr}
	if err := copyContentsStrict(closeDest, closeSource); !errors.Is(err, sourceCloseErr) || !errors.Is(err, destCloseErr) {
		t.Fatalf("close error = %v, want both %v and %v", err, sourceCloseErr, destCloseErr)
	}
	if closeSource.closeCalls != 1 || closeDest.closeCalls != 1 {
		t.Fatalf("close-error close calls: source=%d destination=%d, want exactly 1 each", closeSource.closeCalls, closeDest.closeCalls)
	}
}

type countingReadCloser struct {
	io.Reader
	closeErr   error
	closeCalls int
}

func (r *countingReadCloser) Close() error {
	r.closeCalls++
	return r.closeErr
}

func TestRestoreRejectsUnsafeSnapshotIDBeforePathLookup(t *testing.T) {
	_, _ = setupScratchHome(t)
	if err := os.MkdirAll(stateDir(), 0700); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(keyfilePath(), []byte("{}\n"), 0600); err != nil {
		t.Fatal(err)
	}

	binDir := t.TempDir()
	secretTool := filepath.Join(binDir, "secret-tool")
	keyHex := strings.Repeat("00", 32)
	if err := os.WriteFile(secretTool, []byte("#!/bin/sh\nprintf '%s\\n' '"+keyHex+"'\n"), 0700); err != nil {
		t.Fatal(err)
	}
	t.Setenv("PATH", binDir+string(os.PathListSeparator)+os.Getenv("PATH"))

	err := cmdRestore([]string{"../../head", "--force"})
	if err == nil || !strings.Contains(err.Error(), "invalid snapshot ID") {
		t.Fatalf("cmdRestore error = %v, want unsafe snapshot ID rejection", err)
	}
}

func TestValidateSnapshotIDAcceptsOnlyGeneratedSyntax(t *testing.T) {
	if err := validateSnapshotID("20260919T123456Z-g00000000000000000001"); err != nil {
		t.Fatalf("generated snapshot ID rejected: %v", err)
	}
	for _, id := range []string{
		"../../head",
		"20260919T123456Z-g1",
		"20260919T123456Z-g00000000000000000000",
		"20261319T123456Z-g00000000000000000001",
		"20260919T123456Z-g00000000000000000001/extra",
	} {
		if err := validateSnapshotID(id); err == nil {
			t.Errorf("validateSnapshotID(%q) succeeded, want rejection", id)
		}
	}
}
