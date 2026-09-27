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
#
# VHELIO_SIM=1 ./tools/build-nix.sh   compile le firmware de BANC : les huit
# entrees sont pilotables au clavier depuis la console (specs/08 campagne 1).
# Le binaire va dans .build/vhelio-sim, jamais melange a celui de route.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"

# nixpkgs EPINGLE. Sans revision, `nixpkgs#` suit le systeme ou le canal :
# l'avr-gcc changerait a chaque mise a jour, et avec lui le binaire,
# l'empreinte et le temps de cycle mesures. Cette revision fournit avr-gcc
# 15.3.0, binutils 2.46 et ctags 816 : ceux de l'empreinte de specs/06 §7.
# Monter de revision, c'est remesurer (AGENTS.md, « Avant de committer »).
# VHELIO_NIXPKGS=nixpkgs revient au nixpkgs du systeme, pour essayer.
NIXPKGS="${VHELIO_NIXPKGS:-github:NixOS/nixpkgs/e158d9ed9b51c98974c5e66e1ba1c9e0255fecaa}"

exec nix shell \
  "$NIXPKGS#pkgsCross.avr.buildPackages.gcc" \
  "$NIXPKGS#pkgsCross.avr.buildPackages.binutils" \
  "$NIXPKGS#ctags" \
  --command bash "$ROOT/tools/nix-inner.sh" "${1:-}"
