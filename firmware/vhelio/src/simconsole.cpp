#include "simconsole.h"

#if SIM_INPUTS

#include <avr/pgmspace.h>

#include "pins.h"

namespace {

uint8_t g_mask = 0;    /* 1 = entrée réquisitionnée par la console  */
uint8_t g_level = 0;   /* niveau imposé aux entrées réquisitionnées */

/* Touches mnémoniques, dans l'ordre IN1..IN8. Les chiffres 1..8 font la même
 * chose : sur un clavier AZERTY ils exigent la touche majuscule ou le pavé
 * numérique, d'où les lettres. `w` pour la détresse parce que c'est le mot
 * qu'on emploie en conduisant. */
const char KEYS[IN_COUNT] PROGMEM = { 'g', 'd', 'q', 'a', 'r', 'v', 'p', 'w' };

int8_t indexOf(char c) {
  if (c >= '1' && c <= '8') return (int8_t)(c - '1');
  for (uint8_t i = 0; i < IN_COUNT; ++i) {
    if (c == (char)pgm_read_byte(&KEYS[i])) return (int8_t)i;
  }
  return -1;
}

void report() {
  Serial.print(F("[SIM] sim="));
  for (uint8_t i = 0; i < IN_COUNT; ++i) {
    Serial.print((g_mask >> i) & 1 ? '1' : '0');
  }
  Serial.print(F(" niv="));
  for (uint8_t i = 0; i < IN_COUNT; ++i) {
    Serial.print((g_level >> i) & 1 ? '1' : '0');
  }
  Serial.println();
}

void help() {
  Serial.println(F("[SIM] 1 g clign.G   2 d clign.D   3 q acquit   4 a frein AV"));
  Serial.println(F("[SIM] 5 r frein AR  6 v veilleuse 7 p phares   8 w detresse"));
  Serial.println(F("[SIM] 0 = tout relache, sous simulation | x = rendre au materiel"));
  Serial.println(F("[SIM] une touche prend la borne et la ferme ; la meme la rouvre"));
  report();
}

void handle(char c) {
  if (c >= 'A' && c <= 'Z') c += 32;

  if (c == '0') {
    /* Repos franc : les huit bornes sont tenues ouvertes par la console, donc
     * isolées de tout ce qui pourrait traîner sur le bornier. */
    g_mask = 0xFF;
    g_level = 0;
    report();
    return;
  }
  if (c == 'x') {
    g_mask = 0;
    g_level = 0;
    report();
    return;
  }
  if (c == '?' || c == 'h') {
    help();
    return;
  }

  const int8_t i = indexOf(c);
  if (i < 0) return;   /* retours chariot et frappes parasites : ignorés */

  const uint8_t b = (uint8_t)(1u << i);
  if (!(g_mask & b)) {
    /* Première frappe : la console prend la borne ET ferme le contact. C'est
     * le geste attendu — on tape `v` pour allumer la veilleuse, pas pour
     * réquisitionner une entrée et la laisser ouverte. */
    g_mask |= b;
    g_level |= b;
  } else {
    g_level ^= b;
  }
  report();
}

}  // namespace

void simconsole::begin() {
  g_mask = 0;
  g_level = 0;
  Serial.println(F("\n[SIM] *** MODE SIMULATION — SIM_INPUTS=1 ***"));
  Serial.println(F("[SIM] Les entrees peuvent etre pilotees depuis cette console."));
  Serial.println(F("[SIM] NE JAMAIS ROULER AVEC CE FIRMWARE.  '?' pour l'aide."));
}

void simconsole::poll() {
  while (Serial.available() > 0) {
    handle((char)Serial.read());
  }
}

bool simconsole::active(uint8_t idx) {
  return (g_mask >> idx) & 1;
}

bool simconsole::level(uint8_t idx) {
  return (g_level >> idx) & 1;
}

#else

void simconsole::begin() {}
void simconsole::poll() {}
bool simconsole::active(uint8_t) { return false; }
bool simconsole::level(uint8_t) { return false; }

#endif
