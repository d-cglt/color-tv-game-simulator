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
    tennis-variant-singles|tennis-a-singles) slot=00 ;;
    tennis-variant-doubles|tennis-a-doubles) slot=01 ;;
    tennis-original-singles|tennis-b-singles) slot=02 ;;
    tennis-original-doubles|tennis-b-doubles) slot=03 ;;
    hockey-variant-singles|hockey-a-singles) slot=04 ;;
    hockey-variant-doubles|hockey-a-doubles) slot=05 ;;
    hockey-original-singles|hockey-b-singles) slot=06 ;;
    hockey-original-doubles|hockey-b-doubles) slot=07 ;;
    volleyball-variant-singles|volleyball-a-singles) slot=08 ;;
    volleyball-variant-doubles|volleyball-a-doubles) slot=09 ;;
    volleyball-original-singles|volleyball-b-singles) slot=10 ;;
    volleyball-original-doubles|volleyball-b-doubles) slot=11 ;;
    ping-pong-singles) slot=12 ;;
    ping-pong-doubles) slot=13 ;;
    shooting-game-original) slot=14 ;;
    no-content) slot= ;;
    -h|--help) usage; exit 0 ;;
    *) printf 'Unknown game: %s\n' "$1" >&2; usage >&2; exit 2 ;;
esac

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P) || exit 1
core=$root/build/colortvgame_libretro.so
config=$root/scripts/launcher-input.cfg
[ -r "$core" ] || { printf 'Core not found or unreadable: %s\nBuild it with: cmake -S "%s" -B "%s/build" && cmake --build "%s/build"\n' "$core" "$root" "$root" "$root" >&2; exit 1; }
if [ -n "$slot" ]; then
    content=$root/content/slot-$slot.ctg
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
if [ "$frontend" != retrodeck ] && command -v retroarch >/dev/null 2>&1; then
    exec retroarch --appendconfig "$config" -L "$core" "$@"
fi
if [ "$frontend" = native ]; then
    echo 'Native retroarch not found in PATH.' >&2
    exit 1
fi
if command -v flatpak >/dev/null 2>&1 && flatpak info net.retrodeck.retrodeck >/dev/null 2>&1; then
    # Flatpak filesystem specifications use ':' as a delimiter.
    case $root in *:*) echo 'RetroDECK cannot access a project path containing a colon; move the project or use native RetroArch.' >&2; exit 1 ;; esac
    exec flatpak run --filesystem="$root:ro" --command=/app/retrodeck/components/retroarch/bin/retroarch net.retrodeck.retrodeck --appendconfig "$config" -L "$core" "$@"
fi
if [ "$frontend" = retrodeck ]; then
    echo 'RetroDECK Flatpak (net.retrodeck.retrodeck) not found.' >&2
else
    echo 'Neither native retroarch nor the RetroDECK Flatpak was found.' >&2
fi
exit 1
