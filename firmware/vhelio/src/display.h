/*
 * display.h — Afficheur 4 digits de la carte.
 *
 * Découpage volontaire : ce module possède les DIGITS, diag possède le
 * DEUX-POINTS (qui sert de battement de cœur et d'indicateur de défaut).
 * Deux informations indépendantes, deux propriétaires.
 */
#pragma once

#include <Arduino.h>

namespace display {

enum Page : uint8_t { PAGE_SPEED = 0, PAGE_SOC, PAGE_FAULTS, PAGE_ODO, PAGE_COUNT };

void begin();
void update(uint32_t now);
uint8_t page();

}  // namespace display
