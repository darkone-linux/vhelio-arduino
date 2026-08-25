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
#   VHELIO_SKETCH=$PWD/tools/pinscan ./tools/upload.sh /dev/ttyACM0 old
#
# Le Nano de ce projet a un ANCIEN bootloader : le mot-cle "old" (57600 bauds)
# est obligatoire. Il ne concerne que ce script, pas la compilation : atmega328
# et atmega328old produisent le meme binaire.
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

# --- Port deja ouvert -------------------------------------------------------
# Une console laissee ouverte (tools/monitor.sh, screen, picocom) ne verrouille
# pas le port : avrdude l'ouvre quand meme, mais les deux se partagent les
# octets du bootloader et la synchro n'aboutit jamais. Le message est le meme
# "not in sync" qu'un mauvais debit, d'ou la verification prealable.
if command -v fuser >/dev/null 2>&1 && fuser -s "$PORT" 2>/dev/null; then
  echo "$PORT est deja ouvert par un autre programme :"
  fuser -v "$PORT" 2>&1 || true
  echo
  echo "Fermer la console (Ctrl-C) avant de televerser."
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
if ! "${AVRDUDE[@]}" -p atmega328p -c arduino -P "$PORT" -b "$SPEED" \
                     -D -U "flash:w:$HEX:i"; then
  echo
  echo "Echec du televersement."
  if [ "$SPEED" = 115200 ]; then
    # Le Nano de ce projet a un ANCIEN bootloader : il ecoute a 57600. A
    # 115200 avrdude enchaine les "not in sync: resp=0x00", qui ressemblent
    # a une panne de cablage. C'est la cause la plus frequente ici.
    echo "Ce Nano a un ancien bootloader. Reessayer a 57600 bauds :"
    echo "    VHELIO_SKETCH=$SKETCH $0 $PORT old"
  else
    echo "Verifier, dans l'ordre :"
    echo "  - l'inverseur 485_ON / PRO de la carte, qui doit etre sur PRO"
    echo "    (sinon le RS485 occupe D0/D1) ;"
    echo "  - le cable USB (certains ne portent que l'alimentation) ;"
    echo "  - le lien seul, sans rien ecrire :"
    echo "    avrdude -v -p atmega328p -c arduino -P $PORT -b $SPEED"
  fi
  exit 1
fi

echo "== Televerse. Console : $ROOT/tools/monitor.sh $PORT =="
