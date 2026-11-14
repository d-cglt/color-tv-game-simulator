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
[ "$1" = --appendconfig ] && [ "$3" = -L ]
if [ -n "${CTG_TEST_CONTENT:-}" ]; then
    [ "$#" -eq 5 ] && [ "$5" = "$CTG_TEST_CONTENT" ]
else
    [ "$#" -eq 4 ]
fi
config=$2
dir=${config%/retroarch.cfg}
if [ "${CTG_TEST_LEGACY_INFO:-}" = 1 ]; then
    [ "$4" = "$dir/cores/colortvgame_libretro.so" ]
    cmp "$CTG_TEST_PROJECT/build/colortvgame_libretro.so" "$4"
    cores=$dir/cores
else
    [ "$4" = "$CTG_TEST_PROJECT/build/colortvgame_libretro.so" ]
    [ ! -e "$dir/cores" ]
    cores=$CTG_TEST_PROJECT/build
fi
[ "$mode" != retrodeck ] || [ "$access" = "--filesystem=$dir:ro" ]
cmp "$CTG_TEST_PROJECT/colortvgame_libretro.info" "$dir/info/colortvgame_libretro.info"
escaped_info=$(printf '%s' "$dir/info" | sed 's/\\/\\\\/g; s/"/\\"/g')
escaped_cores=$(printf '%s' "$cores" | sed 's/\\/\\\\/g; s/"/\\"/g')
grep -Fx "libretro_info_path = \"$escaped_info\"" "$config" >/dev/null
grep -Fx "libretro_directory = \"$escaped_cores\"" "$config" >/dev/null
while IFS= read -r line; do grep -Fx "$line" "$config" >/dev/null; done < "$CTG_TEST_PROJECT/scripts/launcher-input.cfg"
grep -Fx 'config_save_on_exit = "false"' "$config" >/dev/null
printf '%s\n' "$dir" > "$CTG_TEST_LOG"
if [ "${CTG_TEST_WAIT:-}" = 1 ]; then
    trap 'exit 0' TERM HUP INT
    while :; do sleep 1; done
fi
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

CTG_TEST_HELPER=$tmp/bin/check-info
CTG_TEST_PROJECT=$project
CTG_TEST_INFO=1
export CTG_TEST_HELPER CTG_TEST_PROJECT CTG_TEST_INFO
mkdir "$tmp/temp with spaces and \"quotes\""
TMPDIR=$tmp/temp\ with\ spaces\ and\ \"quotes\"
export TMPDIR
check_cleanup() {
    info_dir=$(cat "$CTG_TEST_LOG")
    [ ! -e "$info_dir" ] || { echo 'Temporary metadata not cleaned up' >&2; exit 1; }
}
slot=0
for name in tennis-a-singles tennis-a-doubles tennis-b-singles tennis-b-doubles \
    hockey-a-singles hockey-a-doubles hockey-b-singles hockey-b-doubles \
    volleyball-a-singles volleyball-a-doubles volleyball-b-singles volleyball-b-doubles \
    ping-pong-singles ping-pong-doubles shooting-game-original; do
    id=$(printf '%02d' "$slot")
    CTG_TEST_CONTENT=$project/content/slot-$id.ctg
    export CTG_TEST_CONTENT
    CTG_FRONTEND=native sh "$project/scripts/launch.sh" "$name"
    check_cleanup
    CTG_FRONTEND=native "$project/scripts/$name.sh"
    check_cleanup
    case $name in
        *-a-*) public_name=$(printf '%s' "$name" | sed 's/-a-/-variant-/') ;;
        *-b-*) public_name=$(printf '%s' "$name" | sed 's/-b-/-original-/') ;;
        *) public_name=$name ;;
    esac
    CTG_FRONTEND=native sh "$project/scripts/launch.sh" "$public_name"
    check_cleanup
    # Verify the selector itself contains the matching ID, not just the filename.
    actual=$(od -An -tu1 "$project/content/slot-$id.ctg")
    set -- $actual
    [ "$#" -eq 5 ] && [ "$1" -eq 67 ] && [ "$2" -eq 84 ] && [ "$3" -eq 71 ] && [ "$4" -eq 49 ] && [ "$5" -eq "$slot" ]
    slot=$((slot + 1))
done
unset CTG_TEST_CONTENT
# Exercise a noexec temporary filesystem when the host provides one.
if [ -w /dev/shm ] && command -v findmnt >/dev/null 2>&1 &&
    findmnt -n -o OPTIONS -T /dev/shm | grep -Eq '(^|,)noexec(,|$)'; then
    TMPDIR=/dev/shm CTG_FRONTEND=native "$project/scripts/no-content.sh"
    check_cleanup
fi
for frontend in native retrodeck auto; do
    CTG_FRONTEND=$frontend "$project/scripts/no-content.sh"
    check_cleanup
done
CTG_TEST_CONTENT=$project/content/slot-02.ctg
export CTG_TEST_CONTENT
CTG_FRONTEND=retrodeck sh "$project/scripts/launch.sh" tennis-b-singles
check_cleanup
unset CTG_TEST_CONTENT
for frontend in native retrodeck; do
    if CTG_TEST_EXIT=42 CTG_FRONTEND=$frontend "$project/scripts/no-content.sh"; then
        echo 'Frontend failure not propagated' >&2; exit 1
    else
        [ "$?" -eq 42 ]
    fi
    check_cleanup
done
for frontend in native retrodeck; do
    rm -f "$CTG_TEST_LOG"
    CTG_TEST_WAIT=1 CTG_FRONTEND=$frontend "$project/scripts/no-content.sh" &
    launcher=$!
    count=0
    while [ ! -s "$CTG_TEST_LOG" ] && [ "$count" -lt 100 ]; do
        sleep 0.1
        count=$((count + 1))
    done
    [ -s "$CTG_TEST_LOG" ] || { kill "$launcher"; echo 'Frontend did not start' >&2; exit 1; }
    kill -TERM "$launcher"
    if wait "$launcher"; then echo 'Interrupted launcher succeeded' >&2; exit 1; fi
    check_cleanup
done
CTG_TEST_LEGACY_INFO=1
export CTG_TEST_LEGACY_INFO
for frontend in native retrodeck auto; do
    CTG_TEST_INFO=1 CTG_FRONTEND=$frontend "$project/scripts/core-info-test.sh"
    info_dir=$(cat "$CTG_TEST_LOG")
    [ ! -e "$info_dir" ] || { echo 'Temporary metadata not cleaned up' >&2; exit 1; }
done
if CTG_TEST_INFO=1 CTG_TEST_EXIT=42 CTG_FRONTEND=native "$project/scripts/core-info-test.sh"; then
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
