/*
 * board_io.h — Couche d'abstraction matérielle de la carte DN22D08.
 *
 * Absorbe trois particularités du montage pour qu'aucun module métier n'ait à
 * s'en soucier : la polarité des optocoupleurs, l'absence de lecture numérique
 * sur A6/A7, et la disponibilité du PWM selon la broche.
 */
#pragma once

#include <Arduino.h>
#include "pins.h"

namespace board {

void begin();

/* Lit une entrée et renvoie son état LOGIQUE (true = signal présent sur la
 * borne), quelle que soit la polarité électrique de l'optocoupleur.
 * Aucun anti-rebond ici : c'est le rôle du module inputs. */
bool readInputRaw(uint8_t idx);

/* Écrit une sortie en tout-ou-rien. */
void setOutput(uint8_t idx, bool on);

/* Écrit une sortie en PWM (0..255 = 0..100 % d'activité LOGIQUE).
 * Si la broche n'a pas de PWM matériel, retombe sur un tout-ou-rien à
 * seuil 128 plutôt que de produire un comportement silencieusement faux. */
void setOutputPwm(uint8_t idx, uint8_t duty);

/* Dernier rapport cyclique logique appliqué (255 = pleine puissance). */
uint8_t outputDuty(uint8_t idx);

/* Toutes les sorties inactives. C'est l'état sûr (specs/07-securite.md §3). */
void allOff();

/* Vrai si la broche affectée à cette sortie sait faire du PWM matériel.
 * Utilisé par l'autotest pour signaler une réaffectation malheureuse. */
bool outputHasPwm(uint8_t idx);

}  // namespace board
