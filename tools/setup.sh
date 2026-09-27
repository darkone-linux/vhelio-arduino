#!/usr/bin/env bash
# Installe arduino-cli et le coeur AVR dans ./.arduino (local au projet,
# rien n'est installe au niveau systeme).
#
# Versions EPINGLEES, et archive verifiee avant d'etre deballee : deux postes
# installes a des dates differentes compilent avec les memes outils, et rien
# de telecharge ne s'execute sans avoir ete controle. La CI passe par ce meme
# script, pour la meme raison.
#
# Monter de version : changer CLI_VERSION et les quatre empreintes, lues dans
# le fichier <version>-checksums.txt de la release GitHub d'arduino-cli, puis
# AVR_CORE. Remesurer ensuite l'empreinte (AGENTS.md, « Avant de committer »).
set -euo pipefail

CLI_VERSION="1.5.1"
AVR_CORE="arduino:avr@1.8.8"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
ARDUINO_DIR="$ROOT/.arduino"
BIN_DIR="$ARDUINO_DIR/bin"

case "$(uname -s)_$(uname -m)" in
  Linux_x86_64)  ARCH=Linux_64bit; SHA256=28a8e119c498a25607821c36cb2dc49e8463941b261a0d99091baa7bc692dd2b ;;
  Linux_aarch64) ARCH=Linux_ARM64; SHA256=1e69e077479f300614d4551334e0a33f08ee40b04315d83b8e7e0e94f0d0ee62 ;;
  Darwin_x86_64) ARCH=macOS_64bit; SHA256=c982e940027996bea9901050e95fae99c59c1dcfee54beedecaf28141e7bf2e7 ;;
  Darwin_arm64)  ARCH=macOS_ARM64; SHA256=cb952e8c1621c95ef5f1d17831c945e3d0ec5973f89c557a7ec8feb9c4f7d4c9 ;;
  *) echo "Plate-forme non prevue : $(uname -sm)"; exit 1 ;;
esac

sha256_ok() {
  if command -v sha256sum >/dev/null 2>&1; then
    echo "$1  $2" | sha256sum -c --status -
  else
    echo "$1  $2" | shasum -a 256 -c -s -
  fi
}

mkdir -p "$BIN_DIR"

if ! "$BIN_DIR/arduino-cli" version 2>/dev/null | grep -q "Version: $CLI_VERSION "; then
  echo "== Telechargement d'arduino-cli $CLI_VERSION =="
  ARCHIVE="arduino-cli_${CLI_VERSION}_${ARCH}.tar.gz"
  TMP="$(mktemp -d)"
  trap 'rm -rf "$TMP"' EXIT
  curl -fsSL -o "$TMP/$ARCHIVE" \
    "https://github.com/arduino/arduino-cli/releases/download/v$CLI_VERSION/$ARCHIVE"
  sha256_ok "$SHA256" "$TMP/$ARCHIVE" \
    || { echo "Empreinte SHA-256 inattendue pour $ARCHIVE : archive refusee."; exit 1; }
  tar -xzf "$TMP/$ARCHIVE" -C "$TMP" arduino-cli
  mv -f "$TMP/arduino-cli" "$BIN_DIR/arduino-cli"
  chmod 755 "$BIN_DIR/arduino-cli"
fi

export ARDUINO_DIRECTORIES_DATA="$ARDUINO_DIR/data"
export ARDUINO_DIRECTORIES_DOWNLOADS="$ARDUINO_DIR/downloads"
export ARDUINO_DIRECTORIES_USER="$ARDUINO_DIR/user"

echo "== Installation du coeur $AVR_CORE =="
"$BIN_DIR/arduino-cli" core update-index
"$BIN_DIR/arduino-cli" core install "$AVR_CORE"

echo "== Pret. Lancer ./tools/build.sh (./tools/build-nix.sh sous NixOS) =="
