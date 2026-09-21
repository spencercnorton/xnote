# Changelog

This file records the public XNote release line. The complete upstream Xpad
history remains available from the canonical Launchpad repository.

## 3.2.0 — 2026-09-21

- The package recommends `gnome-shell-extension-xnote-placement`, so
  `apt install xnote` brings the placement extension along; the README says
  how to enable it and what it does.
- README rewritten to the NorviTech Suite house style: suite, release and
  install badges, an encrypted-backup callout (Dropbox or any rclone remote,
  the recovery code, the hourly timer), the placement callout, a data-location
  table with what leaves the machine, and the suite footer. Community files
  aligned with the shared templates (DCO sign-off, an out-of-scope list, a
  security scope section).
- The GitHub CI workflow can be run by hand (`workflow_dispatch`).

## 3.1.0 — 2026-09-20

- Debian package built from the audited public tree (`scripts/build-deb.sh`,
  `debian/` in the release) and published to the APT archive on every
  release tag; `sudo apt install xnote` on Ubuntu 26.04.
- Documentation: `docs/user-guide.md` (every setting, shortcut, command and
  the backup helper end to end), `docs/development.md` (source layout, tests,
  packaging, the release model, a section for coding agents), a rewritten
  in-app help (`F1`), and the `xnote-cloud-backup(1)` page now lists `sync`
  and `show-recovery`.
- Community files: contributing guide, code of conduct, support and security
  policies, issue forms, pull-request template, a CI workflow on GitHub and
  the Sponsor button.
- Licence text for `src/prefix.c` moved to `src/COPYING.LESSER` so the
  repository is labelled GPL-3.0 rather than LGPL-3.0; `COPYING` unchanged.
- `xnote-cloud-backup restore` no longer refuses after a normal quit: only a
  live XNote answering on its socket counts as running, and XNote removes
  the socket when it quits.
- "Open a new empty pad" applies at startup only; `xnote --show`, `--hide`
  and `--toggle` no longer open an extra note when it is enabled.
- The application icon is the orange note with the folded corner again, as a
  vector (`images/hicolor/scalable/apps/xnote.svg`), so it is crisp at every
  size and the same on the GitHub page and the desktop.
- The AppStream metadata installs as `tech.norvi.xnote.metainfo.xml`, names
  the screenshots and the help URL, and validates cleanly; libadwaita 1.5 is
  the declared minimum (it always was the real one).

## 3.0.6 — 2026-09-20

- README: leads with the name and what a note is, shows a recording of a
  session (typing, `Ctrl+N`, bold, resize) as the hero, a desktop and a
  hovered-toolbar capture, and one paragraph per feature; the palette and
  Preferences captures stay. Captures from an isolated profile with invented
  notes. The two GIFs and the bare 300 px note are gone.

## 3.0.5 — 2026-09-19

- Rename the public README to `README.md` so GitHub renders its screenshots
  and animations on the repository overview page.

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
