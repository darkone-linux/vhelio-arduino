#include "telemetry.h"
#include "bafang.h"
#include "config.h"
#include "wheelspeed.h"

namespace {

uint32_t g_lastUpdate = 0;
uint32_t g_odoMm = 0;
uint32_t g_accum = 0;      /* reste d'intégration, en (mm x 36)             */
uint16_t g_speed = 0;
bool g_valid = false;

}  // namespace

void telemetry::begin() {
  g_lastUpdate = 0;
  g_odoMm = 0;
  g_accum = 0;
  g_speed = 0;
  g_valid = false;
}

void telemetry::update(uint32_t now) {
#if SPEED_SOURCE_WHEEL
  g_valid = wheelspeed::valid();
  g_speed = g_valid ? wheelspeed::speedKmh10() : 0;
#else
  g_valid = bafang::speedValid();
  g_speed = g_valid ? bafang::speedKmh10() : 0;
#endif

  const uint32_t dt = now - g_lastUpdate;
  g_lastUpdate = now;

  if (!g_valid || g_speed == 0 || dt == 0 || dt > 1000) {
    return;   /* dt aberrant : premier cycle, ou reprise après blocage */
  }

  /* v[mm/ms] = v[km/h x10] / 36. On accumule le numérateur pour ne pas
   * perdre la troncature à chaque cycle : à 5 km/h et 5 ms de cycle, une
   * division directe perdrait 13 % de la distance. */
  g_accum += (uint32_t)g_speed * dt;
  const uint32_t mm = g_accum / 36UL;
  if (mm > 0) {
    g_odoMm += mm;
    g_accum -= mm * 36UL;
  }
}

bool telemetry::speedValid() { return g_valid; }
uint16_t telemetry::speedKmh10() { return g_speed; }
uint32_t telemetry::odoMm() { return g_odoMm; }
