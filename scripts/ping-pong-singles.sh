#!/bin/sh
exec "$(dirname -- "$0")/launch.sh" ping-pong-singles "$@"
