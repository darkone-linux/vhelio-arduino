/*
 * diag.h — Autotest, LED de vie, drapeaux de défaut, journal série.
 *
 * diag va CHERCHER les défauts auprès des modules plutôt que d'être notifié :
 * aucun module métier n'a besoin de connaître diag, ce qui garde le graphe de
 * dépendances acyclique.
 */
#pragma once

#include <Arduino.h>

namespace diag {

enum Fault : uint8_t {
  FLT_TURN_CONFLICT = 1 << 0,  /* gauche et droite demandés en même temps   */
  FLT_HORN_STUCK    = 1 << 1,  /* klaxon maintenu au-delà de la limite      */
  FLT_BAFANG_LINK   = 1 << 2,  /* pas de trame valide depuis 2 s            */
  FLT_BRAKE_STUCK   = 1 << 3,  /* freinage continu > 2 min                  */
  FLT_LOOP_SLOW     = 1 << 4,  /* temps de cycle > LOOP_SLOW_US             */
  FLT_WDT_RESET     = 1 << 5,  /* dernier reset provoqué par le chien de garde */
  FLT_BRAKE_NEVER   = 1 << 6   /* 2 km parcourus sans jamais voir de freinage */
};

/* mcusr : copie de MCUSR relevée AVANT son effacement dans setup(). */
void begin(uint8_t mcusr);

/* Balayage visuel des sorties d'éclairage et de signalisation.
 * Le klaxon et la coupure moteur en sont volontairement exclus. */
void selfTest();

void update(uint32_t now);

/* Durée du cycle précédent, en microsecondes. */
void noteLoop(uint32_t us);

uint8_t faults();

}  // namespace diag
