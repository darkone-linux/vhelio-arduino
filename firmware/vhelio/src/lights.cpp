#include "lights.h"
#include "board_io.h"
#include "config.h"
#if BRAKE_FLASH_ENABLE
#include "brakes.h"
#endif

namespace {

bool g_low = false;
bool g_high = false;
bool g_park = false;
bool g_stop = false;

}  // namespace

void lights::begin() {
  g_low = g_high = g_park = g_stop = false;
  board::setOutput(OUT_LOWBEAM, false);
  board::setOutput(OUT_HIGHBEAM, false);
  board::setOutput(OUT_TAIL_PARK, false);
  board::setOutput(OUT_TAIL_STOP, false);
}

void lights::update(uint32_t now, const InputState& in, bool braking) {
  (void)now;

  /* --- Phares --- */
  const bool lowReq = in.level[IN_LOWBEAM];
  bool highReq = in.level[IN_HIGHBEAM];
#if HIGHBEAM_REQUIRES_LOWBEAM
  highReq = highReq && lowReq;
#endif

  g_high = highReq;
#if HIGHBEAM_KEEPS_LOWBEAM
  g_low = lowReq;
#else
  g_low = lowReq && !highReq;
#endif

  board::setOutput(OUT_LOWBEAM, g_low);
  board::setOutput(OUT_HIGHBEAM, g_high);

  /* --- Feux arrière : deux circuits indépendants ---
   * La veilleuse suit l'éclairage, le stop suit le freinage. Les deux
   * peuvent être allumés ensemble, exactement comme un feu automobile à
   * deux filaments. */
#if TAIL_ALWAYS_ON
  g_park = true;
#else
  g_park = lowReq;
#endif

  g_stop = braking;
#if BRAKE_FLASH_ENABLE
  if (braking && brakes::flashBlanking()) g_stop = false;
#endif

  board::setOutput(OUT_TAIL_PARK, g_park);
  board::setOutput(OUT_TAIL_STOP, g_stop);
}

bool lights::lowBeamOn() { return g_low; }
bool lights::highBeamOn() { return g_high; }
bool lights::tailParkOn() { return g_park; }
bool lights::tailStopOn() { return g_stop; }
