#include "aux_out.h"
#include "board_io.h"
#include "config.h"
#include "turnsignals.h"

void aux_out::begin() {
  board::setOutput(OUT_AUX, false);
}

void aux_out::update(uint32_t now) {
#if OUT8_ROLE_BUZZER
  board::setOutput(OUT_AUX, turnsignals::buzzerRequest(now));
#else
  /* Relais accessoires : alimenté en permanence tant que le calculateur
   * tourne. Le contact à clé du véhicule reste la coupure principale. */
  (void)now;
  board::setOutput(OUT_AUX, true);
#endif
}
