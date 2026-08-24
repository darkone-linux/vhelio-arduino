#include "lights.h"
#include "board_io.h"
#include "config.h"
#if BRAKE_FLASH_ENABLE
#include "brakes.h"
#endif

namespace {

bool g_low = false;
bool g_high = false;
uint8_t g_tail = 0;

/* Rampe d'allumage de la veilleuse : limite l'appel de courant sur le
 * convertisseur. Elle ne s'applique JAMAIS au feu stop — rien ne doit
 * retarder un feu de freinage. */
uint32_t g_rampStart = 0;
bool g_ramping = false;

}  // namespace

void lights::begin() {
  g_low = g_high = false;
  g_tail = 0;
  g_ramping = false;
  board::setOutput(OUT_LOWBEAM, false);
  board::setOutput(OUT_HIGHBEAM, false);
  board::setOutputPwm(OUT_TAIL, 0);
}

void lights::update(uint32_t now, const InputState& in, bool braking) {
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

  /* --- Feu arrière : priorité stricte, réévaluée chaque cycle --- */
#if TAIL_ALWAYS_ON
  const bool parkReq = true;
#else
  const bool parkReq = lowReq;
#endif

  uint8_t target;
  if (braking) {
    target = TAIL_PWM_BRAKE;
  } else if (parkReq) {
    target = TAIL_PWM_PARK;
  } else {
    target = 0;
  }

#if BRAKE_FLASH_ENABLE
  /* Phase éteinte du flash d'attaque : on retombe sur l'état d'éclairage. */
  if (braking && brakes::flashBlanking()) {
    target = parkReq ? TAIL_PWM_PARK : 0;
  }
#endif

  uint8_t applied = target;

  /* Rampe uniquement sur la montée 0 -> veilleuse, hors freinage.
   * Le test !g_ramping est indispensable : pendant le premier cycle de rampe
   * le rapport cyclique appliqué vaut encore 0, et sans lui la condition se
   * revérifierait, l'origine serait remise à `now` à chaque tour, et la
   * veilleuse resterait éteinte indéfiniment. */
  if (!braking && target == TAIL_PWM_PARK && g_tail == 0 && !g_ramping) {
    g_ramping = true;
    g_rampStart = now;
  }
  if (braking || target == 0) {
    g_ramping = false;
  }
  if (g_ramping) {
    const uint32_t dt = now - g_rampStart;
    if (dt >= TAIL_SOFTSTART_MS) {
      g_ramping = false;
    } else {
      applied = (uint8_t)((uint32_t)TAIL_PWM_PARK * dt / TAIL_SOFTSTART_MS);
    }
  }

  g_tail = applied;
  board::setOutputPwm(OUT_TAIL, applied);
}

bool lights::lowBeamOn() { return g_low; }
bool lights::highBeamOn() { return g_high; }
uint8_t lights::tailDuty() { return g_tail; }
