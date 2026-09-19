// status.go — durable, non-secret backup health state.

package main

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"time"
)

type RemoteStatus struct {
	LastAttemptUTC        string `json:"last_attempt_utc,omitempty"`
	LastAttemptSnapshotID string `json:"last_attempt_snapshot_id,omitempty"`
	LastSuccessUTC        string `json:"last_success_utc,omitempty"`
	LastSuccessSnapshotID string `json:"last_success_snapshot_id,omitempty"`
	LastError             string `json:"last_error,omitempty"`
	Device                string `json:"device,omitempty"`
}

func remoteStatusPath() string {
	return filepath.Join(stateDir(), "remote-status.json")
}

func loadRemoteStatus() (*RemoteStatus, error) {
	data, err := os.ReadFile(remoteStatusPath())
	if os.IsNotExist(err) {
		return &RemoteStatus{}, nil
	}
	if err != nil {
		return nil, err
	}
	var status RemoteStatus
	if err := json.Unmarshal(data, &status); err != nil {
		return nil, err
	}
	return &status, nil
}

func recordRemoteAttempt(snapshotID, device string, pushErr error) error {
	status, err := loadRemoteStatus()
	if err != nil {
		status = &RemoteStatus{}
	}
	now := time.Now().UTC().Format(time.RFC3339)
	status.LastAttemptUTC = now
	status.LastAttemptSnapshotID = snapshotID
	status.Device = device
	if pushErr == nil {
		status.LastSuccessUTC = now
		status.LastSuccessSnapshotID = snapshotID
		status.LastError = ""
	} else {
		status.LastError = pushErr.Error()
	}
	data, err := json.MarshalIndent(status, "", "  ")
	if err != nil {
		return err
	}
	data = append(data, '\n')
	if err := os.MkdirAll(stateDir(), 0700); err != nil {
		return err
	}
	tmp := remoteStatusPath() + ".tmp"
	if err := os.WriteFile(tmp, data, 0600); err != nil {
		return err
	}
	return os.Rename(tmp, remoteStatusPath())
}

func cmdStatus() error {
	cfg, err := LoadConfig()
	if err != nil {
		return fmt.Errorf("load config: %w", err)
	}
	fmt.Printf("Initialised: %t\n", isInitialised())
	if _, err := os.Stat(unlockRequiredPath()); err == nil {
		fmt.Println("Imported state unlock required: yes")
	}
	ids, err := ListSnapshots()
	if err != nil {
		return err
	}
	fmt.Printf("Local snapshots: %d\n", len(ids))
	if head, err := LoadHead(); err == nil {
		fmt.Printf("Head generation: %d\n", head.Generation)
	}
	if cfg.CloudRemote == "" {
		fmt.Println("Cloud remote: not configured")
		return nil
	}
	host, err := remoteHost(cfg, hostname())
	if err != nil {
		return err
	}
	fmt.Printf("Cloud remote: configured (device %s)\n", host)
	status, err := loadRemoteStatus()
	if err != nil {
		return fmt.Errorf("load remote status: %w", err)
	}
	if status.LastAttemptUTC == "" {
		fmt.Println("Remote attempt: never")
		return nil
	}
	fmt.Printf("Remote last attempt: %s (%s)\n", status.LastAttemptUTC, status.LastAttemptSnapshotID)
	if status.LastSuccessUTC != "" {
		fmt.Printf("Remote last success: %s (%s)\n", status.LastSuccessUTC, status.LastSuccessSnapshotID)
	}
	if status.LastError != "" {
		fmt.Printf("Remote last error: %s\n", status.LastError)
	}
	return nil
}

func hostname() string {
	host, _ := os.Hostname()
	return host
}
