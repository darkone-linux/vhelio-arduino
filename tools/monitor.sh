#!/usr/bin/env bash
# Console serie, sans arduino-cli.
#
# Usage : ./tools/monitor.sh [port] [bauds]
#
# "arduino-cli monitor" ne marche pas sur NixOS : il reclame l'outil prebuilt
# builtin:serial-monitor, lie dynamiquement contre un /lib inexistant, et
# echoue sur "No monitor available for the port protocol serial" apres l'avoir
# telecharge. Lance hors des variables ARDUINO_DIRECTORIES_*, il pollue en
# prime ~/.arduino15 au lieu du ./.arduino du depot. stty et cat suffisent.
#
# 115200 bauds, c'est le debit des CROQUIS (Serial.begin). Le 57600 de
# upload.sh est celui de l'ancien bootloader, et ne vaut QUE pendant le
# televersement. Les confondre donne une console illisible.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"

PORT="${1:-}"
if [ -z "$PORT" ]; then
  for p in /dev/ttyACM0 /dev/ttyUSB0; do
    [ -e "$p" ] && { PORT="$p"; break; }
  done
fi
[ -n "$PORT" ] || { echo "Aucun port serie trouve. Usage : monitor.sh <port> [bauds]"; exit 1; }

SPEED="${2:-115200}"

if [ ! -r "$PORT" ] || [ ! -w "$PORT" ]; then
  echo "Pas d'acces en lecture/ecriture sur $PORT :"
  ls -l "$PORT"
  echo
  echo "Ajouter l'utilisateur au groupe proprietaire du port. Sur NixOS :"
  echo "    users.users.$USER.extraGroups = [ \"dialout\" ];"
  echo "puis nixos-rebuild switch et REOUVRIR LA SESSION."
  exit 1
fi

# Deux lecteurs sur le meme port ne s'excluent pas : ils se PARTAGENT les
# octets, un caractere sur deux chacun. L'affichage devient illisible sans
# qu'aucune des deux consoles ne signale quoi que ce soit.
if command -v fuser >/dev/null 2>&1 && fuser -s "$PORT" 2>/dev/null; then
  echo "$PORT est deja ouvert par un autre programme :"
  fuser -v "$PORT" 2>&1 || true
  echo
  echo "Fermer l'autre console (Ctrl-C) : deux lecteurs se partagent les"
  echo "octets et les deux affichages deviennent illisibles."
  exit 1
fi

echo "== $PORT a $SPEED bauds. Ctrl-C pour quitter. =="

# Garder le descripteur ouvert pendant le stty : sinon la fermeture du port
# entre les deux commandes peut rendre ses reglages a leur valeur par defaut.
# L'ouverture abaisse DTR, donc redemarre le Nano : le croquis repart de zero
# et sa banniere s'affiche.
exec 3<"$PORT"
stty -F "$PORT" "$SPEED" raw -echo
cat <&3
