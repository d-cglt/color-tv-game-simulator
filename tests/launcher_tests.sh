#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
tmp=$(CDPATH= cd -- "$tmp" && pwd -P)
project=$tmp/project\ with\ spaces
mkdir -p "$project/scripts" "$project/content" "$project/build" "$tmp/bin"
cp "$root"/scripts/*.sh "$root/scripts/launcher-input.cfg" "$project/scripts/"
cp "$root"/content/*.ctg "$project/content/"
cp "$root/colortvgame_libretro.info" "$project/"
printf 'test core binary\n' > "$project/build/colortvgame_libretro.so"
cat > "$tmp/bin/retroarch" <<'EOF'
#!/bin/sh
if [ "${CTG_TEST_INFO:-}" = 1 ]; then exec "$CTG_TEST_HELPER" native "$@"; fi
printf '%s\n' "$@" > "$CTG_TEST_LOG"
EOF
cat > "$tmp/bin/flatpak" <<'EOF'
#!/bin/sh
if [ "$1" = info ]; then exit 0; fi
if [ "${CTG_TEST_INFO:-}" = 1 ]; then exec "$CTG_TEST_HELPER" retrodeck "$@"; fi
printf '%s\n' "$@" > "$CTG_TEST_LOG"
EOF
cat > "$tmp/bin/check-info" <<'EOF'
#!/bin/sh
set -eu
mode=$1
shift
if [ "$mode" = retrodeck ]; then
    [ "$1" = run ] && [ "$2" = "--filesystem=$CTG_TEST_PROJECT:ro" ]
    access=$3
    [ "$4" = --command=/app/retrodeck/components/retroarch/bin/retroarch ]
    [ "$5" = net.retrodeck.retrodeck ]
    shift 5
fi
[ "$#" -eq 4 ] && [ "$1" = --appendconfig ] && [ "$3" = -L ]
config=$2
dir=${config%/retroarch.cfg}
[ "$4" = "$dir/cores/colortvgame_libretro.so" ]
cmp "$CTG_TEST_PROJECT/build/colortvgame_libretro.so" "$4"
[ "$mode" != retrodeck ] || [ "$access" = "--filesystem=$dir:ro" ]
cmp "$CTG_TEST_PROJECT/colortvgame_libretro.info" "$dir/info/colortvgame_libretro.info"
grep -Fx "libretro_info_path = \"$dir/info\"" "$config" >/dev/null
grep -Fx "libretro_directory = \"$dir/cores\"" "$config" >/dev/null
while IFS= read -r line; do grep -Fx "$line" "$config" >/dev/null; done < "$CTG_TEST_PROJECT/scripts/launcher-input.cfg"
grep -Fx 'config_save_on_exit = "false"' "$config" >/dev/null
printf '%s\n' "$dir" > "$CTG_TEST_LOG"
exit "${CTG_TEST_EXIT:-0}"
EOF
chmod +x "$tmp/bin/retroarch" "$tmp/bin/flatpak" "$tmp/bin/check-info"
PATH=$tmp/bin:$PATH
CTG_TEST_LOG=$tmp/args
export PATH CTG_TEST_LOG

cat > "$tmp/expected-config" <<'EOF'
config_save_on_exit = "false"
rewind_enable = "false"
savestate_auto_load = "false"
savestate_auto_save = "false"
input_player1_up = "nul"
input_player1_down = "nul"
input_player2_up = "up"
input_player2_down = "down"
EOF
cmp "$tmp/expected-config" "$project/scripts/launcher-input.cfg"

slot=0
for name in tennis-a-singles tennis-a-doubles tennis-b-singles tennis-b-doubles \
    hockey-a-singles hockey-a-doubles hockey-b-singles hockey-b-doubles \
    volleyball-a-singles volleyball-a-doubles volleyball-b-singles volleyball-b-doubles \
    ping-pong-singles ping-pong-doubles shooting-game-original; do
    id=$(printf '%02d' "$slot")
    CTG_FRONTEND=native sh "$project/scripts/launch.sh" "$name"
    printf '%s\n' --appendconfig "$project/scripts/launcher-input.cfg" -L "$project/build/colortvgame_libretro.so" "$project/content/slot-$id.ctg" > "$tmp/expected"
    cmp "$tmp/expected" "$CTG_TEST_LOG"
    CTG_FRONTEND=native "$project/scripts/$name.sh"
    cmp "$tmp/expected" "$CTG_TEST_LOG"
    case $name in
        *-a-*) public_name=$(printf '%s' "$name" | sed 's/-a-/-variant-/') ;;
        *-b-*) public_name=$(printf '%s' "$name" | sed 's/-b-/-original-/') ;;
        *) public_name=$name ;;
    esac
    CTG_FRONTEND=native sh "$project/scripts/launch.sh" "$public_name"
    cmp "$tmp/expected" "$CTG_TEST_LOG"
    # Verify the selector itself contains the matching ID, not just the filename.
    actual=$(od -An -tu1 "$project/content/slot-$id.ctg")
    set -- $actual
    [ "$#" -eq 5 ] && [ "$1" -eq 67 ] && [ "$2" -eq 84 ] && [ "$3" -eq 71 ] && [ "$4" -eq 49 ] && [ "$5" -eq "$slot" ]
    slot=$((slot + 1))
done
CTG_FRONTEND=native "$project/scripts/no-content.sh"
printf '%s\n' --appendconfig "$project/scripts/launcher-input.cfg" -L "$project/build/colortvgame_libretro.so" > "$tmp/expected"
cmp "$tmp/expected" "$CTG_TEST_LOG"
CTG_FRONTEND=retrodeck "$project/scripts/no-content.sh"
printf '%s\n' run "--filesystem=$project:ro" --command=/app/retrodeck/components/retroarch/bin/retroarch net.retrodeck.retrodeck --appendconfig "$project/scripts/launcher-input.cfg" -L "$project/build/colortvgame_libretro.so" > "$tmp/expected"
cmp "$tmp/expected" "$CTG_TEST_LOG"
CTG_FRONTEND=retrodeck sh "$project/scripts/launch.sh" tennis-b-singles
printf '%s\n' run "--filesystem=$project:ro" --command=/app/retrodeck/components/retroarch/bin/retroarch net.retrodeck.retrodeck --appendconfig "$project/scripts/launcher-input.cfg" -L "$project/build/colortvgame_libretro.so" "$project/content/slot-02.ctg" > "$tmp/expected"
cmp "$tmp/expected" "$CTG_TEST_LOG"
CTG_TEST_HELPER=$tmp/bin/check-info
CTG_TEST_PROJECT=$project
export CTG_TEST_HELPER CTG_TEST_PROJECT
mkdir "$tmp/temp with spaces"
for frontend in native retrodeck auto; do
    CTG_TEST_INFO=1 CTG_FRONTEND=$frontend TMPDIR="$tmp/temp with spaces" "$project/scripts/core-info-test.sh"
    info_dir=$(cat "$CTG_TEST_LOG")
    [ ! -e "$info_dir" ] || { echo 'Temporary metadata not cleaned up' >&2; exit 1; }
done
if CTG_TEST_INFO=1 CTG_TEST_EXIT=42 CTG_FRONTEND=native TMPDIR="$tmp/temp with spaces" "$project/scripts/core-info-test.sh"; then
    echo 'Frontend failure not propagated' >&2; exit 1
else
    [ "$?" -eq 42 ]
fi
info_dir=$(cat "$CTG_TEST_LOG")
[ ! -e "$info_dir" ]
if CTG_FRONTEND=invalid "$project/scripts/core-info-test.sh" >/dev/null 2>&1; then echo 'Invalid frontend accepted' >&2; exit 1; fi
if sh "$project/scripts/launch.sh" nonexistent >/dev/null 2>&1; then echo 'Unknown game accepted' >&2; exit 1; fi
rm "$project/build/colortvgame_libretro.so"
if sh "$project/scripts/launch.sh" tennis-b-singles >/dev/null 2>&1; then echo 'Missing core accepted' >&2; exit 1; fi
echo 'Launcher mappings and frontend arguments passed.'
