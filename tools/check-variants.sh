#!/usr/bin/env bash
# Compile le firmware dans plusieurs combinaisons d'options de config.h.
# Les #if ne sont pas verifies par le compilateur tant qu'une branche n'est
# pas prise : sans ce balayage, une variante peut rester cassee des mois.
# Un avertissement sur le code du projet fait echouer la variante (NF-5).
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
CFG="$ROOT/firmware/vhelio/src/config.h"
# VHELIO_BUILD permet de forcer le script de compilation (ex. en CI sur
# Ubuntu, ou build-nix.sh est executable mais `nix` est absent).
BUILD="${VHELIO_BUILD:-$ROOT/tools/build-nix.sh}"
if [ -z "${VHELIO_BUILD:-}" ]; then
  { [ -x "$BUILD" ] && command -v nix >/dev/null 2>&1; } || BUILD="$ROOT/tools/build.sh"
fi

cp "$CFG" "$CFG.bak"
# Rendre config.h ET effacer le binaire : .build/vhelio contiendrait sinon la
# DERNIERE variante compilee, que upload.sh televerserait sans rien signaler.
# Depuis que le balayage couvre SIM_INPUTS, ce serait un firmware de banc.
trap 'mv -f "$CFG.bak" "$CFG"; rm -rf "$ROOT/.build/vhelio"' EXIT

set_opt() { sed -i -E "s/^(#define[[:space:]]+$1[[:space:]]+)[^ ]+.*$/\\1$2/" "$CFG"; }

# Avertissements emis sur le code du projet. Le coeur Arduino en produit
# lui-meme avec --warnings all : on ne regarde que firmware/. "attention" est
# la forme francaise de gcc (build-nix.sh), "warning" celle de la CI.
proj_warnings() {
  grep -E "(warning|attention)[[:space:]]*:" | grep -F "$ROOT/firmware/" | sort -u
}

run_case() {
  local name="$1"; shift
  # Sans option, c'est config.h tel que versionne, donc le binaire de route :
  # il ne doit emettre AUCUN message, pas meme un #warning. Les autres
  # variantes ont droit aux #warning voulus de config.h (flash du stop, banc
  # d'essai), jamais a un avertissement du compilateur.
  local strict=0
  [ $# -eq 0 ] && strict=1
  cp "$CFG.bak" "$CFG"
  while [ $# -gt 0 ]; do set_opt "$1" "$2"; shift 2; done
  rm -rf "$ROOT/.build/vhelio"
  if out="$("$BUILD" 2>&1)"; then
    local warn
    warn="$(printf '%s\n' "$out" | proj_warnings)"
    [ "$strict" = 1 ] || warn="$(printf '%s\n' "$warn" | grep -vF -- '[-Wcpp]')"
    if [ -n "$warn" ]; then
      printf '  AVERT %-42s\n' "$name"
      echo "$warn" | head -8 | sed 's/^/          /'
      FAILED=1
      return
    fi
    printf '  OK    %-42s %s\n' "$name" \
      "$(echo "$out" | grep -oE '[0-9]+ octets \([0-9]+%\)' | head -1)"
  else
    printf '  ECHEC %-42s\n' "$name"
    echo "$out" | grep -Ei "erreur|error" | head -8 | sed 's/^/          /'
    FAILED=1
  fi
}

FAILED=0
echo "== Balayage des variantes de compilation =="
run_case "defaut"
run_case "sans bus Bafang, vitesse par capteur roue" \
  BAFANG_ENABLE 0 SPEED_SOURCE_WHEEL 1
run_case "freins en parallele + flash stop" \
  BRAKE_WIRING_VARIANT 1 BRAKE_FLASH_ENABLE 1
run_case "production silencieuse (ni journal ni autotest)" \
  DEBUG_SERIAL 0 SELFTEST_ENABLE 0 WATCHDOG_ENABLE 0 TAIL_ALWAYS_ON 1
run_case "sans afficheur" DISPLAY_ENABLE 0
run_case "minimal (ni afficheur ni bus ni journal)" \
  DISPLAY_ENABLE 0 BAFANG_ENABLE 0 DEBUG_SERIAL 0
run_case "mode apprentissage Bafang" BAFANG_LEARN_MODE 1
run_case "vitesse Bafang en valeur directe" BAFANG_SPEED_FORMULA 0
run_case "bus Bafang ET capteur roue" SPEED_SOURCE_WHEEL 1
run_case "phare conditionne a la veilleuse" \
  MAIN_REQUIRES_PARK 1 MAIN_KEEPS_PARK 0
run_case "freins inverses (interface transistor)" \
  IN_INVERT_BRAKE_FRONT 1 IN_INVERT_BRAKE_REAR 1
run_case "rappel clignotant muet + page vitesse" \
  BLINK_REMINDER_ON_MS 0 DISPLAY_DEFAULT_PAGE 1
run_case "klaxon raccorde a la carte (voie IN3/R7)" \
  HORN_ENABLE 1 FAULT_LAMP_ENABLE 0
run_case "sans voyant de defaut" FAULT_LAMP_ENABLE 0
run_case "banc d'essai : entrees simulees a la console" SIM_INPUTS 1

[ "$FAILED" = 0 ] && echo "== Toutes les variantes compilent, sans avertissement ==" || echo "== ECHECS =="
exit "$FAILED"
