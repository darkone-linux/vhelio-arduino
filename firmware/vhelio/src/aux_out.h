/*
 * aux_out.h — Arbitrage de OUT8.
 *
 * Selon OUT8_ROLE_BUZZER : buzzer de retour clignotants, ou relais
 * d'alimentation des prises accessoires. Les deux ne tiennent pas sur une
 * seule sortie ; le choix est fait à la compilation.
 */
#pragma once

#include <Arduino.h>

namespace aux_out {

void begin();
void update(uint32_t now);

}  // namespace aux_out
