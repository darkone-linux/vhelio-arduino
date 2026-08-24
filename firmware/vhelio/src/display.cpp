#include "display.h"
#include "config.h"

#if DISPLAY_ENABLE

#include "bafang.h"
#include "board_io.h"
#include "debounce.h"
#include "diag.h"
#include "telemetry.h"

namespace {

uint8_t g_page = DISPLAY_DEFAULT_PAGE;
Debouncer g_btn;

/* Quatre tirets : la grandeur n'est pas disponible, ce qui n'est pas la
 * même chose que « zéro ». */
void showUnavailable() {
  const uint8_t dashes[4] = {
    board::GL_UNDER, board::GL_UNDER, board::GL_UNDER, board::GL_UNDER
  };
  board::showGlyphs(dashes);
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

}  // namespace

void display::begin() {
  g_page = DISPLAY_DEFAULT_PAGE;
  g_btn.begin(DEBOUNCE_BUTTON_MS, false);
  showUnavailable();
}

void display::update(uint32_t now) {
  g_btn.update(board::readButton(BTN_PAGE), now);
  if (g_btn.rose()) {
    g_page = (uint8_t)((g_page + 1) % PAGE_COUNT);
  }

  switch (g_page) {
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

#endif
