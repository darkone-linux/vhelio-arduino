#include "lights.h"
#include "board_io.h"
#include "config.h"
#if BRAKE_FLASH_ENABLE
#include "brakes.h"
#endif

namespace {

bool g_park = false;      /* veilleuse avant */
bool g_main = false;      /* phares          */
bool g_tail = false;      /* position arrière */
bool g_stop = false;      /* stop arrière     */

}  // namespace

void lights::begin() {
  g_park = g_main = g_tail = g_stop = false;
  board::setOutput(OUT_PARK_FRONT, false);
  board::setOutput(OUT_MAIN, false);
  board::setOutput(OUT_TAIL_PARK, false);
  board::setOutput(OUT_TAIL_STOP, false);
}

void lights::update(uint32_t now, const InputState& in, bool braking) {
  (void)now;

  /* --- Éclairage avant --- */
  const bool parkReq = in.level[IN_PARK];
  bool mainReq = in.level[IN_MAIN];
#if MAIN_REQUIRES_PARK
  mainReq = mainReq && parkReq;
#endif

  g_main = mainReq;
#if MAIN_KEEPS_PARK
  g_park = parkReq || mainReq;
#else
  g_park = parkReq && !mainReq;
#endif

  board::setOutput(OUT_PARK_FRONT, g_park);
  board::setOutput(OUT_MAIN, g_main);

  /* --- Feux arrière : deux circuits indépendants ---
   * La position suit l'éclairage, le stop suit le freinage. Les deux peuvent
   * être allumés ensemble, exactement comme un feu automobile à deux
   * filaments.
   *
   * Noter le OU : le feu arrière s'allume dès qu'un niveau d'éclairage avant
   * est demandé, y compris si l'option MAIN_KEEPS_PARK est désactivée ou si
   * seul le comodo est actionné. Rouler phare allumé sans feu rouge arrière
   * est le scénario qu'il faut rendre impossible par construction. */
#if TAIL_ALWAYS_ON
  g_tail = true;
#else
  g_tail = parkReq || mainReq;
#endif

  g_stop = braking;
#if BRAKE_FLASH_ENABLE
  if (braking && brakes::flashBlanking()) g_stop = false;
#endif

  board::setOutput(OUT_TAIL_PARK, g_tail);
  board::setOutput(OUT_TAIL_STOP, g_stop);
}

bool lights::parkOn() { return g_park; }
bool lights::mainOn() { return g_main; }
bool lights::tailParkOn() { return g_tail; }
bool lights::tailStopOn() { return g_stop; }
