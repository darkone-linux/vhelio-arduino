/*
 * lights.h — Éclairage avant et feux arrière.
 *
 * Sur cette carte les sorties sont des relais : pas de modulation possible.
 * Les feux de position et le feu stop occupent donc DEUX relais et deux
 * circuits distincts, comme sur un câblage automobile classique.
 *
 * Deux niveaux d'éclairage avant, commandés séparément :
 *   IN_PARK  — interrupteur dédié « veilleuse »   -> OUT_PARK_FRONT (R1)
 *   IN_MAIN  — interrupteur du comodo, plein feu  -> OUT_MAIN       (R2)
 *
 * Le feu de position arrière suit l'un OU l'autre, sans condition.
 */
#pragma once

#include <Arduino.h>
#include "inputs.h"

namespace lights {

void begin();
void update(uint32_t now, const InputState& in, bool braking);

bool parkOn();      /* veilleuse avant, R1  */
bool mainOn();      /* phares, R2           */
bool tailParkOn();  /* position arrière, R5 */
bool tailStopOn();  /* stop arrière, R6     */

}  // namespace lights
