/*
 * bafang.h — Écoute PASSIVE de la liaison UART afficheur <-> contrôleur.
 *
 * Ce module n'émet JAMAIS. La broche TX déclarée à SoftwareSerial reste non
 * câblée : l'émission est rendue physiquement impossible, pas seulement
 * évitée par le code.
 *
 * Principe P2 : la perte totale de ce lien n'a aucun effet sur les feux, les
 * clignotants, le stop ou le klaxon. Le module peut être retiré à la
 * compilation (BAFANG_ENABLE 0) sans rien casser d'autre.
 *
 * Le décodage repose sur une table de registres non documentée par le
 * constructeur : voir specs/05-protocole-bafang.md, et le mode apprentissage
 * pour la calibrer sur votre matériel.
 */
#pragma once

#include <Arduino.h>

namespace bafang {

void begin();
void poll(uint32_t now);

bool linkUp();
bool speedValid();
uint16_t speedKmh10();   /* km/h x10 */
uint8_t socPct();        /* % de charge, 0 si inconnu */
uint16_t currentA10();   /* ampères x10, 0 si inconnu */

uint16_t framesOk();
uint16_t framesRejected();

}  // namespace bafang
