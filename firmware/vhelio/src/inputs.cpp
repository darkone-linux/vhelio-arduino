#include "inputs.h"
#include "board_io.h"
#include "config.h"
#include "debounce.h"

namespace {

Debouncer g_db[IN_COUNT];
InputState g_state;

/* Durée d'anti-rebond par entrée. Les freins sont les plus rapides : leur
 * retard s'ajoute au délai d'allumage du feu stop (exigence F-2.2). */
const uint16_t DEBOUNCE_MS[IN_COUNT] = {
  DEBOUNCE_COMODO_MS,  /* IN_TURN_LEFT   */
  DEBOUNCE_COMODO_MS,  /* IN_TURN_RIGHT  */
  DEBOUNCE_HORN_MS,    /* IN_AUX         */
  DEBOUNCE_BRAKE_MS,   /* IN_BRAKE_FRONT */
  DEBOUNCE_BRAKE_MS,   /* IN_BRAKE_REAR  */
  DEBOUNCE_COMODO_MS,  /* IN_PARK        */
  DEBOUNCE_COMODO_MS,  /* IN_MAIN        */
  DEBOUNCE_COMODO_MS   /* IN_HAZARD      */
};

/* Inversion logique éventuelle, avant anti-rebond. Ne concerne que les
 * entrées frein lues à travers l'interface transistor de la ligne frein
 * Bafang (hardware/cablage.md §5). Les contacts secs directs, qui sont le
 * câblage retenu, ne l'utilisent pas. */
inline bool applyInversion(uint8_t idx, bool raw) {
  if (idx == IN_BRAKE_FRONT && IN_INVERT_BRAKE_FRONT) return !raw;
  if (idx == IN_BRAKE_REAR && IN_INVERT_BRAKE_REAR) return !raw;
  return raw;
}

}  // namespace

void inputs::begin() {
  for (uint8_t i = 0; i < IN_COUNT; ++i) {
    const bool initial = applyInversion(i, board::readInputRaw(i));
    g_db[i].begin(DEBOUNCE_MS[i], initial);
    g_state.level[i] = initial;
    g_state.rose[i] = false;
    g_state.fell[i] = false;
    g_state.changedAt[i] = 0;
  }
}

void inputs::update(uint32_t now) {
  for (uint8_t i = 0; i < IN_COUNT; ++i) {
#if BRAKE_WIRING_VARIANT == 1
    /* Variante A : une seule entrée frein est câblée. IN_BRAKE_REAR est
     * physiquement libre — la forcer à l'état inactif évite qu'une broche
     * flottante ne déclenche des freinages fantômes. */
    if (i == IN_BRAKE_REAR) {
      g_state.level[i] = false;
      g_state.rose[i] = false;
      g_state.fell[i] = false;
      continue;
    }
#endif
    g_db[i].update(applyInversion(i, board::readInputRaw(i)), now);
    g_state.level[i] = g_db[i].level();
    g_state.rose[i] = g_db[i].rose();
    g_state.fell[i] = g_db[i].fell();
    g_state.changedAt[i] = g_db[i].changedAt();
  }
}

const InputState& inputs::state() {
  return g_state;
}
