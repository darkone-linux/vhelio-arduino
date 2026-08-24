#include "bafang.h"
#include "config.h"
#include "pins.h"

#if BAFANG_ENABLE

#include <SoftwareSerial.h>

namespace {

/* PIN_BAFANG_TX est déclarée mais NON CÂBLÉE : SoftwareSerial exige une
 * broche TX, on lui en donne une qui ne va nulle part. */
SoftwareSerial g_link(PIN_BAFANG_RX, PIN_BAFANG_TX);

const uint8_t MAX_FRAME = 12;
uint8_t g_buf[MAX_FRAME];
uint8_t g_len = 0;
bool g_overflow = false;

uint32_t g_lastByteMs = 0;
uint32_t g_lastValidMs = 0;
uint32_t g_nowMs = 0;

uint16_t g_framesOk = 0;
uint16_t g_framesRejected = 0;

uint16_t g_speedKmh10 = 0;
bool g_speedValid = false;
uint8_t g_soc = 0;
uint16_t g_currentA10 = 0;

/* Somme de tous les octets sauf le dernier, modulo 256, comparée au dernier. */
bool checksumOk(const uint8_t* f, uint8_t n) {
  if (n < 2) return false;
  uint8_t sum = 0;
  for (uint8_t i = 0; i < n - 1; ++i) sum += f[i];
  return sum == f[n - 1];
}

void decodeSpeed(uint16_t raw) {
#if BAFANG_SPEED_FORMULA == 0
  /* Valeur directe en dixièmes de km/h. */
  if (raw > 999) { g_speedValid = false; return; }
  g_speedKmh10 = raw;
  g_speedValid = true;
#else
  /* Période de rotation de roue en millisecondes.
   * v[km/h] x10 = 36 x circonference[mm] / periode[ms]
   * Contrôle : 2200 mm, 317 ms  ->  36*2200/317 = 250  ->  25,0 km/h. */
  if (raw == 0) { g_speedValid = false; return; }
  if (raw >= 5000) {           /* roue quasi immobile */
    g_speedKmh10 = 0;
    g_speedValid = true;
    return;
  }
  if (raw < 50) { g_speedValid = false; return; }  /* > 158 km/h : aberrant */
  const uint32_t v = (36UL * (uint32_t)WHEEL_CIRCUMFERENCE_MM) / raw;
  if (v > 999) { g_speedValid = false; return; }
  g_speedKmh10 = (uint16_t)v;
  g_speedValid = true;
#endif
}

/* Table de registres — À VALIDER en mode apprentissage sur votre matériel.
 * specs/05-protocole-bafang.md §4. */
void decodeFrame(const uint8_t* f, uint8_t n) {
  switch (f[0]) {
    case 0x20:                      /* niveau de charge */
      if (n == 3 && f[1] <= 100) g_soc = f[1];
      break;
    case 0x0A:                      /* courant, unité 0,5 A */
      if (n == 3) g_currentA10 = (uint16_t)f[1] * 5;
      break;
    case 0x11:                      /* vitesse */
      if (n == 4) decodeSpeed(((uint16_t)f[1] << 8) | f[2]);
      break;
    default:
      break;                        /* registre non exploité */
  }
}

#if BAFANG_LEARN_MODE
void dumpFrame(const uint8_t* f, uint8_t n) {
  Serial.print(F("[BAFANG] "));
  for (uint8_t i = 0; i < n; ++i) {
    if (f[i] < 0x10) Serial.print('0');
    Serial.print(f[i], HEX);
    Serial.print(' ');
  }
  Serial.print('(');
  Serial.print(n);
  Serial.println(F("o)"));
}
#endif

void closeFrame() {
  if (g_len == 0) return;

  if (!g_overflow && checksumOk(g_buf, g_len)) {
    ++g_framesOk;
    g_lastValidMs = g_nowMs;
#if BAFANG_LEARN_MODE
    dumpFrame(g_buf, g_len);
#endif
    decodeFrame(g_buf, g_len);
  } else {
    /* Un taux de rejet élevé signale un mauvais piquage, une masse absente
     * ou une mauvaise vitesse de transmission — pas un bug de décodage. */
    ++g_framesRejected;
  }
  g_len = 0;
  g_overflow = false;
}

}  // namespace

void bafang::begin() {
  g_link.begin(BAFANG_BAUD);
  g_link.listen();
}

void bafang::poll(uint32_t now) {
  g_nowMs = now;

  while (g_link.available() > 0) {
    const int c = g_link.read();
    if (c < 0) break;
    /* Un octet arrivé après le silence séparateur ouvre une nouvelle trame. */
    if (g_len > 0 && (now - g_lastByteMs) >= BAFANG_FRAME_GAP_MS) {
      closeFrame();
    }
    if (g_len < MAX_FRAME) {
      g_buf[g_len++] = (uint8_t)c;
    } else {
      g_overflow = true;
    }
    g_lastByteMs = now;
  }

  /* Fin de trame par silence, hors réception. */
  if (g_len > 0 && (now - g_lastByteMs) >= BAFANG_FRAME_GAP_MS) {
    closeFrame();
  }

  if (!linkUp()) {
    g_speedValid = false;
  }
}

bool bafang::linkUp() {
  return g_framesOk > 0 && (g_nowMs - g_lastValidMs) < BAFANG_LINK_TIMEOUT_MS;
}

bool bafang::speedValid() { return g_speedValid && linkUp(); }
uint16_t bafang::speedKmh10() { return g_speedValid ? g_speedKmh10 : 0; }
uint8_t bafang::socPct() { return g_soc; }
uint16_t bafang::currentA10() { return g_currentA10; }
uint16_t bafang::framesOk() { return g_framesOk; }
uint16_t bafang::framesRejected() { return g_framesRejected; }

#else  /* BAFANG_ENABLE == 0 : bouchons, pour que les appelants compilent */

void bafang::begin() {}
void bafang::poll(uint32_t) {}
bool bafang::linkUp() { return false; }
bool bafang::speedValid() { return false; }
uint16_t bafang::speedKmh10() { return 0; }
uint8_t bafang::socPct() { return 0; }
uint16_t bafang::currentA10() { return 0; }
uint16_t bafang::framesOk() { return 0; }
uint16_t bafang::framesRejected() { return 0; }

#endif
