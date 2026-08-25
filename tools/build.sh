#!/usr/bin/env bash
# Compile le firmware pour Arduino Nano (ATmega328P, bootloader recent).
# Passer "old" en argument pour un Nano a ancien bootloader.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
ARDUINO_DIR="$ROOT/.arduino"
CLI="$ARDUINO_DIR/bin/arduino-cli"

[ -x "$CLI" ] || CLI="$(command -v arduino-cli)"

export ARDUINO_DIRECTORIES_DATA="$ARDUINO_DIR/data"
export ARDUINO_DIRECTORIES_DOWNLOADS="$ARDUINO_DIR/downloads"
export ARDUINO_DIRECTORIES_USER="$ARDUINO_DIR/user"

FQBN="arduino:avr:nano:cpu=atmega328"
[ "${1:-}" = "old" ] && FQBN="arduino:avr:nano:cpu=atmega328old"

# Meme convention que build-nix.sh : un sous-repertoire par croquis. Compiler
# deux croquis dans le meme --build-path ne marche pas, arduino-cli le vide a
# chaque changement.
SKETCH="${VHELIO_SKETCH:-$ROOT/firmware/vhelio}"

# VHELIO_SIM=1 : firmware de banc (entrees pilotables au clavier). Le drapeau
# vient de la ligne de commande, pas de config.h, qui reste a 0 dans le depot.
# Repertoire de compilation separe : sinon arduino-cli reutilise des objets
# compiles avec l'autre jeu de drapeaux.
PROPS=()
SUFFIX=""
if [ "${VHELIO_SIM:-0}" = "1" ]; then
  PROPS=(--build-property "compiler.cpp.extra_flags=-DSIM_INPUTS=1")
  SUFFIX="-sim"
fi

"$CLI" compile \
  --fqbn "$FQBN" \
  --warnings all \
  ${PROPS[@]+"${PROPS[@]}"} \
  --build-path "$ROOT/.build/$(basename "$SKETCH")$SUFFIX" \
  "$SKETCH"
