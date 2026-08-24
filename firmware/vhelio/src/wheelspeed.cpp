#include "wheelspeed.h"
#include "config.h"
#include "pins.h"

#if SPEED_SOURCE_WHEEL

namespace {

bool g_last = true;          /* capteur à collecteur ouvert : repos = haut */
uint32_t g_lastPulse = 0;
uint16_t g_periodMs = 0;
bool g_valid = false;

}  // namespace

void wheelspeed::begin() {
  pinMode(PIN_WHEEL, INPUT_PULLUP);
  g_last = (digitalRead(PIN_WHEEL) == HIGH);
  g_valid = false;
}

void wheelspeed::update(uint32_t now) {
  const bool level = (digitalRead(PIN_WHEEL) == HIGH);

  if (g_last && !level) {                       /* front descendant */
    const uint32_t gap = now - g_lastPulse;
    if (gap >= WHEEL_MIN_PULSE_GAP_MS) {        /* filtre les rebonds */
      if (g_lastPulse != 0 && gap < WHEEL_TIMEOUT_MS) {
        g_periodMs = (uint16_t)gap;
        g_valid = true;
      }
      g_lastPulse = now;
    }
  }
  g_last = level;

  if (g_lastPulse != 0 && (now - g_lastPulse) >= WHEEL_TIMEOUT_MS) {
    g_periodMs = 0;          /* roue arrêtée : vitesse nulle, mais valide */
    g_valid = true;
  }
}

bool wheelspeed::valid() { return g_valid; }

uint16_t wheelspeed::speedKmh10() {
  if (!g_valid || g_periodMs == 0) return 0;
  const uint32_t v = (36UL * (uint32_t)WHEEL_CIRCUMFERENCE_MM) / g_periodMs;
  return (v > 999) ? 0 : (uint16_t)v;
}

#else

void wheelspeed::begin() {}
void wheelspeed::update(uint32_t) {}
bool wheelspeed::valid() { return false; }
uint16_t wheelspeed::speedKmh10() { return 0; }

#endif
