<p align="center">
  <img src="images/hicolor/scalable/apps/xnote.svg" alt="XNote icon" width="96">
</p>

<h1 align="center">XNote</h1>

<p align="center">
  <strong>Sticky notes that stay on your desktop, and stay yours.</strong><br>
  A native GTK4/libadwaita notes app for GNOME on Wayland, descended from Xpad.
</p>

<p align="center">
  <a href="https://github.com/spencercnorton/xnote/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/spencercnorton/xnote/actions/workflows/ci.yml/badge.svg"></a>
  <a href="https://apt.globalentry.systems"><img alt="APT repository" src="https://img.shields.io/badge/apt-Ubuntu%2026.04-e95420.svg?logo=ubuntu&logoColor=white"></a>
  <a href="COPYING"><img alt="GPL-3.0-or-later" src="https://img.shields.io/badge/licence-GPL--3.0--or--later-blue.svg"></a>
  <a href="https://buy.stripe.com/8x26oH2U44f65TRe574wM04"><img alt="Donate" src="https://img.shields.io/badge/donate-Stripe-635bff.svg?logo=stripe&logoColor=white"></a>
  <a href="https://github.com/spencercnorton/xnote-placement"><img alt="Placement extension" src="https://img.shields.io/badge/GNOME_Shell-placement_extension-4a86cf.svg"></a>
</p>

<p align="center">
  <img alt="Five sticky notes on a desktop: a line is typed into one, a new note is created with Ctrl+N, its title made bold, and it is resized" src="screenshots/xnote-session.png" width="900">
</p>

A note is a small window with your words in it, in the colour you gave it,
where you left it. XNote keeps each one as an independent window, saves every
change as you type, and never sends a byte anywhere unless you ask it to.

## What it looks like

**Notes, not a notes app.** There is no list to open first: each note is its
own window on the desktop, sized and coloured on its own, with the first line
as its title. Hover a note and a small toolbar appears; move the pointer away
and it is gone again.

<p align="center">
  <img alt="Five sticky notes of different colours and sizes on a desktop" src="screenshots/xnote-desktop.png" width="900">
</p>

**Everything is a keystroke.** `Ctrl+N` for a new note, `Ctrl+B`/`I`/`U` for
formatting, `Ctrl+F` to search inside a note; hold `Ctrl` and drag to move a
note, `Ctrl` and right-drag to resize it. Right-click for the rest.

<p align="center">
  <img alt="The floating toolbar on a hovered note" src="screenshots/xnote-toolbar.png" width="900">
</p>

**Colours and fonts, per note or for all of them.** The dot on a hovered
note's toolbar opens a row of swatches — one click and the note changes
colour. The full palette, your own colours and the font live in Preferences,
once for every note or overridden on a single one. New notes can take a
random colour so a cluster reads at a glance.


<p align="center">
  <img alt="Clicking the colour dot on a note's toolbar opens a row of swatches; picking one recolours the note" src="screenshots/xnote-color-popover.png" width="470">
  <img alt="The XNote colour palette" src="screenshots/xnote-color-picker.png" width="420">
</p>
<p align="center">
  <img alt="Preferences — Layout: font, colours and the default note size" src="screenshots/xnote-preferences-layout.png" width="420">
</p>

**Saved as you type, backed up if you want.** Notes live as plain files under
`~/.config/xnote`; the optional `xnote-cloud-backup` helper keeps encrypted,
authenticated snapshots locally or on an `rclone` remote, and a recovery code
brings them back on a new machine.

**At home on GNOME.** Native Wayland, libadwaita styling that follows your
theme, an optional StatusNotifierItem tray menu, and the companion
[XNote Placement](https://github.com/spencercnorton/xnote-placement) extension
that puts every note back on the workspace and monitor you left it on.

<p align="center">
  <img alt="Preferences — View: toolbar, autohide, scrollbar and window decorations" src="screenshots/xnote-preferences-view.png" width="420">
</p>

## Install

### Ubuntu 26.04 — from the APT repository

Add the repository once and XNote updates with everything else:

```bash
curl -fsSL https://apt.globalentry.systems/setup.sh | sudo sh
sudo apt install xnote
```

`setup.sh` installs the signing key and the suite for your release; read it
first if you prefer to do those two steps by hand — it is short. Only `amd64`
packages for 26.04 are published today. The package conflicts with, replaces
and provides `xpad`, and on first launch XNote moves the notes it finds in
`~/.config/xpad` to `~/.config/xnote`.

### Other distributions — build from source

XNote is tested on Ubuntu 26.04 with GNOME Shell 50. Building it requires:

- GTK 4.10 or newer
- libadwaita 1.5 or newer
- GtkSourceView 5
- GLib/GIO 2.66 or newer
- Pango and GdkPixbuf development files
- Autoconf, Automake, Libtool, Gettext, and a C compiler
- Go 1.26.8 or newer only when building `xnote-cloud-backup` (older releases
  contain a security flaw in the archive parser used during restore)

Other recent Linux distributions should work when they provide compatible
libraries. XNote does not directly link against X11 libraries.

On Ubuntu, install the development dependencies:

```sh
sudo apt install build-essential autoconf automake libtool intltool gettext \
  autopoint pkg-config autoconf-archive libgtk-4-dev libadwaita-1-dev \
  libgtksourceview-5-dev libglib2.0-dev libpango1.0-dev \
  libgdk-pixbuf-2.0-dev
```

To include the encrypted-backup helper, install a Go 1.26.8-or-newer
toolchain. The checked-in installer is the same checksummed Go 1.26.8 archive
used for release builds (it installs under `/opt` and needs root there):

```sh
sudo ./ci/install-go-toolchain.sh
export PATH="/opt/xnote-go-1.26.8/bin:$PATH"
go version
```

Without a suitable `go` executable, the desktop application still builds but
`xnote-cloud-backup` is omitted. An older Go fails closed; the build never
downloads a replacement toolchain or module dependencies.

Then build and install:

```sh
git clone https://github.com/spencercnorton/xnote.git
cd xnote
NOCONFIGURE=1 ./autogen.sh
./configure --prefix=/usr
make -j"$(nproc)"
sudo make install
```

Run `xnote` to start the application. Existing Xpad notes under
`~/.config/xpad` are migrated to `~/.config/xnote` on first launch when the
XNote directory does not already exist.

`scripts/build-deb.sh` builds the same `.deb` the repository publishes, and
the source tarball beside it.

## Documentation

The [user guide](docs/user-guide.md) covers the things the screenshots do not:

- [First run](docs/user-guide.md#first-run) — the first note, autostart and
  where Xpad notes go
- [Notes](docs/user-guide.md#notes) — the toolbar, the context menu, colours,
  fonts, close versus delete
- [Preferences](docs/user-guide.md#preferences) — every page and what each
  setting changes
- [Tray](docs/user-guide.md#tray) — the StatusNotifierItem menu and what
  happens without one
- [Keyboard shortcuts](docs/user-guide.md#keyboard-shortcuts)
- [Command line](docs/user-guide.md#command-line) — driving the running
  instance with `--new`, `--show`, `--hide`, `--toggle`, `--quit`
- [Backup and recovery](docs/user-guide.md#backup-and-recovery) —
  `xnote-cloud-backup` from `init` to `restore` on a new machine
- [Troubleshooting](docs/user-guide.md#troubleshooting)

[docs/development.md](docs/development.md) maps the source tree, the tests
and the release model. The man pages `xnote(1)` and `xnote-cloud-backup(1)`
install with the package (their roff source is in `doc/`).

## Where your data lives

| Path | Purpose |
|---|---|
| `~/.config/xnote/` | Live notes, in plain text: one `content-*` (the words, with formatting tags) and one `info-*` (size, colour, font) per note; `default-style` holds Preferences; `cloud-backup.conf` configures the backup helper; `server` is the socket a running instance listens on. Honours `$XDG_CONFIG_HOME`. |
| `~/.config/autostart/xnote.desktop` | A symlink, created and removed by Preferences → Startup → "Start XNote automatically after login" |
| `~/.local/share/xnote-backup/` | The backup helper's state: `keyfile.json` (the master key, wrapped by your passphrase and again by the recovery code), `head.json`, `snapshots/`, `remote-status.json`, `backup.lock`. Honours `$XDG_DATA_HOME`. |
| `<CLOUD_REMOTE>/<device>/kits/<snapshot-id>/` | Off-box recovery kits, one per backup, append-only; the helper never deletes remote history |
| `xnote-cloud-backup.timer`, `xnote-cloud-backup.service` | systemd user units installed by the package, disabled until you enable them; the timer runs `xnote-cloud-backup sync` hourly |

## Note data and privacy

Live notes are plaintext files under `~/.config/xnote`. Protect that directory
with the same care as the notes themselves. XNote does not send note contents
over the network.

The optional `xnote-cloud-backup` helper creates authenticated encrypted
snapshots under `~/.local/share/xnote-backup`. A configured storage provider
can still observe metadata such as device name, timestamps, file sizes, item
count, and backup cadence. See `man xnote-cloud-backup` before enabling remote
backups, and keep the generated recovery code somewhere separate from the
computer.

Backup is opt-in. Initialise it and verify a manual backup before enabling the
hourly user timer (the timer is installed by the `.deb`; a source build has
none — see the [guide](docs/user-guide.md#backup-and-recovery)):

```sh
xnote-cloud-backup init
xnote-cloud-backup backup
systemctl --user enable --now xnote-cloud-backup.timer
```

The pictures above were captured from a fresh, isolated profile with invented
notes; no personal note or desktop data appears in them.

## Contributing and support

- Bugs and feature requests: [open an issue](https://github.com/spencercnorton/xnote/issues/new/choose);
  questions go to [Discussions](https://github.com/spencercnorton/xnote/discussions).
  [SUPPORT.md](SUPPORT.md) says what to include.
- Security reports: [private vulnerability reporting](https://github.com/spencercnorton/xnote/security/advisories/new) — see [SECURITY.md](SECURITY.md).
- Pull requests are welcome; read [CONTRIBUTING.md](CONTRIBUTING.md) first —
  this repository is a release mirror, and accepted changes ship in the next
  tagged release. Releases are listed in [CHANGELOG.md](CHANGELOG.md).
- If XNote saves you time, you can [support its development](https://buy.stripe.com/8x26oH2U44f65TRe574wM04).

## Development

```sh
make check                                      # GLib unit tests, C — what CI runs
make check-go                                   # Go tests for the backup helper (needs Go at configure time)
cd cloud-helper && GOFLAGS=-mod=vendor GOPROXY=off GOTOOLCHAIN=local go test ./...   # the same, without automake
dbus-run-session -- tests/wayland-smoke.sh /usr/bin/xnote   # end to end, under a headless Weston
python3 tests/public-check.py                   # what every public release must satisfy
```

The Wayland smoke test requires a headless Weston compositor; CI runs it
against the installed build tree.

```
src/              # the GTK4 application, C — files and symbols keep their upstream xpad- names
cloud-helper/     # xnote-cloud-backup, Go, dependencies vendored
data/             # xnote.desktop and AppStream metadata, generated from .in templates
doc/              # man pages and the in-app help text
docs/             # user guide and development notes
images/           # the two SVG icons (app and symbolic tray)
po/               # gettext catalogue
tests/            # GLib C tests, the Wayland smoke test, the public-release check
ci/               # the checksummed Go toolchain installer
debian/           # Debian packaging (the changelog is generated at build time)
scripts/          # build-deb.sh: the .deb and source tarball the APT repository publishes
screenshots/      # the captures above
```

## Provenance and licence

XNote is a descendant of [Xpad](https://launchpad.net/xpad), forked from its
development tree after the 5.8 release. The public history keeps every
upstream commit up to the
[baseline commit](https://git.launchpad.net/xpad/commit/?id=637c7b51f1b09a28553a926f594f626d363c526a),
which is why the contributor list shows Xpad's authors; everything after it is
XNote — see the
[comparison](https://github.com/spencercnorton/xnote/compare/637c7b51f1b09a28553a926f594f626d363c526a...main).

XNote is free software under the GNU General Public License, version 3 or, at
your option, any later version: [COPYING](COPYING). The BinReloc code in
`src/prefix.c` and `src/prefix.h` is under the GNU Lesser General Public
License, version 3 or later: [src/COPYING.LESSER](src/COPYING.LESSER).
[NOTICE](NOTICE) and [AUTHORS](AUTHORS) carry attribution.
