/*
 * scan.cpp — Vérification du brochage de la carte DN22D08.
 *
 * À téléverser AVANT tout câblage définitif. Procédure complète dans
 * specs/03-affectation-es.md §5.
 *
 * Deux phases en alternance :
 *   - SORTIES : D2..D9 activées l'une après l'autre, 1 s chacune, avec le nom
 *     annoncé sur le port série. Mesurer sur chaque bornier de sortie et noter
 *     la correspondance réelle.
 *   - ENTRÉES : état de A0..A7 publié 4 fois par seconde. Appliquer +12 V
 *     successivement sur chaque borne d'entrée et noter quelle broche bascule,
 *     ET DANS QUEL SENS.
 *
 * A6 et A7 n'ont pas de tampon numérique sur l'ATmega328P : elles sont lues en
 * analogique et la valeur brute est affichée en plus de l'état seuillé.
 *
 * Le code est ici et non dans le .ino pour la même raison que dans le
 * firmware : l'IDE Arduino réécrit les .ino et y insère des prototypes
 * générés, ce qui casse selon la version de ctags disponible.
 */

#include <Arduino.h>

const uint8_t OUT_PINS[] = { 2, 3, 4, 5, 6, 7, 8, 9 };
const uint8_t IN_PINS[]  = { A0, A1, A2, A3, A4, A5, A6, A7 };
const uint8_t N = 8;

const uint16_t STEP_MS = 1000;
const uint16_t IN_PERIOD_MS = 250;

uint8_t g_step = 0;
uint32_t g_stepAt = 0;
uint32_t g_inAt = 0;

bool isAnalogOnly(uint8_t pin) { return pin == A6 || pin == A7; }

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < N; ++i) {
    pinMode(OUT_PINS[i], OUTPUT);
    digitalWrite(OUT_PINS[i], LOW);
    if (!isAnalogOnly(IN_PINS[i])) pinMode(IN_PINS[i], INPUT_PULLUP);
  }
  Serial.println(F("\n=== pinscan DN22D08 ==="));
  Serial.println(F("Sorties: D2..D9, 1 s chacune."));
  Serial.println(F("Entrees: A0..A7, appliquer +12 V sur chaque borne."));
  Serial.println(F("A6/A7 lues en analogique (valeur brute affichee).\n"));
}

void loop() {
  const uint32_t now = millis();

  if (now - g_stepAt >= STEP_MS) {
    g_stepAt = now;
    digitalWrite(OUT_PINS[g_step], LOW);
    g_step = (g_step + 1) % N;
    digitalWrite(OUT_PINS[g_step], HIGH);
    Serial.print(F(">>> SORTIE ACTIVE : D"));
    Serial.print(OUT_PINS[g_step]);
    Serial.print(F("  (attendu OUT"));
    Serial.print(g_step + 1);
    Serial.println(F(")"));
  }

  if (now - g_inAt >= IN_PERIOD_MS) {
    g_inAt = now;
    Serial.print(F("    entrees A0..A7 : "));
    for (uint8_t i = 0; i < N; ++i) {
      const uint8_t pin = IN_PINS[i];
      if (isAnalogOnly(pin)) {
        const int v = analogRead(pin);
        Serial.print(v >= 512 ? '1' : '0');
        Serial.print('(');
        Serial.print(v);
        Serial.print(')');
      } else {
        Serial.print(digitalRead(pin) == HIGH ? '1' : '0');
      }
      Serial.print(' ');
    }
    Serial.println();
  }
}
