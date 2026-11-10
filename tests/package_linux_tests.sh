#!/bin/sh
set -eu
root=$1
core=$2
loader=$3
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
sh "$root/scripts/package-linux.sh" "$core" "$tmp/package.tar.gz"
mkdir "$tmp/extracted"
tar -xzf "$tmp/package.tar.gz" -C "$tmp/extracted"
package=$tmp/extracted/colortvgame-linux
# Check both the exact payload and the redistributed notices.
find "$tmp/extracted" -type f -printf '%P\n' | LC_ALL=C sort > "$tmp/files"
printf '%s\n' colortvgame-linux/LICENSE colortvgame-linux/LICENSE-libretro.txt colortvgame-linux/README.md colortvgame-linux/colortvgame_libretro.so > "$tmp/expected"
cmp "$tmp/expected" "$tmp/files"
cmp "$root/LICENSE" "$package/LICENSE"
cmp "$root/docs/LINUX_RELEASE.md" "$package/README.md"
sed -n '1,28p' "$root/third_party/libretro/libretro.h" > "$tmp/notice"
cmp "$tmp/notice" "$package/LICENSE-libretro.txt"
cmp "$core" "$package/colortvgame_libretro.so"
"$loader" "$package/colortvgame_libretro.so"
