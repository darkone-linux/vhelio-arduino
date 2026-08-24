/*
 * wheelspeed.h — Vitesse depuis le capteur de roue, par scrutation.
 *
 * Source alternative au décodage Bafang (SPEED_SOURCE_WHEEL). Volontairement
 * scrutée et non interrompue : D10/D11 (SoftwareSerial) et D12 partagent le
 * même vecteur PCINT0 sur l'ATmega328P, et deux gestionnaires du même vecteur
 * ne peuvent pas coexister. À 40 km/h la roue fait 5 tours/s, la scrutation à
 * chaque cycle de boucle est largement suffisante.
 */
#pragma once

#include <Arduino.h>

namespace wheelspeed {

void begin();
void update(uint32_t now);

bool valid();
uint16_t speedKmh10();

}  // namespace wheelspeed
