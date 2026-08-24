#include "turnsignals.h"
#include "board_io.h"
#include "config.h"
#include "telemetry.h"

namespace {

turnsignals::Mode g_mode = turnsignals::OFF;
bool g_conflict = false;

uint32_t g_phaseStart = 0;   /* origine du cycle de clignotement           */
bool g_phaseOn = false;      /* phase allumée en cours                     */

uint32_t g_modeSince = 0;    /* entrée dans LEFT/RIGHT                     */
uint32_t g_modeOdoMm = 0;    /* odomètre à l'entrée dans LEFT/RIGHT        */
bool g_reminder = false;

}  // namespace

void turnsignals::begin() {
  g_mode = OFF;
  g_conflict = false;
  g_phaseOn = false;
  g_reminder = false;
  board::setOutput(OUT_TURN_LEFT, false);
  board::setOutput(OUT_TURN_RIGHT, false);
}

void turnsignals::update(uint32_t now, const InputState& in) {
  /* --- Choix du mode. Le comodo est un inverseur maintenu : l'état est
   * recalculé à chaque cycle à partir des niveaux, pas des fronts. --- */
  const bool left = in.level[IN_TURN_LEFT];
  const bool right = in.level[IN_TURN_RIGHT];
  g_conflict = left && right;

  Mode wanted;
  if (in.level[IN_HAZARD]) {
    wanted = HAZARD;
  } else if (left && !right) {
    wanted = LEFT;
  } else if (right && !left) {
    wanted = RIGHT;
  } else {
    wanted = OFF;
  }

  if (wanted != g_mode) {
    g_mode = wanted;
    /* Remise à zéro de la phase : garantit un démarrage sur allumé (F-3.3)
     * et évite un premier flash tronqué à la bascule gauche/droite. */
    g_phaseStart = now;
    g_phaseOn = (g_mode != OFF);
    g_modeSince = now;
    g_modeOdoMm = telemetry::odoMm();
    g_reminder = false;
  }

  if (g_mode == OFF) {
    g_phaseOn = false;
    board::setOutput(OUT_TURN_LEFT, false);
    board::setOutput(OUT_TURN_RIGHT, false);
    return;
  }

  /* --- Rappel d'oubli. Pas d'annulation automatique : le comodo resterait
   * en position, et l'état affiché divergerait de l'état réel. On alerte,
   * on ne décide pas à la place du conducteur. --- */
  if (g_mode == LEFT || g_mode == RIGHT) {
    const bool tooLong = (now - g_modeSince) >= BLINK_REMINDER_MS;
    const bool tooFar = telemetry::speedValid() &&
                        (telemetry::odoMm() - g_modeOdoMm) >= BLINK_REMINDER_MM;
    g_reminder = tooLong || tooFar;
  } else {
    g_reminder = false;  /* la détresse est intentionnellement durable */
  }

  /* --- Cadence. --- */
  uint32_t elapsed = now - g_phaseStart;
  while (elapsed >= BLINK_PERIOD_MS) {
    g_phaseStart += BLINK_PERIOD_MS;
    elapsed -= BLINK_PERIOD_MS;
  }

  /* Aucun buzzer : les clignotants sont portés par des relais, dont le
   * claquement à 1,33 Hz EST le retour sonore. C'est exactement le bruit
   * d'un relais de clignotant d'origine, et il ne coûte ni sortie ni
   * composant.
   *
   * Le rappel d'oubli exploite le même canal. Il raccourcit la phase allumée
   * sans toucher à la PÉRIODE : la cadence reste à 80 cycles/min, donc dans
   * la plage réglementaire, mais le rythme du claquement passe de régulier à
   * syncopé. C'est audible depuis le poste de conduite alors même que le
   * boîtier est sous la coque et l'afficheur invisible (Q12). */
  uint16_t onMs = BLINK_ON_MS;
#if BLINK_REMINDER_ON_MS
  if (g_reminder) onMs = BLINK_REMINDER_ON_MS;
#endif
  const bool on = (elapsed < onMs);

  g_phaseOn = on;

  board::setOutput(OUT_TURN_LEFT, on && (g_mode == LEFT || g_mode == HAZARD));
  board::setOutput(OUT_TURN_RIGHT, on && (g_mode == RIGHT || g_mode == HAZARD));
}

turnsignals::Mode turnsignals::mode() { return g_mode; }
bool turnsignals::conflict() { return g_conflict; }
bool turnsignals::reminderActive() { return g_reminder; }

