/*
 * lights.h — Phares et feux arrière.
 *
 * Sur cette carte les sorties sont des relais : pas de modulation possible.
 * Les feux de position et le feu stop occupent donc DEUX relais et deux
 * circuits distincts, comme sur un câblage automobile classique.
 */
#pragma once

#include <Arduino.h>
#include "inputs.h"

namespace lights {

void begin();
void update(uint32_t now, const InputState& in, bool braking);

bool lowBeamOn();
bool highBeamOn();
bool tailParkOn();
bool tailStopOn();

}  // namespace lights
