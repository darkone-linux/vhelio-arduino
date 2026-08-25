#include "diag.h"
#include "bafang.h"
#include "debounce.h"
#include "board_io.h"
#include "brakes.h"
#include "config.h"
#include "display.h"
#include "horn.h"
#include "inputs.h"
#include "lights.h"
#include "simconsole.h"
#include "telemetry.h"
#include "turnsignals.h"

namespace {

uint8_t g_faults = 0;
uint8_t g_acked = 0;      /* défauts acquittés, voyant éteint pour eux */
uint32_t g_loopUs = 0;
uint32_t g_loopMaxUs = 0;

uint32_t g_beatToggled = 0;
bool g_beatOn = false;

#if FAULT_LAMP_ENABLE
Debouncer g_ackBtn;

/* P2 — la télémétrie est un confort, pas une fonction de sécurité. Un bus
 * Bafang muet ne doit pas allumer un voyant rouge devant le conducteur :
 * il resterait allumé en permanence et le voyant ne voudrait plus rien dire.
 * Ce contrôle existe pour qu'on ne puisse pas l'oublier en modifiant le
 * masque sans relire la justification. */
static_assert((FAULT_LAMP_MASK & diag::FLT_BAFANG_LINK) == 0,
              "FLT_BAFANG_LINK ne doit pas allumer le voyant (principe P2)");
#endif

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
  g_faults = 0;
  g_acked = 0;
#if FAULT_LAMP_ENABLE
  g_ackBtn.begin(DEBOUNCE_ACK_MS, false);
  board::setOutput(OUT_FAULT, false);
#endif
  /* WDRF : un reset par chien de garde en roulage est une anomalie, elle
   * doit rester visible après la reprise. */
  if (mcusr & _BV(WDRF)) g_faults |= FLT_WDT_RESET;

#if DEBUG_SERIAL
  Serial.print(F("\n[VH] Vhelio firmware "));
  Serial.print(F(VHELIO_FW_VERSION));
  Serial.print(F("  reset=0x"));
  Serial.println(mcusr, HEX);
#endif
}

void diag::selfTest() {
#if SELFTEST_ENABLE
  /* setup() est le seul endroit où bloquer est acceptable : le chien de
   * garde n'est pas encore armé et rien ne roule.
   * Le klaxon et la coupure moteur sont volontairement exclus. */
  /* Le voyant de défaut est inclus : c'est le contrôle du voyant lui-même,
   * exactement comme les témoins d'un tableau de bord qui s'allument à la
   * mise du contact. Sans cela, une LED grillée serait indiscernable d'une
   * absence de défaut. La coupure moteur (R8) reste exclue. */
  const uint8_t seq[] = {
    OUT_PARK_FRONT, OUT_MAIN, OUT_TURN_LEFT, OUT_TURN_RIGHT,
    OUT_TAIL_PARK, OUT_TAIL_STOP,
#if FAULT_LAMP_ENABLE
    OUT_FAULT
#endif
  };

  /* Tous les segments et tous les points, pendant que les relais claquent.
   * Un segment mort ne se découvre que là : en usage normal, il ne
   * manquerait qu'un morceau de caractère, ce qui se lit comme un autre
   * caractère plutôt que comme une panne. */
  const uint32_t tLamp = millis();
  const uint8_t all[4] = {
    board::GL_TEST, board::GL_TEST, board::GL_TEST, board::GL_TEST
  };
  board::showGlyphs(all);

  for (uint8_t i = 0; i < sizeof(seq); ++i) {
    board::setOutput(seq[i], true);
    const uint32_t t0 = millis();
    while (millis() - t0 < SELFTEST_STEP_MS) {
      board::refresh();   /* l'afficheur doit continuer d'être multiplexé */
    }
    board::setOutput(seq[i], false);
  }

  /* Les relais ont pris 7 x 200 ms ; on ne prolonge que le reliquat. */
  while (millis() - tLamp < SELFTEST_LAMP_MS) board::refresh();

  const uint8_t none[4] = {
    board::GL_BLANK, board::GL_BLANK, board::GL_BLANK, board::GL_BLANK
  };
  board::showGlyphs(none);
  board::refresh();
#endif
}

void diag::noteLoop(uint32_t us) {
  g_loopUs = us;
  if (us > g_loopMaxUs) g_loopMaxUs = us;
  if (us > LOOP_SLOW_US) g_faults |= FLT_LOOP_SLOW;
}

void diag::acknowledge() {
  /* Les défauts mémorisés disparaissent : ce sont des événements passés, et
   * sans cela ils survivraient jusqu'à la coupure de l'alimentation. */
  g_faults &= (uint8_t)~(FLT_WDT_RESET | FLT_LOOP_SLOW);
  /* Les défauts encore actifs restent signalés à l'afficheur et au journal,
   * mais cessent d'allumer le voyant. */
  g_acked |= (uint8_t)(g_faults & FAULT_LAMP_MASK);
}

void diag::update(uint32_t now) {
  /* --- Collecte des défauts. Les bits latchés (WDT, cycle lent) ne sont pas
   * effacés : ce sont des événements, pas des états. --- */
  const uint8_t latched = g_faults & (FLT_WDT_RESET | FLT_LOOP_SLOW);
  uint8_t f = latched;

  if (turnsignals::conflict()) f |= FLT_TURN_CONFLICT;
#if HORN_ENABLE
  if (horn::stuck()) f |= FLT_HORN_STUCK;
#endif
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

  /* --- Voyant de défaut sur R7. ---
   * Allumage FIXE : un relais n'est pas fait pour clignoter, c'est déjà la
   * raison pour laquelle R3 et R4 sont les pièces d'usure du montage. La
   * distinction entre défauts se lit sur l'afficheur, coque ouverte. */
#if FAULT_LAMP_ENABLE
  g_ackBtn.update(inputs::state().level[IN_ACK], now);
  if (g_ackBtn.rose()) acknowledge();

  /* Un défaut qui disparaît perd son acquittement : s'il revient, il rallume
   * le voyant. L'acquittement porte sur un événement, pas sur une catégorie. */
  g_acked &= g_faults;
  board::setOutput(OUT_FAULT, (g_faults & FAULT_LAMP_MASK & ~g_acked) != 0);
#endif

  /* --- Battement de cœur sur le point décimal du digit de gauche.
   * L'afficheur n'a pas de deux-points, contrairement à ce que supposait la
   * conception initiale : rien que des points décimaux. Et la LED D13 du
   * Nano n'est pas utilisable non plus — elle porte le TX logiciel du
   * Bafang, que SoftwareSerial maintient au repos à l'état haut : elle
   * reste allumée en fixe. --- */
  const uint16_t period = g_faults ? DISPLAY_BLINK_FAULT_MS : DISPLAY_BLINK_MS;
  if (now - g_beatToggled >= period) {
    g_beatToggled = now;
    g_beatOn = !g_beatOn;
  }
  /* Éteint aussi pendant le noir qui sépare deux points de contrôle : c'est
   * le seul marqueur dont dispose l'opérateur, il doit être franc.
   * setHeartbeat() ne fait rien quand l'état ne change pas, appeler à chaque
   * tour ne coûte donc rien. */
  board::setHeartbeat(g_beatOn && !display::blanking());

#if DEBUG_SERIAL && !BAFANG_LEARN_MODE
  if (now - g_lastLog < DEBUG_PERIOD_MS) return;
  g_lastLog = now;

  const InputState& in = inputs::state();
  Serial.print(F("[VH] in="));
  for (uint8_t i = 0; i < IN_COUNT; ++i) Serial.print(in.level[i] ? '1' : '0');

#if SIM_INPUTS
  /* Quelles bornes ne sont PAS lues sur leur optocoupleur. Cette colonne est
   * la seule chose qui distingue une ligne de journal de banc d'une ligne de
   * journal de roulage : elle doit rester sous les yeux. */
  Serial.print(F(" sim="));
  for (uint8_t i = 0; i < IN_COUNT; ++i) {
    Serial.print(simconsole::active(i) ? '1' : '0');
  }
#endif

  Serial.print(F(" VL=")); Serial.print(lights::parkOn());
  Serial.print(F(" PH=")); Serial.print(lights::mainOn());
  Serial.print(F(" AR=")); Serial.print(lights::tailParkOn());
  Serial.print(F(" ST=")); Serial.print(lights::tailStopOn());
  Serial.print(F(" TRN="));
  switch (turnsignals::mode()) {
    case turnsignals::LEFT:   Serial.print('L'); break;
    case turnsignals::RIGHT:  Serial.print('R'); break;
    case turnsignals::HAZARD: Serial.print('H'); break;
    default:                  Serial.print('-'); break;
  }
  if (turnsignals::reminderActive()) Serial.print('!');
#if HORN_ENABLE
  Serial.print(F(" HN=")); Serial.print(horn::sounding());
#endif
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
#if DISPLAY_ENABLE
  Serial.print(F(" pg=")); Serial.print(display::page());
#endif
  Serial.print(F(" loop=")); Serial.print(g_loopUs);
  Serial.print('/'); Serial.print(g_loopMaxUs); Serial.print(F("us"));
  Serial.print(F(" flt=0x"));
  if (g_faults < 0x10) Serial.print('0');
  Serial.print(g_faults, HEX);
#if FAULT_LAMP_ENABLE
  if (lampOn()) Serial.print(F(" LAMP"));
  else if (g_acked) Serial.print(F(" ack"));
#endif
  Serial.println();
#endif
}

uint8_t diag::faults() { return g_faults; }

bool diag::lampOn() {
#if FAULT_LAMP_ENABLE
  return (g_faults & FAULT_LAMP_MASK & ~g_acked) != 0;
#else
  return false;
#endif
}
