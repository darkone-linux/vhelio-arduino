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
# Le NUMERO DU POINT est envoye a la carte, qui l'affiche sur le digit de
# droite. L'ecran s'eteint 2 s a chaque changement : c'est ce noir qui marque
# le passage d'un point au suivant. D'ou l'ordre systematique
#   numero -> attendre la fin du noir -> declencher l'evenement
# sans quoi l'evenement, qui ne dure qu'une seconde, se produirait pendant le
# noir et ne serait jamais lu.
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

DUR=260
( timeout "$DUR" cat <&3 ) > "$LOG" &
CATPID=$!
T0=$SECONDS

say() { printf '  t+%03ds  %s\n' "$((SECONDS - T0))" "$1"; }

# point <numero> <intitule> — annonce a la carte, puis laisse passer le noir.
point() {
  printf '#%s' "$1" >&3
  if [ "$1" = 0 ]; then
    say "---- hors point numerote, l'afficheur revient a 0 ----"
  else
    say "==== POINT $1 : $2 ===="
  fi
  sleep 2.3
}

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
echo "== L'ouverture du port vient de RESET la carte : c'est voulu, le point 1"
echo "== est cet autotest-la. Un televersement juste avant en a provoque un"
echo "== autre, quelques secondes plus tot : voir DEUX fois le test d'afficheur"
echo "== est donc normal, seul le second appartient a la sequence."
echo

step 4 - "DEMARRAGE : tous les segments allumes 2 s, et pendant ce temps"
say "        7 claquements de 200 ms (R1..R6 puis R7). R8 MUET."
point 1 "l'autotest que vous venez d'entendre"
step 2 - "afficheur : ---1"

if [ "$PHASE" = all ] || [ "$PHASE" = eclairage ]; then
  point 2 "veilleuse puis phares"
  step 4 v "veilleuse -> R1 et R5 collent (2 claquements). Afficheur : UE 2"
  step 4 p "phares    -> R2 colle, R1 et R5 RESTENT. Afficheur : Ph 2"
  point 3 "relacher la veilleuse, phares allumes"
  step 4 v "RIEN NE BOUGE, pas un claquement (MAIN_KEEPS_PARK)"
  point 4 "tout eteindre"
  step 4 p "R1, R2, R5 retombent : 3 claquements"
  step 2 x "repos"
fi

if [ "$PHASE" = all ] || [ "$PHASE" = clignotants ]; then
  point 5 "cadence du clignotant"
  step 25 g "R3 claque a 1,33 Hz. CHRONOMETRER 30 cycles = 22,5 s +/- 1 s. Afficheur : CLG5"
  point 6 "conflit gauche + droite"
  step 5 d "SILENCE TOTAL, les deux relaches, et R7 colle. Afficheur : Err6"
  step 3 d "droite relachee -> retour a gauche, R7 retombe"
  point 0 "detresse"
  step 6 w "R3 et R4 EN PHASE : le claquement est double. Afficheur : CL2 0"
  step 3 x "repos"
fi

if [ "$PHASE" = all ] || [ "$PHASE" = freins ]; then
  point 7 "relachement du frein en deux temps"
  step 4 a "frein avant -> R6 et R8 collent. Afficheur : Fr 7"
  step 4 a "relache -> R6 AUSSITOT, R8 300 ms plus tard : deux temps distincts"
  step 4 r "frein arriere -> meme chose par IN5"
  step 4 r "relache"
  step 2 x "repos"
fi

if [ "$PHASE" = all ] || [ "$PHASE" = voyant ]; then
  point 0 "voyant de defaut et acquittement"
  step 3 g "gauche"
  step 4 d "conflit -> R7 colle et RESTE colle (allumage fixe)"
  step 2 3 "acquittement -> R7 retombe, afficheur AC. Le defaut reste au journal"
  step 2 3 "bouton relache"
  step 4 d "conflit leve -> retour a gauche"
  step 4 d "conflit refait -> R7 SE RALLUME : l'acquittement portait sur l'evenement"
  step 3 x "repos"
fi

if [ "$PHASE" = all ] || [ "$PHASE" = rappel ]; then
  point 8 "rappel d'oubli du clignotant"
  step 50 g "gauche maintenu -> a +45 s : rythme SYNCOPE (200/550) et CLG8 devient CLO8"
  step 2 x "repos"
fi

point 0 "fin"
kill "$CATPID" 2>/dev/null
wait "$CATPID" 2>/dev/null
echo
echo "== Journal console : $LOG =="
