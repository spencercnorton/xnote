package main

import (
	"os"
	"testing"
)

// withStdinFile points os.Stdin at a temp file holding content, and restores it.
// A file (not a pipe) keeps the test free of writer-side deadlocks; either way
// term.IsTerminal is false, which is the path under test.
func withStdinFile(t *testing.T, content string) {
	t.Helper()
	f, err := os.CreateTemp(t.TempDir(), "stdin")
	if err != nil {
		t.Fatalf("temp file: %v", err)
	}
	if _, err := f.WriteString(content); err != nil {
		t.Fatalf("write: %v", err)
	}
	if _, err := f.Seek(0, 0); err != nil {
		t.Fatalf("seek: %v", err)
	}
	orig := os.Stdin
	os.Stdin = f
	stdinReader = nil
	t.Cleanup(func() {
		os.Stdin = orig
		stdinReader = nil
		f.Close()
	})
}

// The regression: `init` prompts twice, and a per-call bufio reader made the
// second read return EOF because the first had already buffered all of stdin.
// That is why `xnote-cloud-backup init` could not be driven from a pipe.
func TestReadLineNoEchoReadsSuccessiveLines(t *testing.T) {
	withStdinFile(t, "first-secret\nsecond-secret\n")

	got1, err := readLineNoEcho()
	if err != nil {
		t.Fatalf("first read: %v", err)
	}
	if string(got1) != "first-secret" {
		t.Fatalf("first read = %q, want %q", got1, "first-secret")
	}

	got2, err := readLineNoEcho()
	if err != nil {
		t.Fatalf("second read (the regression): %v", err)
	}
	if string(got2) != "second-secret" {
		t.Fatalf("second read = %q, want %q", got2, "second-secret")
	}

	// The first result must survive the second read; bufio.Scanner.Bytes()
	// would have been invalidated by now.
	if string(got1) != "first-secret" {
		t.Fatalf("first result clobbered by second read: %q", got1)
	}
}

// readPassphraseTwice is what init actually calls.
func TestReadPassphraseTwiceFromPipe(t *testing.T) {
	withStdinFile(t, "matching-pass\nmatching-pass\n")
	pw, err := readPassphraseTwice("p1: ", "p2: ")
	if err != nil {
		t.Fatalf("readPassphraseTwice: %v", err)
	}
	if string(pw) != "matching-pass" {
		t.Fatalf("got %q, want %q", pw, "matching-pass")
	}
}

func TestReadPassphraseTwiceRejectsMismatch(t *testing.T) {
	withStdinFile(t, "one\ntwo\n")
	if _, err := readPassphraseTwice("p1: ", "p2: "); err == nil {
		t.Fatal("mismatched passphrases were accepted")
	}
}

// A final line with no trailing newline is real input, not EOF.
func TestReadLineNoEchoAcceptsUnterminatedFinalLine(t *testing.T) {
	withStdinFile(t, "no-trailing-newline")
	got, err := readLineNoEcho()
	if err != nil {
		t.Fatalf("read: %v", err)
	}
	if string(got) != "no-trailing-newline" {
		t.Fatalf("got %q", got)
	}
}

// Genuinely empty stdin must still be an error, not an empty passphrase.
func TestReadLineNoEchoEmptyStdinIsError(t *testing.T) {
	withStdinFile(t, "")
	if _, err := readLineNoEcho(); err == nil {
		t.Fatal("empty stdin returned no error")
	}
}

// CRLF input must not leave a stray \r inside the passphrase.
func TestReadLineNoEchoStripsCRLF(t *testing.T) {
	withStdinFile(t, "crlf-pass\r\n")
	got, err := readLineNoEcho()
	if err != nil {
		t.Fatalf("read: %v", err)
	}
	if string(got) != "crlf-pass" {
		t.Fatalf("got %q, want %q", got, "crlf-pass")
	}
}
