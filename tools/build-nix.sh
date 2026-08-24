#!/usr/bin/env bash
# Compilation sur NixOS.
#
# Les binaires prebuilt fournis par arduino-cli (avr-gcc, ctags) sont lies
# dynamiquement contre un /lib inexistant sur NixOS. Ce script fournit les
# equivalents nixpkgs et lance la compilation dedans.
#
# Usage : ./tools/build-nix.sh [old]     (old = Nano a ancien bootloader)
#
# VHELIO_SKETCH=tools/pinscan ./tools/build-nix.sh   compile le croquis de
# verification de brochage au lieu du firmware.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

exec nix shell \
  nixpkgs#pkgsCross.avr.buildPackages.gcc \
  nixpkgs#pkgsCross.avr.buildPackages.binutils \
  nixpkgs#ctags \
  --command bash "$ROOT/tools/nix-inner.sh" "${1:-}"
