// config.go — read ~/.config/xnote/cloud-backup.conf and resolve XDG dirs.
//
// Recognised keys in cloud-backup.conf (shell-style KEY=value, # comments):
//   KEEP_SNAPSHOTS  int   how many local snapshots to retain (default 60)
//   CLOUD_REMOTE    str   off-box push target; empty = local-only (default).
//                         A filesystem path (starts with / ~ or .) is copied
//                         append-only with rsync. Mount remote storage first,
//                         then provide its local path (for example,
//                         /mnt/backup/xnote). Anything else is treated as an rclone
//                         remote (e.g. dropbox:xnote-backup) and needs `rclone`
//                         plus a one-time `rclone config` sign-in.
//   CLOUD_DEVICE    str   stable remote device directory (default: hostname).
//                         Set this when recovering on a differently named host.
//   CLOUD_TIMEOUT   int   hard seconds for one cloud push (default 120); guards a
//                         hung soft mount from wedging the backup.

package main

import (
	"bufio"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

// Config holds settings from cloud-backup.conf (and defaults).
type Config struct {
	KeepSnapshots int
	// CloudRemote is the off-box push target: a mounted filesystem path
	// or an rclone remote (name:path). Empty means local-only (no push).
	CloudRemote string
	// CloudDevice is the stable device directory below CloudRemote. Empty uses
	// the current hostname.
	CloudDevice string
	// CloudTimeoutSec caps a single cloud push so a hung soft mount cannot wedge
	// the backup.
	CloudTimeoutSec int
}

var defaultConfig = Config{
	KeepSnapshots:   60,
	CloudRemote:     "",
	CloudDevice:     "",
	CloudTimeoutSec: 120,
}

// LoadConfig reads cloud-backup.conf and returns a Config.
// Missing file is not an error — defaults apply.
func LoadConfig() (Config, error) {
	cfg := defaultConfig
	path := filepath.Join(configDir(), "cloud-backup.conf")
	f, err := os.Open(path)
	if err != nil {
		if os.IsNotExist(err) {
			return cfg, nil
		}
		return cfg, fmt.Errorf("open config %s: %w", path, err)
	}
	defer f.Close()

	sc := bufio.NewScanner(f)
	for sc.Scan() {
		line := strings.TrimSpace(sc.Text())
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		if k, v, ok := strings.Cut(line, "="); ok {
			k = strings.TrimSpace(k)
			v = strings.Trim(strings.TrimSpace(v), `"'`)
			switch k {
			case "KEEP_SNAPSHOTS":
				if n, err := strconv.Atoi(v); err == nil && n > 0 {
					cfg.KeepSnapshots = n
				}
			case "CLOUD_REMOTE":
				cfg.CloudRemote = v
			case "CLOUD_DEVICE":
				cfg.CloudDevice = v
			case "CLOUD_TIMEOUT":
				if n, err := strconv.Atoi(v); err == nil && n > 0 {
					cfg.CloudTimeoutSec = n
				}
			}
		}
	}
	if err := sc.Err(); err != nil {
		return cfg, fmt.Errorf("read config %s: %w", path, err)
	}
	return cfg, nil
}

// configDir returns ~/.config/xnote
func configDir() string {
	if d := os.Getenv("XDG_CONFIG_HOME"); d != "" {
		return filepath.Join(d, "xnote")
	}
	return filepath.Join(os.Getenv("HOME"), ".config", "xnote")
}

// stateDir returns ~/.local/share/xnote-backup
func stateDir() string {
	if d := os.Getenv("XDG_DATA_HOME"); d != "" {
		return filepath.Join(d, "xnote-backup")
	}
	return filepath.Join(os.Getenv("HOME"), ".local", "share", "xnote-backup")
}
