#include "diag.h"
#include "bafang.h"
#include "board_io.h"
#include "brakes.h"
#include "config.h"
#include "horn.h"
#include "inputs.h"
#include "lights.h"
#include "telemetry.h"
#include "turnsignals.h"

namespace {

uint8_t g_faults = 0;
uint32_t g_loopUs = 0;
uint32_t g_loopMaxUs = 0;

uint32_t g_ledToggled = 0;
bool g_ledOn = false;

#if DEBUG_SERIAL
uint32_t g_lastLog = 0;

void printFixed1(uint16_t x10) {
  Serial.print(x10 / 10);
  Serial.print('.');
  Serial.print(x10 % 10);
}
#endif

}  // namespace

void diag::begin(uint8_t mcusr) {
  pinMode(PIN_HEARTBEAT, OUTPUT);
  digitalWrite(PIN_HEARTBEAT, LOW);
  g_faults = 0;
  /* WDRF : un reset par chien de garde en roulage est une anomalie, elle
   * doit rester visible après la reprise. */
  if (mcusr & _BV(WDRF)) g_faults |= FLT_WDT_RESET;

#if DEBUG_SERIAL
  Serial.print(F("\n[VH] VHelio firmware "));
  Serial.print(F(VHELIO_FW_VERSION));
  Serial.print(F("  reset=0x"));
  Serial.println(mcusr, HEX);
  if (!board::outputHasPwm(OUT_TAIL)) {
    /* Le feu arrière ne peut plus distinguer veilleuse et stop par
     * l'intensité : c'est une erreur de brochage, pas un détail. */
    Serial.println(F("[VH] ALERTE: OUT_TAIL n'est pas sur une broche PWM"));
  }
#endif
}

void diag::selfTest() {
#if SELFTEST_ENABLE
  /* setup() est le seul endroit où bloquer est acceptable : le chien de
   * garde n'est pas encore armé et rien ne roule. */
  const uint8_t seq[] = {
    OUT_LOWBEAM, OUT_HIGHBEAM, OUT_TURN_LEFT, OUT_TURN_RIGHT, OUT_TAIL, OUT_AUX
  };
  for (uint8_t i = 0; i < sizeof(seq); ++i) {
    board::setOutput(seq[i], true);
    const uint32_t t0 = millis();
    while (millis() - t0 < SELFTEST_STEP_MS) { /* attente active */ }
    board::setOutput(seq[i], false);
  }
#endif
}

void diag::noteLoop(uint32_t us) {
  g_loopUs = us;
  if (us > g_loopMaxUs) g_loopMaxUs = us;
  if (us > LOOP_SLOW_US) g_faults |= FLT_LOOP_SLOW;
}

void diag::update(uint32_t now) {
  /* --- Collecte des défauts. Les bits latchés (WDT, LOOP_SLOW) ne sont pas
   * effacés : ce sont des événements, pas des états. --- */
  const uint8_t latched = g_faults & (FLT_WDT_RESET | FLT_LOOP_SLOW);
  uint8_t f = latched;

  if (turnsignals::conflict()) f |= FLT_TURN_CONFLICT;
  if (horn::stuck()) f |= FLT_HORN_STUCK;
  if (brakes::stuck()) f |= FLT_BRAKE_STUCK;
#if BAFANG_ENABLE
  if (!bafang::linkUp()) f |= FLT_BAFANG_LINK;
#endif
  /* Aucun freinage vu après 2 km : très probablement un fil de contacteur
   * coupé. C'est le seul défaut qui rattrape une panne silencieuse du feu
   * stop en câblage direct (specs/07-securite.md §1). */
  if (!brakes::everBraked() && telemetry::odoMm() > BRAKE_NEVER_MM) {
    f |= FLT_BRAKE_NEVER;
  }
  g_faults = f;

  /* --- LED de vie : 1 Hz nominal, 5 Hz si un défaut est actif. --- */
  const uint16_t period = g_faults ? HEARTBEAT_FAULT_MS : HEARTBEAT_OK_MS;
  if (now - g_ledToggled >= period) {
    g_ledToggled = now;
    g_ledOn = !g_ledOn;
    digitalWrite(PIN_HEARTBEAT, g_ledOn ? HIGH : LOW);
  }

#if DEBUG_SERIAL && !BAFANG_LEARN_MODE
  if (now - g_lastLog < DEBUG_PERIOD_MS) return;
  g_lastLog = now;

  const InputState& in = inputs::state();
  Serial.print(F("[VH] in="));
  for (uint8_t i = 0; i < IN_COUNT; ++i) Serial.print(in.level[i] ? '1' : '0');

  Serial.print(F(" LB=")); Serial.print(lights::lowBeamOn());
  Serial.print(F(" HB=")); Serial.print(lights::highBeamOn());
  Serial.print(F(" TAIL=")); Serial.print(lights::tailDuty());
  Serial.print(F(" TRN="));
  switch (turnsignals::mode()) {
    case turnsignals::LEFT:   Serial.print('L'); break;
    case turnsignals::RIGHT:  Serial.print('R'); break;
    case turnsignals::HAZARD: Serial.print('H'); break;
    default:                  Serial.print('-'); break;
  }
  Serial.print(F(" HN=")); Serial.print(horn::sounding());
  Serial.print(F(" CUT=")); Serial.print(brakes::motorCut());

  Serial.print(F(" spd=")); printFixed1(telemetry::speedKmh10());
  if (!telemetry::speedValid()) Serial.print('?');
  Serial.print(F(" odo=")); Serial.print(telemetry::odoMm() / 1000UL);
  Serial.print(F("m"));
#if BAFANG_ENABLE
  Serial.print(F(" soc=")); Serial.print(bafang::socPct());
  Serial.print(F(" I=")); printFixed1(bafang::currentA10());
  Serial.print(F(" ok=")); Serial.print(bafang::framesOk());
  Serial.print(F(" rej=")); Serial.print(bafang::framesRejected());
#endif
  Serial.print(F(" loop=")); Serial.print(g_loopUs);
  Serial.print('/'); Serial.print(g_loopMaxUs); Serial.print(F("us"));
  Serial.print(F(" flt=0x"));
  if (g_faults < 0x10) Serial.print('0');
  Serial.println(g_faults, HEX);
#endif
}

uint8_t diag::faults() { return g_faults; }
