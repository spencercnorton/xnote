// lock.go — process serialization for state-changing backup operations.

package main

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"syscall"
)

type stateLock struct {
	file *os.File
}

func acquireStateLock() (*stateLock, error) {
	if err := os.MkdirAll(stateDir(), 0700); err != nil {
		return nil, fmt.Errorf("create state directory for lock: %w", err)
	}

	path := filepath.Join(stateDir(), "backup.lock")
	f, err := os.OpenFile(path, os.O_CREATE|os.O_RDWR, 0600)
	if err != nil {
		return nil, fmt.Errorf("open backup lock: %w", err)
	}
	if err := syscall.Flock(int(f.Fd()), syscall.LOCK_EX|syscall.LOCK_NB); err != nil {
		f.Close()
		if errors.Is(err, syscall.EWOULDBLOCK) || errors.Is(err, syscall.EAGAIN) {
			return nil, fmt.Errorf("another XNote backup/recovery operation is already running")
		}
		return nil, fmt.Errorf("lock backup state: %w", err)
	}
	return &stateLock{file: f}, nil
}

func (l *stateLock) Close() error {
	if l == nil || l.file == nil {
		return nil
	}
	err := syscall.Flock(int(l.file.Fd()), syscall.LOCK_UN)
	closeErr := l.file.Close()
	l.file = nil
	if err != nil {
		return err
	}
	return closeErr
}

func withStateLock(fn func() error) error {
	lock, err := acquireStateLock()
	if err != nil {
		return err
	}
	defer lock.Close()
	return fn()
}
