#include "horn.h"
#include "config.h"

#if HORN_ENABLE

/* Le klaxon réutilise la voie IN3 / R7, normalement affectée au voyant de
 * défaut ; config.h interdit les deux à la fois. */
#include "board_io.h"

namespace {

enum State : uint8_t { IDLE, SOUND, LOCKED };

State g_state = IDLE;
uint32_t g_soundSince = 0;

}  // namespace

void horn::begin() {
  g_state = IDLE;
  board::setOutput(OUT_FAULT, false);
}

void horn::update(uint32_t now, const InputState& in) {
  const bool pressed = in.level[IN_ACK];

  switch (g_state) {
    case IDLE:
      if (pressed) {
        g_state = SOUND;
        g_soundSince = now;
      }
      break;
    case SOUND:
      if (!pressed) {
        g_state = IDLE;
      } else if (now - g_soundSince >= HORN_MAX_ON_MS) {
        /* Protège le klaxon et le convertisseur si le bouton reste collé ou
         * si un fil se met à la masse. Le relâchement réarme. */
        g_state = LOCKED;
      }
      break;
    case LOCKED:
      if (!pressed) {
        g_state = IDLE;
      }
      break;
  }

  board::setOutput(OUT_FAULT, g_state == SOUND);
}

bool horn::sounding() { return g_state == SOUND; }
bool horn::stuck() { return g_state == LOCKED; }

#else

/* Klaxon autonome : la voie IN3 / R7 porte le voyant de défaut et son bouton
 * d'acquittement (module diag). Rien à faire ici. */
void horn::begin() {}
void horn::update(uint32_t, const InputState&) {}
bool horn::sounding() { return false; }
bool horn::stuck() { return false; }

#endif
