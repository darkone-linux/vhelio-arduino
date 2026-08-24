/*
 * turnsignals.h — Clignotants, feux de détresse, rappel d'oubli.
 */
#pragma once

#include <Arduino.h>
#include "inputs.h"

namespace turnsignals {

enum Mode : uint8_t { OFF = 0, LEFT = 1, RIGHT = 2, HAZARD = 3 };

void begin();
void update(uint32_t now, const InputState& in);

Mode mode();

/* Gauche et droite demandés simultanément : défaut de comodo ou de câblage.
 * On éteint tout — mieux vaut ne rien signaler que signaler deux directions. */
bool conflict();

/* Le buzzer doit-il sonner à cet instant ? Consommé par aux_out, qui est le
 * seul propriétaire de OUT_AUX. */
bool buzzerRequest(uint32_t now);

/* Clignotant directionnel actif depuis trop longtemps ou trop de distance. */
bool reminderActive();

}  // namespace turnsignals
