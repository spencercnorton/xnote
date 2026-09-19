// terminal.go — read a line without terminal echo (passphrase input).

package main

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"os"
	"strings"

	"golang.org/x/term"
)

// stdinReader is created once, on first non-terminal read, and reused.
//
// It must NOT be per-call. bufio reads ahead, so a reader built fresh for each
// prompt swallows everything the previous one buffered: `init` asks twice
// ("Enter backup passphrase" / "Confirm passphrase"), the first reader consumed
// all of stdin into its buffer, and the second saw EOF. That made `init`
// impossible to drive from a pipe — it only worked on a real terminal, where
// term.ReadPassword reads the fd directly and no buffering happens. Anything
// scripting a restore (a runbook, a recovery drill, CI) hit "EOF reading
// passphrase" instead.
var stdinReader *bufio.Reader

// readLineNoEcho reads a line from stdin without echoing (for passphrase input).
// Falls back to a plain buffered read if stdin is not a terminal.
func readLineNoEcho() ([]byte, error) {
	fd := int(os.Stdin.Fd())
	if term.IsTerminal(fd) {
		pw, err := term.ReadPassword(fd)
		fmt.Fprintln(os.Stderr) // newline after hidden input
		return pw, err
	}

	// Not a terminal (pipe, test, systemd) — read a plain line.
	if stdinReader == nil {
		stdinReader = bufio.NewReader(os.Stdin)
	}
	line, err := stdinReader.ReadString('\n')
	// ReadString returns what it read even on error, so trim first and treat a
	// final line with no trailing newline as valid input rather than EOF.
	line = strings.TrimRight(line, "\r\n")
	if err != nil {
		if !errors.Is(err, io.EOF) {
			return nil, err
		}
		if line == "" {
			return nil, fmt.Errorf("EOF reading passphrase")
		}
	}
	// ReadString allocates, so unlike bufio.Scanner.Bytes() the result stays
	// valid after the next read — which matters now that the reader is shared.
	return []byte(line), nil
}
