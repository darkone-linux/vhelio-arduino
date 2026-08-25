#!/usr/bin/env bash
# Deroule la campagne 1 du plan de tests sur le firmware de BANC.
#
# Usage : ./tools/seq-banc.sh [port] [phase]
#         phase = all (defaut) | eclairage | clignotants | freins | voyant | rappel
#
# Prerequis :
#   VHELIO_SIM=1 ./tools/build-nix.sh
#   VHELIO_SIM=1 ./tools/upload.sh /dev/ttyACM0 old
#   carte alimentee en 12 V (sans quoi AUCUN relais ne colle)
#
# Le script envoie les touches et ANNONCE ce qui doit s'entendre, horodate.
# Il ne peut rien verifier lui-meme : la chaine de registres ne se relit pas,
# et le seul temoin des relais est leur claquement. C'est donc un metronome
# pour l'operateur, pas un test automatique.
#
# La console est journalisee dans .build/seq-banc.log : elle dit ce que le
# FIRMWARE a decide, le claquement dit ce que la CARTE a fait. Les deux
# ensemble font le test ; le journal seul ne prouve rien du materiel.
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
PORT="${1:-/dev/ttyACM0}"
PHASE="${2:-all}"
LOG="$ROOT/.build/seq-banc.log"

[ -e "$PORT" ] || { echo "Port absent : $PORT"; exit 1; }
mkdir -p "$ROOT/.build"

# L'ouverture du port abaisse DTR : le Nano redemarre et rejoue son autotest.
exec 3<>"$PORT"
stty -F "$PORT" 115200 raw -echo

DUR=210
( timeout "$DUR" cat <&3 ) > "$LOG" &
CATPID=$!
T0=$SECONDS

say() { printf '  t+%03ds  %s\n' "$((SECONDS - T0))" "$1"; }

# step <secondes> <touche|-> <ce qui doit s'entendre>
step() {
  local d="$1" key="$2" msg="$3"
  if [ "$key" != "-" ]; then
    printf '%s' "$key" >&3
    say "[$key] $msg"
  else
    say "$msg"
  fi
  sleep "$d"
}

echo "== Sequence de banc, $PORT, phase '$PHASE' — journal : $LOG =="
echo

step 4 - "AUTOTEST : 7 claquements de 200 ms (R1..R6 puis R7). R8 MUET."

if [ "$PHASE" = all ] || [ "$PHASE" = eclairage ]; then
  echo "-- T1.3 eclairage --"
  step 4 v "veilleuse   -> R1 et R5 collent (2 claquements)"
  step 4 p "phares      -> R2 colle. R1 et R5 RESTENT collés"
  step 4 v "veilleuse relachee -> RIEN NE BOUGE (MAIN_KEEPS_PARK)"
  step 4 p "phares relaches    -> R1, R2, R5 retombent (3 claquements)"
  step 2 x "repos"
fi

if [ "$PHASE" = all ] || [ "$PHASE" = clignotants ]; then
  echo "-- T1.4 clignotants --"
  step 25 g "gauche -> R3 claque a 1,33 Hz. CHRONOMETRER 30 cycles = 22,5 s +/- 1 s"
  step 5 d "gauche + droite -> SILENCE TOTAL (les deux relaches) et R7 colle"
  step 5 d "droite relachee -> retour a gauche, R7 retombe"
  step 3 x "repos"
  step 6 w "detresse -> R3 et R4 EN PHASE : le claquement est double"
  step 3 x "repos"
fi

if [ "$PHASE" = all ] || [ "$PHASE" = freins ]; then
  echo "-- T1.5 freinage --"
  step 4 a "frein avant  -> R6 (stop) et R8 (coupure) collent"
  step 4 a "relache      -> R6 retombe AUSSITOT, R8 300 ms plus tard : 2 temps"
  step 4 r "frein arriere -> meme chose par IN5"
  step 4 r "relache"
  step 2 x "repos"
fi

if [ "$PHASE" = all ] || [ "$PHASE" = voyant ]; then
  echo "-- T1.6 voyant de defaut et acquittement --"
  step 3 g "gauche"
  step 4 d "conflit -> R7 colle et RESTE colle (allumage fixe, pas de battement)"
  step 2 3 "acquittement -> R7 retombe. Le defaut reste au journal"
  step 2 3 "bouton relache"
  step 4 d "conflit leve -> retour a gauche"
  step 4 d "conflit refait -> R7 SE RALLUME : l'acquittement portait sur l'evenement"
  step 3 x "repos"
fi

if [ "$PHASE" = all ] || [ "$PHASE" = rappel ]; then
  echo "-- T1.4 rappel d'oubli --"
  step 50 g "gauche maintenu -> a t+45 s le rythme devient SYNCOPE (200/550)"
  step 2 x "repos"
fi

say "fin"
kill "$CATPID" 2>/dev/null
wait "$CATPID" 2>/dev/null
echo
echo "== Journal console : $LOG =="
