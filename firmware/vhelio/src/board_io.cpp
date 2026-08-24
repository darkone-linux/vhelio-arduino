#include "board_io.h"
#include "config.h"

namespace {

/* ---------------------------------------------------------------------
 * Brochage physique. C'EST ICI qu'on corrige si la carte diffère.
 * Procédure de vérification : specs/03-affectation-es.md §5.
 * ------------------------------------------------------------------ */
const uint8_t OUT_PIN[OUT_COUNT] = { 2, 3, 4, 5, 6, 7, 8, 9 };
const uint8_t IN_PIN[IN_COUNT]   = { A0, A1, A2, A3, A4, A5, A6, A7 };

/* Entrée optocouplée : +12 V sur la borne allume la LED, qui tire la broche
 * du Nano à l'état bas. D'où « actif = niveau bas ». */
const bool IN_ACTIVE_LOW[IN_COUNT] = {
  true, true, true, true, true, true, true, true
};

/* Sortie : broche à l'état haut = charge alimentée. */
const bool OUT_ACTIVE_HIGH[OUT_COUNT] = {
  true, true, true, true, true, true, true, true
};

uint8_t g_duty[OUT_COUNT];

/* A6 et A7 n'ont pas de tampon d'entrée numérique sur l'ATmega328P :
 * digitalRead() y renvoie n'importe quoi. */
inline bool isAnalogOnly(uint8_t pin) {
  return pin == A6 || pin == A7;
}

/* Broches à PWM matériel sur un Nano. */
inline bool hasPwm(uint8_t pin) {
  return pin == 3 || pin == 5 || pin == 6 || pin == 9 || pin == 10 || pin == 11;
}

}  // namespace

void board::begin() {
  for (uint8_t i = 0; i < OUT_COUNT; ++i) {
    pinMode(OUT_PIN[i], OUTPUT);
    g_duty[i] = 0;
    digitalWrite(OUT_PIN[i], OUT_ACTIVE_HIGH[i] ? LOW : HIGH);
  }
  for (uint8_t i = 0; i < IN_COUNT; ++i) {
    /* Le tirage interne donne un état inactif franc si la carte d'E/S n'est
     * pas alimentée. Sans effet sur A6/A7, qui n'en ont pas : ces deux
     * entrées dépendent du tirage présent sur la carte. */
    if (!isAnalogOnly(IN_PIN[i])) {
      pinMode(IN_PIN[i], INPUT_PULLUP);
    }
  }
}

bool board::readInputRaw(uint8_t idx) {
  const uint8_t pin = IN_PIN[idx];
  bool electricalHigh;
  if (isAnalogOnly(pin)) {
    electricalHigh = (analogRead(pin) >= ANALOG_INPUT_THRESHOLD);
  } else {
    electricalHigh = (digitalRead(pin) == HIGH);
  }
  return IN_ACTIVE_LOW[idx] ? !electricalHigh : electricalHigh;
}

void board::setOutput(uint8_t idx, bool on) {
  g_duty[idx] = on ? 255 : 0;
  const bool level = OUT_ACTIVE_HIGH[idx] ? on : !on;
  /* digitalWrite coupe le PWM éventuellement en cours sur la broche. */
  digitalWrite(OUT_PIN[idx], level ? HIGH : LOW);
}

void board::setOutputPwm(uint8_t idx, uint8_t duty) {
  g_duty[idx] = duty;
  const uint8_t pin = OUT_PIN[idx];
  const uint8_t applied = OUT_ACTIVE_HIGH[idx] ? duty : (uint8_t)(255 - duty);

  if (!hasPwm(pin)) {
    digitalWrite(pin, applied >= 128 ? HIGH : LOW);
    return;
  }
  /* Les extrêmes en tout-ou-rien : analogWrite(0) laisse un résidu sur
   * certaines broches, et cela évite de garder un timer occupé pour rien. */
  if (applied == 0) {
    digitalWrite(pin, LOW);
  } else if (applied == 255) {
    digitalWrite(pin, HIGH);
  } else {
    analogWrite(pin, applied);
  }
}

uint8_t board::outputDuty(uint8_t idx) {
  return g_duty[idx];
}

bool board::outputHasPwm(uint8_t idx) {
  return hasPwm(OUT_PIN[idx]);
}

void board::allOff() {
  for (uint8_t i = 0; i < OUT_COUNT; ++i) {
    setOutput(i, false);
  }
}
