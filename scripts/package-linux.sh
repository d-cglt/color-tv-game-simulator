#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "Usage: sh scripts/package-linux.sh <built-core.so> <output.tar.gz>" >&2
    exit 2
fi
core=$1
archive=$2
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
if [ ! -f "$core" ]; then
    echo "Core not found: $core" >&2
    exit 1
fi
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT HUP INT TERM
mkdir "$stage/colortvgame-linux"
cp "$core" "$stage/colortvgame-linux/colortvgame_libretro.so"
cp "$root/LICENSE" "$stage/colortvgame-linux/LICENSE"
cp "$root/docs/LINUX_RELEASE.md" "$stage/colortvgame-linux/README.md"
# Copy the notice directly from the vendored header to preserve its attribution and terms.
sed -n '1,28p' "$root/third_party/libretro/libretro.h" > "$stage/colortvgame-linux/LICENSE-libretro.txt"
tar -czf "$archive" -C "$stage" colortvgame-linux
