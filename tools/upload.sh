#!/usr/bin/env bash
# Televerse un croquis DEJA COMPILE.
#
# Usage : ./tools/upload.sh [port] [old]
#
# Ce script ne compile plus. La compilation passe par ./tools/build-nix.sh,
# qui seul monte la chaine AVR de nixpkgs ; recompiler ici, hors du nix shell,
# echouait sur le compiler.path redirige par nix-inner.sh.
#
# Le croquis se choisit avec VHELIO_SKETCH, exactement comme a la compilation :
#   VHELIO_SKETCH=$PWD/tools/pinscan ./tools/build-nix.sh
#   VHELIO_SKETCH=$PWD/tools/pinscan ./tools/upload.sh /dev/ttyACM0
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
ARDUINO_DIR="$ROOT/.arduino"
CLI="$ARDUINO_DIR/bin/arduino-cli"
[ -x "$CLI" ] || CLI="$(command -v arduino-cli || true)"

SKETCH="${VHELIO_SKETCH:-$ROOT/firmware/vhelio}"
NAME="$(basename "$SKETCH")"
HEX="$ROOT/.build/$NAME/$NAME.ino.hex"

if [ ! -f "$HEX" ]; then
  echo "Rien a televerser : $HEX est absent."
  echo "Compiler d'abord :  VHELIO_SKETCH=$SKETCH ./tools/build-nix.sh"
  exit 1
fi

# --- Port -------------------------------------------------------------------
# Le Nano officiel sort en /dev/ttyACM0 ; les clones a CH340 en /dev/ttyUSB0.
PORT="${1:-}"
if [ -z "$PORT" ]; then
  for p in /dev/ttyACM0 /dev/ttyUSB0; do
    [ -e "$p" ] && { PORT="$p"; break; }
  done
fi
[ -n "$PORT" ] || { echo "Aucun port serie trouve. Usage : upload.sh <port> [old]"; exit 1; }

SPEED=115200
[ "${2:-}" = "old" ] && SPEED=57600

# Sur NixOS le port appartient a root:dialout et l'utilisateur n'y est pas par
# defaut. avrdude echouerait sur un "can't open device" peu parlant.
if [ ! -r "$PORT" ] || [ ! -w "$PORT" ]; then
  echo "Pas d'acces en lecture/ecriture sur $PORT :"
  ls -l "$PORT"
  echo
  echo "Ajouter l'utilisateur au groupe proprietaire du port. Sur NixOS :"
  echo "    users.users.$USER.extraGroups = [ \"dialout\" ];"
  echo "puis nixos-rebuild switch et REOUVRIR LA SESSION (newgrp ne suffit"
  echo "pas pour les processus deja lances)."
  exit 1
fi

# --- avrdude ----------------------------------------------------------------
# Celui qu'embarque arduino-cli est lie dynamiquement contre un /lib inexistant
# sur NixOS : "Could not start dynamically linked executable". On prend donc,
# dans l'ordre, celui du systeme, celui de nixpkgs, puis le prebuilt.
BUNDLED="$ARDUINO_DIR/data/packages/arduino/tools/avrdude/8.0.0-arduino1/bin/avrdude"
if command -v avrdude >/dev/null 2>&1; then
  AVRDUDE=(avrdude)
elif command -v nix >/dev/null 2>&1; then
  AVRDUDE=(nix shell nixpkgs#avrdude --command avrdude)
elif [ -x "$BUNDLED" ]; then
  AVRDUDE=("$BUNDLED")
else
  echo "Aucun avrdude utilisable."; exit 1
fi

echo "== $NAME -> $PORT ($SPEED bauds) =="
"${AVRDUDE[@]}" -p atmega328p -c arduino -P "$PORT" -b "$SPEED" \
                -D -U "flash:w:$HEX:i"

echo "== Televerse. Console : $CLI monitor -p $PORT -c baudrate=115200 =="
