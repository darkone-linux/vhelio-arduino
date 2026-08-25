/*
 * scheduler.cpp — Boucle principale du calculateur d'éclairage et de signalisation du Vhélio.
 *
 * Arduino Nano + carte d'E/S rail DIN DN22D08.
 * Spécification complète dans ../../../specs/.
 *
 * setup() et loop() vivent ici, et non dans vhelio.ino : l'IDE Arduino
 * génère automatiquement les prototypes des fonctions trouvées dans un .ino,
 * en s'appuyant sur un ctags patché. Hors de cette chaîne exacte, la
 * génération est fausse ou muette. Dans un .cpp, aucun prétraitement n'a
 * lieu : le code compilé est exactement celui qui est écrit.
 *
 * Modèle d'exécution : ordonnanceur coopératif à balayage complet. À chaque
 * tour, tous les modules sont réévalués à partir du même instant `now`.
 * Aucun delay(), aucune boucle d'attente en dehors de l'autotest de setup().
 *
 * Rappel des quatre principes (specs/00-vue-densemble.md §3) :
 *   P1  le firmware n'est jamais un point de défaillance unique
 *   P2  la télémétrie est un confort, pas une fonction de sécurité
 *   P3  état sûr par défaut
 *   P4  aucune boucle bloquante
 */

#include <Arduino.h>
#include <avr/wdt.h>

#include "bafang.h"
#include "board_io.h"
#include "brakes.h"
#include "config.h"
#include "diag.h"
#include "display.h"
#include "horn.h"
#include "inputs.h"
#include "lights.h"
#include "pins.h"
#include "simconsole.h"
#include "telemetry.h"
#include "turnsignals.h"
#include "wheelspeed.h"

void setup() {
  /* Relever puis effacer MCUSR, et désarmer le chien de garde AVANT tout le
   * reste. Sans cela, après un reset par WDT, le chien reste armé avec un
   * délai trop court pour que le bootloader finisse : la carte repart en
   * boucle de reset. */
  const uint8_t mcusr = MCUSR;
  MCUSR = 0;
  wdt_disable();

  board::begin();
  board::allOff();          /* état sûr avant toute autre initialisation */

#if DEBUG_SERIAL
  Serial.begin(DEBUG_BAUD);
#endif

  diag::begin(mcusr);
  simconsole::begin();      /* banc d'essai ; ne compile rien si SIM_INPUTS=0 */
  inputs::begin();
  brakes::begin();
  turnsignals::begin();
  lights::begin();
  horn::begin();
  display::begin();
  telemetry::begin();
  wheelspeed::begin();
  bafang::begin();

  diag::selfTest();         /* bloquant ~0,9 s, chien de garde non armé */

#if WATCHDOG_ENABLE
  wdt_enable(WDTO_1S);
#endif
}

void loop() {
  const uint32_t t0 = micros();
  const uint32_t now = millis();

  /* Acquisition ------------------------------------------------------- */
  simconsole::poll();       /* avant inputs : les touches valent des bornes */
  bafang::poll(now);
  wheelspeed::update(now);
  inputs::update(now);
  const InputState& in = inputs::state();

  /* Décision et commande ---------------------------------------------- *
   * brakes AVANT lights : c'est la seule dépendance d'ordre du système,
   * lights consomme brakes::braking() pour piloter le relais de stop.    */
  brakes::update(now, in);
  turnsignals::update(now, in);
  lights::update(now, in, brakes::braking());
  horn::update(now, in);

  /* Observation -------------------------------------------------------- */
  telemetry::update(now);
  display::update(now);
  diag::update(now);

  /* Émission d'un digit et de l'octet des relais sur la chaîne de registres.
   * C'est ce seul appel qui APPLIQUE réellement l'état des relais décidé
   * plus haut : tant qu'il n'a pas eu lieu, rien n'a bougé côté matériel.
   * Quatre tours de boucle forment une trame d'affichage complète. */
  board::refresh();

  diag::noteLoop(micros() - t0);

#if WATCHDOG_ENABLE
  /* Un seul acquittement, en fin de boucle. Jamais dans une boucle interne :
   * cela masquerait précisément le blocage qu'on cherche à détecter. */
  wdt_reset();
#endif
}
