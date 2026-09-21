## What changed

<!-- One paragraph. Link the issue if there is one: "Fixes #12". -->

## Why

## How it was tested

<!-- `make check` / `make check-go` output, whether tests/wayland-smoke.sh passed, or what you ran by hand and on which distro / GNOME Shell / GTK / libadwaita versions. -->

## Checklist

- [ ] Builds with `./configure --enable-debug=most` (`-Wall -Werror`, what CI uses)
- [ ] `make check` passes; `make check-go` passes if `cloud-helper/` changed
- [ ] No new direct X11 linkage (`readelf -d src/xnote | grep NEEDED`)
- [ ] `xpad_` file and symbol names left as they are
- [ ] No secrets, machine names, or personal paths in the diff
- [ ] Docs updated if behaviour changed (README, `docs/user-guide.md`, `doc/xnote.1`, `doc/xnote-user-help.txt`; `doc/xnote-cloud-backup.1` for the helper)

<!--
How this lands: this repository is a release mirror. A maintainer reviews the
pull request here, applies accepted changes to the development tree, and the
change ships in the next tagged release — the pull request is then closed
with a reference to that release. See CONTRIBUTING.md.
-->
