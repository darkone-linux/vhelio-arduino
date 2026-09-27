#include "brakes.h"
#include "board_io.h"
#include "config.h"

namespace {

enum CutState : uint8_t { RELEASED, CUT, HOLD };

CutState g_cut = RELEASED;
uint32_t g_holdSince = 0;
uint32_t g_brakingSince = 0;
bool g_braking = false;
bool g_seenFront = false;   /* entrée frein avant vue active au moins une fois */
bool g_seenRear = false;    /* idem, frein arrière                            */
bool g_stuck = false;

#if BRAKE_FLASH_ENABLE
enum FlashState : uint8_t { FLASH_IDLE, FLASH_ON, FLASH_OFF, FLASH_SOLID };
FlashState g_flash = FLASH_IDLE;
uint8_t g_flashLeft = 0;
uint32_t g_flashSince = 0;
#endif

}  // namespace

void brakes::begin() {
  g_cut = RELEASED;
  g_braking = false;
  g_seenFront = false;
  g_seenRear = false;
  g_stuck = false;
  board::setOutput(OUT_MOTOR_CUT, false);
}

void brakes::update(uint32_t now, const InputState& in) {
  const bool wasBraking = g_braking;
  g_braking = in.level[IN_BRAKE_FRONT] || in.level[IN_BRAKE_REAR];

  /* Une trace PAR ENTRÉE : avec deux contacteurs, c'est justement le fil de
   * l'un d'eux qu'on cherche à trouver coupé, pendant que l'autre fonctionne. */
  if (in.level[IN_BRAKE_FRONT]) g_seenFront = true;
  if (in.level[IN_BRAKE_REAR]) g_seenRear = true;

  if (g_braking && !wasBraking) {
    g_brakingSince = now;
#if BRAKE_FLASH_ENABLE
    g_flash = FLASH_ON;
    g_flashLeft = BRAKE_FLASH_COUNT;
    g_flashSince = now;
#endif
  }

#if BRAKE_FLASH_ENABLE
  if (!g_braking) {
    g_flash = FLASH_IDLE;
  } else {
    switch (g_flash) {
      case FLASH_ON:
        if (now - g_flashSince >= BRAKE_FLASH_ON_MS) {
          g_flashSince = now;
          g_flash = (--g_flashLeft == 0) ? FLASH_SOLID : FLASH_OFF;
        }
        break;
      case FLASH_OFF:
        if (now - g_flashSince >= BRAKE_FLASH_OFF_MS) {
          g_flashSince = now;
          g_flash = FLASH_ON;
        }
        break;
      default:
        break;
    }
  }
#endif

  /* Automate de coupure moteur. L'état HOLD absorbe les micro-relâchements
   * d'un levier modulé : sans lui, l'assistance se réengagerait par à-coups
   * en plein freinage. */
  switch (g_cut) {
    case RELEASED:
      if (g_braking) g_cut = CUT;
      break;
    case CUT:
      if (!g_braking) {
        g_cut = HOLD;
        g_holdSince = now;
      }
      break;
    case HOLD:
      if (g_braking) {
        g_cut = CUT;
      } else if (now - g_holdSince >= BRAKE_HOLD_MS) {
        g_cut = RELEASED;
      }
      break;
  }

  board::setOutput(OUT_MOTOR_CUT, g_cut != RELEASED);

  /* Calculé ici, et pas dans stuck() : tous les modules doivent raisonner
   * sur le même instant `now`, capturé une seule fois par cycle. */
  g_stuck = g_braking && (now - g_brakingSince >= BRAKE_STUCK_MS);
}

bool brakes::braking() { return g_braking; }
bool brakes::motorCut() { return g_cut != RELEASED; }
bool brakes::allBrakesSeen() {
#if BRAKE_WIRING_VARIANT == 1
  return g_seenFront;       /* IN_BRAKE_REAR n'est pas câblée */
#else
  return g_seenFront && g_seenRear;
#endif
}

bool brakes::stuck() { return g_stuck; }

#if BRAKE_FLASH_ENABLE
bool brakes::flashBlanking() { return g_flash == FLASH_OFF; }
#endif
