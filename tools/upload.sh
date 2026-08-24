#!/usr/bin/env bash
# Televerse le firmware. Usage : ./tools/upload.sh /dev/ttyUSB0 [old]
set -euo pipefail

PORT="${1:?usage: upload.sh <port> [old]}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARDUINO_DIR="$ROOT/.arduino"
CLI="$ARDUINO_DIR/bin/arduino-cli"

[ -x "$CLI" ] || CLI="$(command -v arduino-cli)"

export ARDUINO_DIRECTORIES_DATA="$ARDUINO_DIR/data"
export ARDUINO_DIRECTORIES_DOWNLOADS="$ARDUINO_DIR/downloads"
export ARDUINO_DIRECTORIES_USER="$ARDUINO_DIR/user"

FQBN="arduino:avr:nano:cpu=atmega328"
[ "${2:-}" = "old" ] && FQBN="arduino:avr:nano:cpu=atmega328old"

"$CLI" compile --fqbn "$FQBN" --build-path "$ROOT/.build" "$ROOT/firmware/vhelio"
"$CLI" upload  --fqbn "$FQBN" --port "$PORT" --input-dir "$ROOT/.build" "$ROOT/firmware/vhelio"

echo "== Televerse. Console : $CLI monitor -p $PORT -c baudrate=115200 =="
