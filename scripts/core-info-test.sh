#!/bin/sh
set -eu

if [ "$#" -ne 0 ]; then
    echo 'Usage: core-info-test.sh (set CTG_FRONTEND to auto, native, or retrodeck)' >&2
    exit 2
fi
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
core=$root/build/colortvgame_libretro.so
info=$root/colortvgame_libretro.info
config=$root/scripts/launcher-input.cfg
for file in "$core" "$info" "$config"; do
    [ -r "$file" ] || { printf 'File not found or unreadable: %s\n' "$file" >&2; exit 1; }
done
frontend=${CTG_FRONTEND:-auto}
case $frontend in
    auto|native|retrodeck) ;;
    *) printf 'Invalid CTG_FRONTEND: %s (use auto, native, or retrodeck)\n' "$frontend" >&2; exit 2 ;;
esac

tmp=$(mktemp -d "${TMPDIR:-/tmp}/ctg-core-info.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
trap 'exit 1' HUP INT TERM
tmp=$(CDPATH= cd -- "$tmp" && pwd -P)
case $tmp in
    *'
'*) echo 'Temporary directory paths containing newlines are not supported.' >&2; exit 1 ;;
esac
mkdir "$tmp/info" "$tmp/cores"
cp "$info" "$tmp/info/colortvgame_libretro.info"
cp "$core" "$tmp/cores/colortvgame_libretro.so"
cp "$config" "$tmp/retroarch.cfg"
# Escape the path for RetroArch's quoted configuration values.
escaped_info=$(printf '%s' "$tmp/info" | sed 's/\\/\\\\/g; s/"/\\"/g')
escaped_cores=$(printf '%s' "$tmp/cores" | sed 's/\\/\\\\/g; s/"/\\"/g')
printf '\nlibretro_info_path = "%s"\nlibretro_directory = "%s"\nconfig_save_on_exit = "false"\n' "$escaped_info" "$escaped_cores" >> "$tmp/retroarch.cfg"

if [ "$frontend" != retrodeck ] && command -v retroarch >/dev/null 2>&1; then
    retroarch --appendconfig "$tmp/retroarch.cfg" -L "$tmp/cores/colortvgame_libretro.so"
elif [ "$frontend" = native ]; then
    echo 'Native retroarch not found in PATH.' >&2
    exit 1
elif command -v flatpak >/dev/null 2>&1 && flatpak info net.retrodeck.retrodeck >/dev/null 2>&1; then
    case $root in *:*) echo 'RetroDECK cannot access a project path containing a colon.' >&2; exit 1 ;; esac
    case $tmp in *:*) echo 'RetroDECK cannot access a temporary path containing a colon.' >&2; exit 1 ;; esac
    flatpak run --filesystem="$root:ro" --filesystem="$tmp:ro" --command=/app/retrodeck/components/retroarch/bin/retroarch net.retrodeck.retrodeck --appendconfig "$tmp/retroarch.cfg" -L "$tmp/cores/colortvgame_libretro.so"
else
    if [ "$frontend" = retrodeck ]; then
        echo 'RetroDECK Flatpak (net.retrodeck.retrodeck) not found.' >&2
    else
        echo 'Neither native retroarch nor the RetroDECK Flatpak was found.' >&2
    fi
    exit 1
fi
