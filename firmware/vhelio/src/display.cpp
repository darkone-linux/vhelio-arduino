#include "display.h"
#include "config.h"

#if DISPLAY_ENABLE

#include <avr/pgmspace.h>

#include "bafang.h"
#include "board_io.h"
#include "brakes.h"
#include "debounce.h"
#include "diag.h"
#include "inputs.h"
#include "lights.h"
#include "telemetry.h"
#include "turnsignals.h"

namespace {

uint8_t g_page = DISPLAY_DEFAULT_PAGE;
Debouncer g_btn;

uint8_t g_point = 0;
bool g_pointChanged = false;
bool g_blank = false;
uint32_t g_blankSince = 0;

/* --- Impulsions d'événement ----------------------------------------------
 * Un freinage peut durer 80 ms, un allumage de phare est instantané : sans
 * maintien, l'afficheur ne montrerait rien de lisible. L'impulsion garde le
 * message DISPLAY_EVENT_MS après le fait. */
struct Pulse {
  uint32_t since;
  bool on;
};

Pulse g_pBrake = { 0, false };
Pulse g_pAck   = { 0, false };
Pulse g_pFault = { 0, false };
Pulse g_pMain  = { 0, false };
Pulse g_pPark  = { 0, false };

bool g_wasBraking = false;
bool g_wasMain = false;
bool g_wasPark = false;

inline void fire(Pulse& p, uint32_t now) {
  p.since = now;
  p.on = true;
}

inline bool alive(Pulse& p, uint32_t now) {
  if (p.on && (now - p.since) >= DISPLAY_EVENT_MS) p.on = false;
  return p.on;
}

/* --- Messages, dans l'ordre de PRIORITÉ décroissante ----------------------
 * Le freinage passe devant tout : c'est le seul événement de cette liste qui
 * engage la sécurité. L'acquittement vient juste après, et DEVANT le défaut :
 * on acquitte précisément parce que « Err » est affiché, un « AC » qui
 * passerait derrière ne serait jamais vu. Puis les allumages, et les
 * clignotants en dernier — ils durent, on a le temps de les voir. */
enum Msg : uint8_t {
  MSG_NONE = 0, MSG_BRAKE, MSG_ACK, MSG_FAULT, MSG_MAIN, MSG_PARK,
  MSG_TURN_L, MSG_TURN_R, MSG_HAZARD, MSG_REMIND, MSG_COUNT
};

const uint8_t MSG_GLYPH[MSG_COUNT][3] PROGMEM = {
  { board::GL_DASH, board::GL_DASH, board::GL_DASH  },  /* ---  rien       */
  { board::GL_F,    board::GL_r,    board::GL_BLANK },  /* Fr   freinage   */
  { board::GL_A,    board::GL_C,    board::GL_BLANK },  /* AC   acquitté   */
  { board::GL_E,    board::GL_r,    board::GL_r     },  /* Err  défaut     */
  { board::GL_P,    board::GL_h,    board::GL_BLANK },  /* Ph   phares     */
  { board::GL_U,    board::GL_E,    board::GL_BLANK },  /* UE   veilleuse  */
  { board::GL_C,    board::GL_L,    board::GL_G     },  /* CLG  gauche     */
  { board::GL_C,    board::GL_L,    board::GL_d     },  /* CLd  droite     */
  { board::GL_C,    board::GL_L,    board::GL_2     },  /* CL2  détresse   */
  { board::GL_C,    board::GL_L,    board::GL_O     }   /* CLO  oubli      */
};

/* Quatre tirets bas : la grandeur n'est pas disponible, ce qui n'est pas la
 * même chose que « zéro ». */
void showUnavailable() {
  const uint8_t dashes[4] = {
    board::GL_UNDER, board::GL_UNDER, board::GL_UNDER, board::GL_UNDER
  };
  board::showGlyphs(dashes);
}

void showBlank() {
  const uint8_t none[4] = {
    board::GL_BLANK, board::GL_BLANK, board::GL_BLANK, board::GL_BLANK
  };
  board::showGlyphs(none);
}

void showFaults(uint8_t f) {
  const uint8_t g[4] = {
    board::GL_F,
    (uint8_t)(f / 100),
    (uint8_t)((f / 10) % 10),
    (uint8_t)(f % 10)
  };
  board::showGlyphs(g);
}

void showEvents(uint32_t now) {
  /* Fronts montants : l'allumage est un événement, l'état allumé n'en est
   * pas un. */
  const bool braking = brakes::braking();
  if (braking && !g_wasBraking) fire(g_pBrake, now);
  g_wasBraking = braking;

  const bool mainOn = lights::mainOn();
  if (mainOn && !g_wasMain) fire(g_pMain, now);
  g_wasMain = mainOn;

  const bool parkOn = lights::parkOn();
  if (parkOn && !g_wasPark) fire(g_pPark, now);
  g_wasPark = parkOn;

  /* Le défaut, lui, n'est pas un front : tant qu'il dure on réarme, si bien
   * que « Err » reste affiché tout du long puis survit DISPLAY_EVENT_MS —
   * un défaut fugace laisse donc quand même une trace lisible.
   *
   * FAULT_LAMP_MASK et non pas tous les défauts : sans lui, un véhicule dont
   * la télémétrie Bafang n'est pas branchée afficherait « Err » en
   * permanence, et le mot cesserait de vouloir dire quoi que ce soit. C'est
   * le principe P2, déjà appliqué au voyant rouge — afficheur et voyant
   * disent ainsi toujours la même chose. */
  if (diag::faults() & FAULT_LAMP_MASK) fire(g_pFault, now);

#if FAULT_LAMP_ENABLE
  /* Le bouton d'acquittement est le seul organe que le conducteur actionne
   * sans qu'aucune sortie ne bouge : sans accusé de réception, rien ne dit
   * que l'appui a été pris en compte. */
  if (inputs::state().rose[IN_ACK]) fire(g_pAck, now);
#endif

  uint8_t m;
  if (alive(g_pBrake, now)) {
    m = MSG_BRAKE;
  } else if (alive(g_pAck, now)) {
    m = MSG_ACK;
  } else if (alive(g_pFault, now)) {
    m = MSG_FAULT;
  } else if (alive(g_pMain, now)) {
    m = MSG_MAIN;
  } else if (alive(g_pPark, now)) {
    m = MSG_PARK;
  } else {
    switch (turnsignals::mode()) {
      case turnsignals::HAZARD: m = MSG_HAZARD; break;
      /* Le rappel d'oubli remplace le côté au lieu de s'y ajouter : après
       * 45 s ou 300 m, savoir QUE le clignotant est resté allumé importe
       * plus que de savoir lequel — et le claquement syncopé, lui, ne dit
       * rien du côté non plus. */
      case turnsignals::LEFT:
      case turnsignals::RIGHT:
        m = turnsignals::reminderActive()
              ? MSG_REMIND
              : (turnsignals::mode() == turnsignals::LEFT ? MSG_TURN_L
                                                          : MSG_TURN_R);
        break;
      default:                  m = MSG_NONE;   break;
    }
  }

  uint8_t g[4];
  for (uint8_t i = 0; i < 3; ++i) g[i] = pgm_read_byte(&MSG_GLYPH[m][i]);
  /* GL_0..GL_9 valent 0..9 : le numéro de point EST son glyphe. */
  g[3] = g_point;
  board::showGlyphs(g);
}

}  // namespace

void display::begin() {
  g_page = DISPLAY_DEFAULT_PAGE;
  g_btn.begin(DEBOUNCE_BUTTON_MS, false);
  g_point = 0;
  g_pointChanged = false;
  g_blank = false;
  showUnavailable();
}

void display::setPoint(uint8_t n) {
  if (n == g_point) return;
  g_point = n;
  /* On ne date pas ici : setPoint est appelé depuis la console, hors du
   * balayage, et tous les modules doivent raisonner sur le même `now`. */
  g_pointChanged = true;
}

bool display::blanking() {
  return g_blank;
}

void display::update(uint32_t now) {
  g_btn.update(board::readButton(BTN_PAGE), now);
  if (g_btn.rose()) {
    g_page = (uint8_t)((g_page + 1) % PAGE_COUNT);
  }

  if (g_pointChanged) {
    g_pointChanged = false;
    g_blank = true;
    g_blankSince = now;
  }
  if (g_blank) {
    if (now - g_blankSince < DISPLAY_BLANK_MS) {
      showBlank();
      return;
    }
    g_blank = false;
  }

  switch (g_page) {
    case PAGE_EVENTS:
      showEvents(now);
      break;

    case PAGE_SPEED:
      if (telemetry::speedValid()) {
        board::showNumber(telemetry::speedKmh10() / 10);
      } else {
        showUnavailable();
      }
      break;

    case PAGE_SOC: {
#if BAFANG_ENABLE
      const uint8_t soc = bafang::socPct();
      if (bafang::linkUp() && soc > 0) {
        board::showNumber(soc);
      } else {
        showUnavailable();
      }
#else
      showUnavailable();
#endif
      break;
    }

    case PAGE_FAULTS:
      showFaults(diag::faults());
      break;

    case PAGE_ODO:
      /* Odomètre en kilomètres entiers ; déborde au-delà de 9999 km, ce qui
       * n'a pas de sens ici puisqu'il repart à zéro à chaque mise sous
       * tension. */
      board::showNumber((uint16_t)(telemetry::odoMm() / 1000000UL));
      break;

    default:
      showUnavailable();
      break;
  }
}

uint8_t display::page() { return g_page; }

#else

void display::begin() {}
void display::update(uint32_t) {}
uint8_t display::page() { return 0; }
void display::setPoint(uint8_t) {}
bool display::blanking() { return false; }

#endif
