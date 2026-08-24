#!/usr/bin/env bash
# Compile le firmware dans plusieurs combinaisons d'options de config.h.
# Les #if ne sont pas verifies par le compilateur tant qu'une branche n'est
# pas prise : sans ce balayage, une variante peut rester cassee des mois.
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CFG="$ROOT/firmware/vhelio/src/config.h"
BUILD="$ROOT/tools/build-nix.sh"
[ -x "$BUILD" ] || BUILD="$ROOT/tools/build.sh"

cp "$CFG" "$CFG.bak"
trap 'mv -f "$CFG.bak" "$CFG"' EXIT

set_opt() { sed -i -E "s/^(#define[[:space:]]+$1[[:space:]]+)[^ ]+.*$/\\1$2/" "$CFG"; }

run_case() {
  local name="$1"; shift
  cp "$CFG.bak" "$CFG"
  while [ $# -gt 0 ]; do set_opt "$1" "$2"; shift 2; done
  rm -rf "$ROOT/.build/vhelio"
  if out="$("$BUILD" 2>&1)"; then
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
  BLINK_REMINDER_ON_MS 0 DISPLAY_DEFAULT_PAGE 0
run_case "klaxon raccorde a la carte (voie auxiliaire)" HORN_ENABLE 1

[ "$FAILED" = 0 ] && echo "== Toutes les variantes compilent ==" || echo "== ECHECS =="
exit "$FAILED"
