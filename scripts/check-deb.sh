#!/usr/bin/env bash
# Check a built XNote package for what lintian cannot know about.
#
#   scripts/check-deb.sh xnote_<version>_<arch>.deb
#
# scripts/build-deb.sh runs it on the package it builds; CI runs it on its own.
# The backup timer is opt-in: installing the package must never enable or
# start it for anyone. The helper must be the patched toolchain's build, not
# whatever an older distro Go would have produced.
# Needs dpkg-deb and a `go` on PATH (`go version -m` reads which Go built the
# helper).
set -euo pipefail
[ $# -eq 1 ] || { echo 'usage: scripts/check-deb.sh xnote_<version>_<arch>.deb' >&2; exit 2; }
deb=$1
root=$(cd "$(dirname "$0")/.." && pwd)
go_want=$(sed -n 's/^go \([0-9.]*\)$/\1/p' "$root/cloud-helper/go.mod")
test -n "$go_want"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# dh_installsystemduser --no-enable emits a postinst that re-enables the unit
# only where `debian-installed` says a previous install had it; the enabling
# variant has no such guard and enables on a fresh install. Assert the guard
# is there, and that nothing starts the unit.
dpkg-deb --control "$deb" "$work/control"
if ! grep -Fq -- "--user debian-installed 'xnote-cloud-backup.timer'" "$work/control/postinst"; then
    echo 'ERROR - postinst lacks the --no-enable guard: a fresh install would enable the opt-in backup timer.' >&2
    exit 1
fi
if grep -Eq 'deb-systemd-invoke.*[[:space:]]start' "$work/control/postinst"; then
    echo 'ERROR - postinst would start the opt-in backup unit.' >&2
    exit 1
fi
dpkg-deb --extract "$deb" "$work/tree"
go_built=$(go version -m "$work/tree/usr/bin/xnote-cloud-backup" | sed -n '1s/.*: go//p')
test "$(printf '%s\n%s\n' "$go_want" "$go_built" | sort -V | head -n 1)" = "$go_want" || {
    echo "ERROR - xnote-cloud-backup was built with go$go_built; go.mod requires go$go_want or newer." >&2
    exit 1
}
