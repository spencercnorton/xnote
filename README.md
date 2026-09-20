<p align="center">
  <img src="images/hicolor/scalable/apps/xnote.svg" alt="XNote icon" width="96">
</p>

<h1 align="center">XNote</h1>

<p align="center">
  <strong>Sticky notes that stay on your desktop, and stay yours.</strong><br>
  A native GTK4/libadwaita notes app for GNOME on Wayland, descended from Xpad.
</p>

<p align="center">
  <a href="COPYING"><img alt="GPL-3.0-or-later" src="https://img.shields.io/badge/licence-GPL--3.0--or--later-blue.svg"></a>
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

## Requirements

XNote is tested on Ubuntu 26.04 with GNOME Shell 50. Building it requires:

- GTK 4.10 or newer
- libadwaita 1.4 or newer
- GtkSourceView 5
- GLib/GIO 2.66 or newer
- Pango and GdkPixbuf development files
- Autoconf, Automake, Libtool, Gettext, and a C compiler
- Go 1.26.8 or newer only when building `xnote-cloud-backup` (older releases
  contain a security flaw in the archive parser used during restore)

Other recent Linux distributions should work when they provide compatible
libraries. XNote does not directly link against X11 libraries.

## Build and install

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
NOCONFIGURE=1 ./autogen.sh
./configure --prefix=/usr
make -j"$(nproc)"
sudo make install
```

Run `xnote` to start the application. Existing Xpad notes under
`~/.config/xpad` are migrated to `~/.config/xnote` on first launch when the
XNote directory does not already exist.

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
hourly user timer:

```sh
xnote-cloud-backup init
xnote-cloud-backup backup
systemctl --user enable --now xnote-cloud-backup.timer
```

The pictures above were captured from a fresh, isolated profile with invented
notes; no personal note or desktop data appears in them.

## Development

```sh
make check
cd cloud-helper && go test -mod=vendor ./...
```

The Wayland smoke test requires a headless Weston compositor; CI runs it
against the installed build tree. Bugs and feature requests belong in the
[GitHub issue tracker](https://github.com/spencercnorton/xnote/issues).

## Provenance and license

XNote is a fork of [Xpad](https://launchpad.net/xpad), based on its public
development tree. See `NOTICE` and `AUTHORS` for attribution.

XNote is free software under the GNU General Public License version 3 or, at
your option, any later version. See `COPYING`.
