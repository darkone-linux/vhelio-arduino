#!/usr/bin/env bash
# Appele par build-nix.sh, DANS le nix shell qui fournit la chaine AVR.
# Ne pas lancer directement.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARDUINO_DIR="$ROOT/.arduino"
CLI="$ARDUINO_DIR/bin/arduino-cli"
PLATFORM_DIR="$ARDUINO_DIR/data/packages/arduino/hardware/avr/1.8.8"
CTAGS_STUB="$ARDUINO_DIR/data/packages/builtin/tools/ctags/5.8-arduino11/ctags"
TC_BIN="$ROOT/.build/toolchain/bin"

[ -x "$CLI" ] || { echo "Lancer d'abord ./tools/setup.sh"; exit 1; }

FQBN="arduino:avr:nano:cpu=atmega328"
[ "${1:-}" = "old" ] && FQBN="arduino:avr:nano:cpu=atmega328old"

# VHELIO_SKETCH permet de compiler tools/pinscan au lieu du firmware.
SKETCH="${VHELIO_SKETCH:-$ROOT/firmware/vhelio}"
OUTDIR="$ROOT/.build/$(basename "$SKETCH")"

# --- Ferme de liens ---------------------------------------------------------
# arduino-cli exige un repertoire unique pour compiler.path, alors que nix
# disperse les outils dans le store.
rm -rf "$TC_BIN"
mkdir -p "$TC_BIN"
for t in avr-gcc avr-g++ avr-ar avr-ranlib avr-objcopy avr-objdump \
         avr-size avr-nm avr-strip avr-ld; do
  if p="$(command -v "$t" 2>/dev/null)"; then ln -sf "$p" "$TC_BIN/$t"; fi
done

# --- avr-gcc-ar -------------------------------------------------------------
# nixpkgs ne le fournit pas. Sans lui, core.a est archive sans le greffon LTO,
# et le lien echoue sur des symboles introuvables (pinMode, digitalWrite...).
LTO_DIR="$(dirname "$(avr-gcc -print-prog-name=lto-wrapper)")"
{
  echo '#!/bin/sh'
  echo "exec \"$TC_BIN/avr-ar\" --plugin \"$LTO_DIR/liblto_plugin.so\" \"\$@\""
} > "$TC_BIN/avr-gcc-ar"
chmod +x "$TC_BIN/avr-gcc-ar"

# --- Redirection de la chaine de compilation --------------------------------
# -fno-use-cxa-atexit : le gcc AVR de nixpkgs est configure avec __cxa_atexit,
# absent de l'avr-libc embarquee. Sans ce drapeau, tout objet statique ayant un
# destructeur (Serial, SoftwareSerial) casse le lien sur __dso_handle.
{
  echo "compiler.path=$TC_BIN/"
  echo "compiler.ar.cmd=avr-gcc-ar"
  echo "compiler.cpp.extra_flags=-fno-use-cxa-atexit"
} > "$PLATFORM_DIR/platform.local.txt"

# --- ctags ------------------------------------------------------------------
# arduino-cli genere les prototypes du .ino avec un ctags prebuilt, lui aussi
# lie dynamiquement. On lui substitue l'Exuberant Ctags de nixpkgs.
#
# Ce ctags n'emet pas le champ typeref attendu : les prototypes generes
# seraient depourvus de type de retour. C'est sans consequence ici parce que
# vhelio.ino ne contient AUCUNE fonction (tout est dans src/), donc rien n'est
# genere. Ne pas y remettre de code : le probleme reviendrait.
#
# Ne pas remplacer par universal-ctags : il decale les numeros de ligne d'une
# unite, ce qui fait inserer les prototypes A L'INTERIEUR de setup(), qui
# s'appelle alors lui-meme. Le firmware compile, et part en recursion au boot.
if [ -e "$CTAGS_STUB" ] && [ ! -L "$CTAGS_STUB" ]; then
  mv "$CTAGS_STUB" "$CTAGS_STUB.orig"
fi
ln -sf "$(command -v ctags)" "$CTAGS_STUB"

# --- Compilation ------------------------------------------------------------
export ARDUINO_DIRECTORIES_DATA="$ARDUINO_DIR/data"
export ARDUINO_DIRECTORIES_DOWNLOADS="$ARDUINO_DIR/downloads"
export ARDUINO_DIRECTORIES_USER="$ARDUINO_DIR/user"

exec "$CLI" compile \
  --fqbn "$FQBN" \
  --warnings all \
  --build-path "$OUTDIR" \
  "$SKETCH"
