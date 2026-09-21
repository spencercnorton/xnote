#!/usr/bin/env bash
# Build the Debian package and the source tarball published beside it.
#
#   scripts/build-deb.sh [outdir]     -> outdir/xnote_<version>_<arch>.deb
#                                        outdir/xnote_<version>.tar.gz
#
# Run from the public export tree (CI does, so the package and the tarball
# hold exactly the public surface) or from a clean checkout. A private
# development checkout (one that carries .public-release.toml) gets the .deb
# only: its tree must never become the tarball served as source. The tarball
# is the tree as found, so build products from an earlier configure would
# ship in it — use a clean tree.
# debian/changelog is generated from configure.ac's AC_INIT, so the version
# has one home; a checkout that tracks its own debian/changelog gets it back
# when the script exits.
# Needs the build dependencies of debian/control, dpkg-buildpackage, and a Go
# toolchain at least as new as cloud-helper/go.mod's `go` line (the build
# runs with GOTOOLCHAIN=local, so an older Go fails instead of downloading).
# GNU coreutils (`date -d`, `tar --sort`) — Linux only, which is where a .deb
# is built.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
out=$(realpath -m "${1:-$root/dist}")
version=$(sed -n 's/^AC_INIT(\[XNote\],\[\([^]]*\)\],.*/\1/p' "$root/configure.ac")
test -n "$version"
go_want=$(sed -n 's/^go \([0-9.]*\)$/\1/p' "$root/cloud-helper/go.mod")
test -n "$go_want"
arch=$(dpkg --print-architecture)
stamp=${SOURCE_DATE_EPOCH:-$(date +%s)}
export SOURCE_DATE_EPOCH="$stamp"
mkdir -p "$out"

# The source first, before the build writes anything into the tree. The
# screenshots are not source and would add megabytes to every archive release.
if [ -e "$root/.public-release.toml" ]; then
    echo 'private checkout: building the .deb only; the source tarball comes from the export tree' >&2
else
    tar -C "$root/.." --sort=name --mtime="@$stamp" --owner=0 --group=0 --numeric-owner \
        --exclude=.git --exclude=dist --exclude=debian/changelog --exclude=screenshots \
        --transform "s|^$(basename "$root")|xnote-$version|" \
        -czf "$out/xnote_$version.tar.gz" "$(basename "$root")"
fi

changelog="$root/debian/changelog"
saved=$(mktemp)
if [ -f "$changelog" ]; then
    cp "$changelog" "$saved"
    trap 'cp "$saved" "$changelog"; rm -f "$saved"' EXIT
else
    trap 'rm -f "$saved" "$changelog"' EXIT
fi
cat > "$changelog" <<CHANGELOG
xnote ($version) resolute; urgency=medium

  * Release $version. The release notes:
    https://github.com/spencercnorton/xnote/blob/main/CHANGELOG.md

 -- Norvi <apt@globalentry.systems>  $(date -u -d "@$stamp" '+%a, %d %b %Y %H:%M:%S +0000')
CHANGELOG
(cd "$root" && dpkg-buildpackage -us -uc -b)
deb="$out/xnote_${version}_${arch}.deb"
mv "$root/../xnote_${version}_${arch}.deb" "$deb"
rm -f "$root"/../xnote_"${version}"_*.buildinfo "$root"/../xnote_"${version}"_*.changes \
    "$root"/../xnote-dbgsym_"${version}"_*.ddeb

# The backup timer is opt-in: installing the package must never enable or
# start it for anyone. The helper must be the patched toolchain's build, not
# whatever an older distro Go would have produced.
# dh_installsystemduser --no-enable emits a postinst that re-enables the unit
# only where `debian-installed` says a previous install had it; the enabling
# variant has no such guard and enables on a fresh install. Assert the guard
# is there, and that nothing starts the unit.
control_dir=$(mktemp -d)
dpkg-deb --control "$deb" "$control_dir"
if ! grep -Fq -- "--user debian-installed 'xnote-cloud-backup.timer'" "$control_dir/postinst"; then
    echo 'ERROR - postinst lacks the --no-enable guard: a fresh install would enable the opt-in backup timer.' >&2
    exit 1
fi
if grep -Eq 'deb-systemd-invoke.*[[:space:]]start' "$control_dir/postinst"; then
    echo 'ERROR - postinst would start the opt-in backup unit.' >&2
    exit 1
fi
package_tree=$(mktemp -d)
dpkg-deb --extract "$deb" "$package_tree"
go_built=$(go version -m "$package_tree/usr/bin/xnote-cloud-backup" | sed -n '1s/.*: go//p')
test "$(printf '%s\n%s\n' "$go_want" "$go_built" | sort -V | head -n 1)" = "$go_want" || {
    echo "ERROR - xnote-cloud-backup was built with go$go_built; go.mod requires go$go_want or newer." >&2
    exit 1
}
rm -rf "$control_dir" "$package_tree"
ls -l "$out"
