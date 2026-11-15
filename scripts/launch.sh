#!/bin/sh
set -eu

usage() {
    cat <<'EOF'
Usage: launch.sh GAME
Games (provisional names):
  tennis-original-singles tennis-original-doubles tennis-variant-singles tennis-variant-doubles
  hockey-original-singles hockey-original-doubles hockey-variant-singles hockey-variant-doubles
  volleyball-original-singles volleyball-original-doubles volleyball-variant-singles volleyball-variant-doubles
  ping-pong-singles   ping-pong-doubles   shooting-game-original
  no-content          (open game selector)
Legacy a/b names remain supported (a = variant, b = original).
Set CTG_FRONTEND to auto (default), native, or retrodeck.
EOF
}

[ "$#" -eq 1 ] || { usage >&2; exit 2; }
case $1 in
    tennis-variant-singles|tennis-a-singles) selector='Tennis Variant - Singles (Color TV Game 15).ctg' ;;
    tennis-variant-doubles|tennis-a-doubles) selector='Tennis Variant - Doubles (Color TV Game 15).ctg' ;;
    tennis-original-singles|tennis-b-singles) selector='Tennis Original - Singles (Color TV Game 6 + 15).ctg' ;;
    tennis-original-doubles|tennis-b-doubles) selector='Tennis Original - Doubles (Color TV Game 6 + 15).ctg' ;;
    hockey-variant-singles|hockey-a-singles) selector='Hockey Variant - Singles (Color TV Game 15).ctg' ;;
    hockey-variant-doubles|hockey-a-doubles) selector='Hockey Variant - Doubles (Color TV Game 15).ctg' ;;
    hockey-original-singles|hockey-b-singles) selector='Hockey Original - Singles (Color TV Game 6 + 15).ctg' ;;
    hockey-original-doubles|hockey-b-doubles) selector='Hockey Original - Doubles (Color TV Game 6 + 15).ctg' ;;
    volleyball-variant-singles|volleyball-a-singles) selector='Volleyball Variant - Singles (Color TV Game 15).ctg' ;;
    volleyball-variant-doubles|volleyball-a-doubles) selector='Volleyball Variant - Doubles (Color TV Game 15).ctg' ;;
    volleyball-original-singles|volleyball-b-singles) selector='Volleyball Original - Singles (Color TV Game 6 + 15).ctg' ;;
    volleyball-original-doubles|volleyball-b-doubles) selector='Volleyball Original - Doubles (Color TV Game 6 + 15).ctg' ;;
    ping-pong-singles) selector='Ping-Pong - Singles (Color TV Game 15).ctg' ;;
    ping-pong-doubles) selector='Ping-Pong - Doubles (Color TV Game 15).ctg' ;;
    shooting-game-original) selector='Shooting Game (Color TV Game 15).ctg' ;;
    no-content) selector= ;;
    -h|--help) usage; exit 0 ;;
    *) printf 'Unknown game: %s\n' "$1" >&2; usage >&2; exit 2 ;;
esac

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P) || exit 1
core=$root/build/colortvgame_libretro.so
config=$root/scripts/launcher-input.cfg
info=$root/colortvgame_libretro.info
[ -r "$info" ] || { printf 'Core info not found or unreadable: %s\n' "$info" >&2; exit 1; }
[ -r "$core" ] || { printf 'Core not found or unreadable: %s\nBuild it with: cmake -S "%s" -B "%s/build" && cmake --build "%s/build"\n' "$core" "$root" "$root" "$root" >&2; exit 1; }
if [ -n "$selector" ]; then
    content=$root/content/$selector
    [ -r "$content" ] || { printf 'Content not found or unreadable: %s\n' "$content" >&2; exit 1; }
    set -- "$content"
else
    set --
fi
[ -r "$config" ] || { printf 'Launcher configuration not found or unreadable: %s\n' "$config" >&2; exit 1; }

frontend=${CTG_FRONTEND:-auto}
case $frontend in
    auto|native|retrodeck) ;;
    *) printf 'Invalid CTG_FRONTEND: %s (use auto, native, or retrodeck)\n' "$frontend" >&2; exit 2 ;;
esac
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ctg-core-info.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
child=
trap 'if [ -n "$child" ]; then kill "$child" 2>/dev/null || :; wait "$child" 2>/dev/null || :; fi; exit 1' HUP INT TERM
tmp=$(CDPATH= cd -- "$tmp" && pwd -P)
case $tmp in
    *'
'*) echo 'Temporary directory paths containing newlines are not supported.' >&2; exit 1 ;;
esac
case $root in
    *'
'*) echo 'Project paths containing newlines are not supported.' >&2; exit 1 ;;
esac
mkdir "$tmp/info"
cp "$info" "$tmp/info/colortvgame_libretro.info"
cp "$config" "$tmp/retroarch.cfg"
escaped_info=$(printf '%s' "$tmp/info" | sed 's/\\/\\\\/g; s/"/\\"/g')
escaped_cores=$(printf '%s' "$root/build" | sed 's/\\/\\\\/g; s/"/\\"/g')
printf '\nlibretro_info_path = "%s"\nlibretro_directory = "%s"\nconfig_save_on_exit = "false"\n' "$escaped_info" "$escaped_cores" >> "$tmp/retroarch.cfg"

run_frontend() {
    "$@" <&0 &
    child=$!
    wait "$child"
    child=
}

if [ "$frontend" != retrodeck ] && command -v retroarch >/dev/null 2>&1; then
    run_frontend retroarch --appendconfig "$tmp/retroarch.cfg" -L "$core" "$@"
    exit $?
fi
if [ "$frontend" = native ]; then
    echo 'Native retroarch not found in PATH.' >&2
    exit 1
fi
if command -v flatpak >/dev/null 2>&1 && flatpak info net.retrodeck.retrodeck >/dev/null 2>&1; then
    # Flatpak filesystem specifications use ':' as a delimiter.
    case $root in *:*) echo 'RetroDECK cannot access a project path containing a colon; move the project or use native RetroArch.' >&2; exit 1 ;; esac
    case $tmp in *:*) echo 'RetroDECK cannot access a temporary path containing a colon; change TMPDIR or use native RetroArch.' >&2; exit 1 ;; esac
    run_frontend flatpak run --filesystem="$root:ro" --filesystem="$tmp:ro" --command=/app/retrodeck/components/retroarch/bin/retroarch net.retrodeck.retrodeck --appendconfig "$tmp/retroarch.cfg" -L "$core" "$@"
    exit $?
fi
if [ "$frontend" = retrodeck ]; then
    echo 'RetroDECK Flatpak (net.retrodeck.retrodeck) not found.' >&2
else
    echo 'Neither native retroarch nor the RetroDECK Flatpak was found.' >&2
fi
exit 1
