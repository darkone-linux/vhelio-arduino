#!/usr/bin/env bash
# Assemble le paquet d'une version : firmwares de route et de banc + journal.
#
# Usage : ./tools/package.sh [old]     (old = Nano a ancien bootloader)
#
# Le numero de version vient de firmware/vhelio/src/config.h
# (VHELIO_FW_VERSION), jamais de la ligne de commande. Le paquet produit est
# dist/vhelio-X.Y.Z.zip ; c'est lui que la CI publie sur la Release GitHub
# correspondant a l'etiquette vX.Y.Z.
#
# Sur NixOS, passer par VHELIO_BUILD=./tools/build-nix.sh si besoin ; par
# defaut on choisit build-nix.sh quand `nix` est disponible, build.sh sinon.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
CFG="$ROOT/firmware/vhelio/src/config.h"

VERSION="$(sed -nE 's/^#define[[:space:]]+VHELIO_FW_VERSION[[:space:]]+"([^"]+)".*/\1/p' "$CFG")"
[ -n "$VERSION" ] || { echo "VHELIO_FW_VERSION introuvable dans $CFG"; exit 1; }

BUILD="${VHELIO_BUILD:-}"
if [ -z "$BUILD" ]; then
  if command -v nix >/dev/null 2>&1; then BUILD="$ROOT/tools/build-nix.sh";
  else BUILD="$ROOT/tools/build.sh"; fi
fi

BOOT_ARG="${1:-}"
DIST="$ROOT/dist/vhelio-$VERSION"

echo "== Paquet vhelio-$VERSION (via $BUILD) =="

# Firmware de route : le seul qui doit rouler.
"$BUILD" $BOOT_ARG
# Firmware de banc : entrees pilotables a la console (campagne 1).
VHELIO_SIM=1 "$BUILD" $BOOT_ARG

route_hex="$ROOT/.build/vhelio/vhelio.ino.hex"
route_elf="$ROOT/.build/vhelio/vhelio.ino.elf"
banc_hex="$ROOT/.build/vhelio-sim/vhelio.ino.hex"
for f in "$route_hex" "$banc_hex"; do
  [ -f "$f" ] || { echo "Binaire manquant : $f"; exit 1; }
done

rm -rf "$DIST"
mkdir -p "$DIST"
cp "$route_hex" "$DIST/vhelio-route-$VERSION.hex"
[ -f "$route_elf" ] && cp "$route_elf" "$DIST/vhelio-route-$VERSION.elf"
cp "$banc_hex" "$DIST/vhelio-banc-$VERSION.hex"
cp "$ROOT/CHANGELOG.md" "$ROOT/README.md" "$DIST/"

{
  echo "vhelio $VERSION"
  echo "date : $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "git : $(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo "?")"
  echo "fqbn : arduino:avr:nano:cpu=atmega328${BOOT_ARG:+$BOOT_ARG}"
  echo ""
  echo "vhelio-route-$VERSION.hex : firmware de ROUTE (a televerser)."
  echo "vhelio-banc-$VERSION.hex  : firmware de BANC (VHELIO_SIM=1, ne doit pas rouler)."
} > "$DIST/LISEZ-MOI.txt"

rm -f "$ROOT/dist/vhelio-$VERSION.zip"
(cd "$ROOT/dist" && zip -qr "vhelio-$VERSION.zip" "vhelio-$VERSION")

echo "== Paquet pret : dist/vhelio-$VERSION.zip =="
unzip -l "$ROOT/dist/vhelio-$VERSION.zip"
