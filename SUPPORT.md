# Support

## Where to ask

- **Something is broken** — open a
  [bug report](https://github.com/spencercnorton/xnote/issues/new/choose).
- **How do I…?** or **would it make sense to…?** — start a thread in
  [Discussions](https://github.com/spencercnorton/xnote/discussions).
- **A security problem** — never in public. Use
  [private vulnerability reporting](https://github.com/spencercnorton/xnote/security/advisories/new);
  see [SECURITY.md](SECURITY.md).

There is no e-mail address, chat room or forum; GitHub is the only channel.

## What to include

- The output of `xnote --version`, your distribution and release, and
  whether your session is Wayland or X11.
- For anything involving the tray icon: your desktop (GNOME Shell version,
  or the other environment) and whether the AppIndicator / KStatusNotifierItem
  extension is installed.
- What you did, what you expected, and what happened instead. If it
  reproduces, the steps.
- The terminal output: quit XNote (`xnote --quit`), start it again from a
  terminal, reproduce the problem and paste what was printed. Remove anything
  private first — note text, paths, hostnames.
- For backup questions: the output of `xnote-cloud-backup status` and the
  relevant lines of `~/.config/xnote/cloud-backup.conf`, with any remote name
  redacted. Never paste a recovery code, a passphrase or `keyfile.json`.

## What to expect

This repository is a **release mirror**: `main` moves forward one tagged
release at a time, and fixes land as the next release rather than as
individual commits. Issues are triaged by one maintainer in spare time;
an acknowledgement usually comes within a week.

Before opening a new issue, check the
[existing ones](https://github.com/spencercnorton/xnote/issues) and the
[CHANGELOG](CHANGELOG.md) for the release you are running — the fix may
already be out.
