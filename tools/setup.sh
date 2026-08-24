#!/usr/bin/env bash
# Installe arduino-cli et le coeur AVR dans ./.arduino (local au projet,
# rien n'est installe au niveau systeme).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARDUINO_DIR="$ROOT/.arduino"
BIN_DIR="$ARDUINO_DIR/bin"

mkdir -p "$BIN_DIR"

if [ ! -x "$BIN_DIR/arduino-cli" ]; then
  echo "== Telechargement d'arduino-cli =="
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh \
    | BINDIR="$BIN_DIR" sh
fi

export ARDUINO_DIRECTORIES_DATA="$ARDUINO_DIR/data"
export ARDUINO_DIRECTORIES_DOWNLOADS="$ARDUINO_DIR/downloads"
export ARDUINO_DIRECTORIES_USER="$ARDUINO_DIR/user"

echo "== Installation du coeur AVR =="
"$BIN_DIR/arduino-cli" core update-index
"$BIN_DIR/arduino-cli" core install arduino:avr

echo "== Pret. Lancer ./tools/build.sh =="
