#!/usr/bin/env bash
# Appele par build-nix.sh, DANS le nix shell qui fournit la chaine AVR.
# Ne pas lancer directement.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
ARDUINO_DIR="$ROOT/.arduino"
CLI="$ARDUINO_DIR/bin/arduino-cli"
# Versions resolues dynamiquement : un `core update-index && core install`
# peut faire monter le coeur AVR ou ctags, et un chemin code en dur casserait
# alors la compilation avec un message obscur. On prend la plus recente.
PLATFORM_DIR="$(printf '%s\n' "$ARDUINO_DIR"/data/packages/arduino/hardware/avr/*/ | sort -V | tail -n 1)"
CTAGS_STUB="$(printf '%s\n' "$ARDUINO_DIR"/data/packages/builtin/tools/ctags/*/ctags | sort -V | tail -n 1)"
[ -n "$PLATFORM_DIR" ] && [ -d "$PLATFORM_DIR" ] || { echo "Coeur AVR introuvable sous $ARDUINO_DIR/data/packages/arduino/hardware/avr/ : lancer ./tools/setup.sh"; exit 1; }
[ -n "$CTAGS_STUB" ] || { echo "ctags introuvable sous $ARDUINO_DIR/data/packages/builtin/tools/ctags/ : lancer ./tools/setup.sh"; exit 1; }
# La ferme de liens NE DOIT PAS etre sous .build/ : arduino-cli considere
# --build-path comme son repertoire a lui et le VIDE des qu'il change de
# croquis. La chaine de compilation disparaissait donc au premier build
# suivant, avec un "no such file or directory: .../avr-g++" a la cle.
TC_BIN="$ARDUINO_DIR/toolchain/bin"

[ -x "$CLI" ] || { echo "Lancer d'abord ./tools/setup.sh"; exit 1; }

FQBN="arduino:avr:nano:cpu=atmega328"
[ "${1:-}" = "old" ] && FQBN="arduino:avr:nano:cpu=atmega328old"

# VHELIO_SKETCH permet de compiler tools/pinscan au lieu du firmware.
SKETCH="${VHELIO_SKETCH:-$ROOT/firmware/vhelio}"

# VHELIO_SIM=1 : firmware de banc, entrees pilotables depuis la console.
# Le drapeau vient de la ligne de commande et NON de config.h, qui reste ainsi
# a 0 dans le depot : on ne peut pas oublier de l'y remettre. Le binaire va
# dans un repertoire separe, sinon arduino-cli reutiliserait des objets
# compiles avec l'autre jeu de drapeaux.
SIM_FLAG=""
SUFFIX=""
if [ "${VHELIO_SIM:-0}" = "1" ]; then
  SIM_FLAG=" -DSIM_INPUTS=1"
  SUFFIX="-sim"
fi

OUTDIR="$ROOT/.build/$(basename "$SKETCH")$SUFFIX"

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
  echo "compiler.cpp.extra_flags=-fno-use-cxa-atexit$SIM_FLAG"
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
