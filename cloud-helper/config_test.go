package main

import (
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestLoadConfigMissingUsesDefaults(t *testing.T) {
	setupScratchHome(t)

	cfg, err := LoadConfig()
	if err != nil {
		t.Fatalf("missing config should use defaults: %v", err)
	}
	if cfg != defaultConfig {
		t.Fatalf("missing config = %#v, want defaults %#v", cfg, defaultConfig)
	}
}

func TestLoadConfigReportsOpenFailure(t *testing.T) {
	cfgDir, _ := setupScratchHome(t)
	configPath := filepath.Join(cfgDir, "cloud-backup.conf")
	if err := os.Symlink("cloud-backup.conf", configPath); err != nil {
		t.Fatal(err)
	}

	if _, err := LoadConfig(); err == nil || !strings.Contains(err.Error(), "open config") {
		t.Fatalf("LoadConfig error = %v, want non-missing open failure", err)
	}
}

func TestLoadConfigReportsScannerError(t *testing.T) {
	cfgDir, _ := setupScratchHome(t)
	configPath := filepath.Join(cfgDir, "cloud-backup.conf")
	oversizedLine := "CLOUD_REMOTE=" + strings.Repeat("x", 70*1024) + "\n"
	if err := os.WriteFile(configPath, []byte(oversizedLine), 0600); err != nil {
		t.Fatal(err)
	}

	if _, err := LoadConfig(); err == nil || !strings.Contains(err.Error(), "read config") {
		t.Fatalf("LoadConfig error = %v, want scanner failure", err)
	}
}
