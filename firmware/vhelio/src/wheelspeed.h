/*
 * wheelspeed.h — Vitesse depuis le capteur de roue, par scrutation.
 *
 * Source alternative au décodage Bafang (SPEED_SOURCE_WHEEL). Volontairement
 * scrutée et non interrompue : la seule broche encore libre est A6, qui n'a ni
 * tampon numérique ni interruption sur changement d'état. À 40 km/h la roue
 * fait 5 tours/s ; une scrutation à chaque cycle de boucle est largement
 * suffisante. Prévoir une résistance de tirage externe de 10 kOhm vers +5 V.
 */
#pragma once

#include <Arduino.h>

namespace wheelspeed {

void begin();
void update(uint32_t now);

bool valid();
uint16_t speedKmh10();

}  // namespace wheelspeed
