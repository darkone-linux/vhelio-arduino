/*
 * display.h — Afficheur 4 digits de la carte.
 *
 * Découpage volontaire : ce module possède les DIGITS, diag possède le point
 * décimal de gauche (battement de cœur). Deux informations indépendantes,
 * deux propriétaires.
 *
 * La page par défaut n'affiche pas une grandeur mais un ÉVÉNEMENT : ce qui
 * vient de se produire, en clair. Un code hexadécimal (`F004`) suppose qu'on
 * ait la documentation sous les yeux ; `Fr`, `Ph`, `Err` se lisent sans rien.
 * Les grandeurs restent accessibles au bouton K1, et la page défauts garde le
 * code — « Err » dit qu'il y a un défaut, elle seule dit lequel.
 */
#pragma once

#include <Arduino.h>

namespace display {

enum Page : uint8_t {
  PAGE_EVENTS = 0,   /* Fr, Ph, UE, Err, CLL, CLr, CL2, ---  */
  PAGE_SPEED,
  PAGE_SOC,
  PAGE_FAULTS,       /* F0xx : QUEL défaut                   */
  PAGE_ODO,
  PAGE_COUNT
};

void begin();
void update(uint32_t now);
uint8_t page();

/* Numéro du point de contrôle affiché sur le digit de droite.
 *   0     exploitation normale
 *   1..8  point en cours de la séquence de banc (tools/seq-banc.sh)
 * Un changement de numéro éteint l'afficheur pendant DISPLAY_BLANK_MS : c'est
 * ce noir qui marque le passage d'un point au suivant. */
void setPoint(uint8_t n);

/* Vrai pendant ce noir. diag s'en sert pour éteindre aussi le battement de
 * cœur : un point qui continue de clignoter sur un écran par ailleurs noir
 * affaiblit le seul marqueur dont dispose l'opérateur. */
bool blanking();

}  // namespace display
