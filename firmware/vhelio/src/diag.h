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
  FLT_HORN_STUCK    = 1 << 1,  /* voie auxiliaire bloquée (klaxon si activé) */
  FLT_BAFANG_LINK   = 1 << 2,  /* pas de trame valide depuis 2 s            */
  FLT_BRAKE_STUCK   = 1 << 3,  /* freinage continu > 2 min                  */
  FLT_LOOP_SLOW     = 1 << 4,  /* temps de cycle > LOOP_SLOW_US             */
  FLT_WDT_RESET     = 1 << 5,  /* dernier reset provoqué par le chien de garde */
  FLT_BRAKE_NEVER   = 1 << 6   /* 2 km parcourus sans jamais voir de freinage */
};

/* mcusr : copie de MCUSR relevée AVANT son effacement dans setup(). */
void begin(uint8_t mcusr);

/* Balayage visuel des sorties d'éclairage et de signalisation.
 * Le voyant de défaut (R7) est inclus : c'est le contrôle de la LED elle-même.
 * Seule la coupure moteur (R8) est exclue. */
void selfTest();

void update(uint32_t now);

/* Voyant de défaut sur R7 : au moins un défaut du masque FAULT_LAMP_MASK est
 * actif et non acquitté. */
bool lampOn();

/* Acquittement, déclenché par le bouton sur IN3.
 *   - efface les défauts MÉMORISÉS (reset chien de garde, cycle lent), qui
 *     sinon persisteraient jusqu'à la coupure de l'alimentation ;
 *   - éteint le voyant pour les défauts encore actifs, sans les masquer sur
 *     l'afficheur ni au journal.
 * Un défaut qui disparaît puis revient rallume le voyant : l'acquittement
 * porte sur un événement, pas sur une catégorie. */
void acknowledge();

/* Durée du cycle précédent, en microsecondes. */
void noteLoop(uint32_t us);

uint8_t faults();

}  // namespace diag
