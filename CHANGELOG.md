# Changelog

This file records the public XNote release line. The complete upstream Xpad
history remains available from the canonical Launchpad repository.

## 3.0.4 — 2026-09-19

- Added privacy-reviewed screenshots and short animations covering notes,
  preferences, color selection, and note resizing.

## 3.0.3 — 2026-09-19

- Prepared the first audited public source release.
- Added public provenance, privacy, security, and contribution documentation.
- Hardened backup configuration and restore validation.
- Require Go 1.26.8 or newer for the backup helper so restore is never built
  with the vulnerable `archive/tar` implementation described by GO-2026-4869.
- Kept the encrypted-backup timer disabled after package installation until
  the user explicitly initializes and enables it.
- Made generated source archives complete and reproducible, including the
  installed user-help document, committed translation template, and explicit
  Go source manifest; retained the checksummed vendored Go modules for offline
  source and Debian builds; and support read-only, out-of-tree distribution
  builds.
- Replaced the undocumented raster icon with a GPL-covered derivative of the
  upstream Xpad vector artwork.

## 3.0.2 — 2026-08-03

- Made encrypted-backup initialization safe to drive through a pipe.

## 3.0.0 — 2026-07-29

- Completed the native Wayland port and removed X11-specific application code.

## 2.0.0 — 2026-07-14

- Renamed the application and package from Xpad to XNote.
- Added automatic migration from the legacy Xpad configuration directory.
