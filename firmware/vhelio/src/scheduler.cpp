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

#if WATCHDOG_ENABLE
namespace {

/* Marqueur de reset par chien de garde. Section .noinit : le démarrage ne
 * l'initialise ni ne l'efface, il survit donc au reset.
 *
 * MCUSR ne peut pas servir à cela. Le bootloader de la carte est Optiboot 4.4
 * (avrdude lit « HW 3 / FW 4.4 », specs/03 §6), qui efface MCUSR avant de
 * lancer le croquis : setup() y relirait toujours 0.
 *
 * Le chien est donc armé en mode « interruption puis reset » : au premier
 * débordement, WDT_vect pose le marqueur ; au second, le matériel redémarre
 * la carte. Deux périodes de 500 ms : le reset tombe 1 s après le blocage,
 * comme avec l'ancien réglage. Un blocage interruptions masquées fait
 * exception : l'ISR ne s'exécute pas, le reset a lieu quand même, mais sans
 * marqueur. Au démarrage à froid, la RAM contient n'importe quoi : une
 * valeur de 32 bits rend la confusion improbable. */
const uint32_t WDT_MARK = 0x57445452UL;   /* « WDTR » */
volatile uint32_t g_wdtMark __attribute__((section(".noinit")));

}  // namespace

ISR(WDT_vect) {
  /* Le matériel vient d'effacer WDIE : le prochain débordement sera un
   * reset. Ne surtout pas réarmer WDIE ici, cela repousserait le reset
   * indéfiniment. */
  g_wdtMark = WDT_MARK;
}
#endif

void setup() {
  /* Relever puis effacer MCUSR, et désarmer le chien de garde AVANT tout le
   * reste. Optiboot le fait déjà ; d'autres bootloaders, ou une carte
   * programmée sans bootloader, laisseraient le chien armé après un reset par
   * WDT, avec un délai trop court pour finir le démarrage : la carte
   * repartirait en boucle de reset. */
  const uint8_t mcusr = MCUSR;
  MCUSR = 0;
  wdt_disable();

#if WATCHDOG_ENABLE
  const bool wdtReset = (g_wdtMark == WDT_MARK);
  g_wdtMark = 0;
#else
  const bool wdtReset = false;
#endif

  board::begin();
  board::allOff();          /* état sûr avant toute autre initialisation */

#if DEBUG_SERIAL
  Serial.begin(DEBUG_BAUD);
#endif

  diag::begin(mcusr, wdtReset);
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

  /* Bloquant 2 s, chien de garde non armé. Pas après un reset par chien de
   * garde : le véhicule roule peut-être, de nuit, et l'autotest priverait de
   * feux et de stop pendant 2 s tout en faisant claquer R3 puis R4 — un faux
   * signal de direction pour les autres usagers. On rend la main tout de
   * suite ; le contrôle des lampes attendra la prochaine mise sous tension. */
  if (!wdtReset) diag::selfTest();

#if WATCHDOG_ENABLE
  /* wdt_enable() règle la période et arme le reset ; WDIE s'y ajoute ensuite,
   * sans séquence temporisée. Voir WDT_vect. */
  wdt_enable(WDTO_500MS);
  WDTCSR |= _BV(WDIE);
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
