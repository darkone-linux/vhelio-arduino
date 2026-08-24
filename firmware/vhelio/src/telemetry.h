/*
 * telemetry.h — Vitesse consolidée et odomètre.
 *
 * Fonction de CONFORT uniquement (principe P2). Rien de ce qui est publié ici
 * n'entre dans une décision de sécurité : la vitesse ne sert qu'au rappel
 * d'oubli des clignotants et au journal.
 */
#pragma once

#include <Arduino.h>

namespace telemetry {

void begin();
void update(uint32_t now);

bool speedValid();
uint16_t speedKmh10();

/* Odomètre depuis la mise sous tension, en millimètres. Repli à 49,7 jours
 * sans conséquence : seules des différences sont comparées. */
uint32_t odoMm();

}  // namespace telemetry
