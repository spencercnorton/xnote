#!/bin/sh
# Install the exact Go toolchain used for XNote release builds.
#
# This file is sourced by GitLab CI so the PATH update remains active for the
# rest of the job. Checksums are from https://go.dev/dl/?mode=json.

set -eu

go_version=1.26.8
case "$(uname -m)" in
    x86_64)
        go_arch=amd64
        go_sha256=d0f743b33e8d8945e6b1f432edd15785c70507121d6e2a723b21285eddf8b57b
        ;;
    aarch64|arm64)
        go_arch=arm64
        go_sha256=211ffced9dcb9633a55eac6364816ec0ddd951389a740e88fa8b3337971bdda0
        ;;
    *)
        echo "unsupported release-builder architecture: $(uname -m)" >&2
        exit 1
        ;;
esac

go_archive="go${go_version}.linux-${go_arch}.tar.gz"
go_url="https://go.dev/dl/${go_archive}"
go_root="/opt/xnote-go-${go_version}"
go_tmp="$(mktemp -d)"
trap 'rm -rf "$go_tmp"' EXIT HUP INT TERM

curl --fail --location --proto '=https' --tlsv1.2 \
    --output "$go_tmp/$go_archive" "$go_url"
printf '%s  %s\n' "$go_sha256" "$go_tmp/$go_archive" | sha256sum --check --strict
test ! -e "$go_root"
mkdir -p "$go_root"
tar -C "$go_root" --strip-components=1 -xzf "$go_tmp/$go_archive"

PATH="$go_root/bin:$PATH"
export PATH
test "$(go version)" = "go version go${go_version} linux/${go_arch}"
echo "using $(go version)"
