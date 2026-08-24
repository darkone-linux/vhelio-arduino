#include "horn.h"
#include "board_io.h"
#include "config.h"

namespace {

enum State : uint8_t { IDLE, SOUND, LOCKED };

State g_state = IDLE;
uint32_t g_soundSince = 0;

}  // namespace

void horn::begin() {
  g_state = IDLE;
  board::setOutput(OUT_HORN, false);
}

void horn::update(uint32_t now, const InputState& in) {
  const bool pressed = in.level[IN_HORN];

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

  board::setOutput(OUT_HORN, g_state == SOUND);
}

bool horn::sounding() { return g_state == SOUND; }
bool horn::stuck() { return g_state == LOCKED; }
