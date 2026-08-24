/*
 * lights.h — Phares et arbitrage du feu arrière.
 *
 * OUT_TAIL porte deux fonctions (veilleuse et stop) sur une seule sortie PWM.
 * Ce module en est le SEUL propriétaire : brakes lui demande le stop, il ne
 * l'écrit pas lui-même. Une sortie, un propriétaire.
 */
#pragma once

#include <Arduino.h>
#include "inputs.h"

namespace lights {

void begin();
void update(uint32_t now, const InputState& in, bool braking);

bool lowBeamOn();
bool highBeamOn();
uint8_t tailDuty();

}  // namespace lights
