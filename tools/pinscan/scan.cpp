/*
 * scan.cpp — Vérification du brochage de la carte DN22D08.
 *
 * À téléverser AVANT tout câblage définitif.
 * Procédure complète : ../../specs/03-affectation-es.md §6.
 *
 * Ce croquis est autonome : il ne dépend d'aucun fichier du firmware, pour
 * pouvoir servir de référence même si le firmware est en cours de refonte.
 *
 * Il vérifie les trois hypothèses sur lesquelles repose tout le firmware :
 *   1. les relais sont pilotés par un registre à décalage (data D13,
 *      horloge A3, verrou A2, validation A1) et non par des broches dédiées ;
 *   2. l'ordre des bits des relais est {R1..R7 -> bits 1..7, R8 -> bit 0} ;
 *   3. les 8 entrées optocouplées sont sur D2 D3 D4 D5 D6 A0 D12 D11 et
 *      sont actives à l'état bas.
 *
 * Si l'une de ces hypothèses est fausse, ce croquis le montre immédiatement,
 * et il faut corriger firmware/vhelio/src/pins.h et board_io.cpp avant tout.
 *
 * Le code est ici et non dans le .ino pour la même raison que dans le
 * firmware : l'IDE Arduino réécrit les .ino et y insère des prototypes
 * générés, ce qui casse selon la version de ctags disponible.
 */

#include <Arduino.h>

/* ---- Hypothèses à vérifier ---------------------------------------------- */
const uint8_t PIN_DATA  = 13;
const uint8_t PIN_CLOCK = A3;
const uint8_t PIN_LATCH = A2;
const uint8_t PIN_OE    = A1;

const uint8_t IN_PIN[8]  = { 2, 3, 4, 5, 6, A0, 12, 11 };
const uint8_t BTN_PIN[4] = { 7, 8, 9, 10 };

/* Bit occupé par chaque relais dans l'octet du registre. */
const uint8_t RELAY_BIT[8] = { 1, 2, 3, 4, 5, 6, 7, 0 };

const uint16_t RELAY_STEP_MS = 1500;
const uint16_t REPORT_MS = 400;

uint8_t g_relayBuffer = 0;
uint8_t g_relay = 0;
uint32_t g_relayAt = 0;
uint32_t g_reportAt = 0;

/* Un seul digit allumé qui tourne : suffit à prouver que la chaîne
 * d'affichage fonctionne, sans embarquer la table des caractères. */
const uint16_t DIGIT_SELECT[4] = { 0x0400, 0x0002, 0x0004, 0x0020 };
uint8_t g_digit = 0;

void shiftFrame(uint16_t displayWord, uint8_t relays) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, lowByte(displayWord));
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, highByte(displayWord));
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, relays);
  digitalWrite(PIN_LATCH, HIGH);
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  digitalWrite(PIN_LATCH, HIGH);
  digitalWrite(PIN_CLOCK, LOW);

  /* HIGH avant OUTPUT : évite une impulsion basse, donc tous les relais
   * collés pendant quelques microsecondes au démarrage. */
  digitalWrite(PIN_OE, HIGH);
  pinMode(PIN_OE, OUTPUT);

  for (uint8_t i = 0; i < 8; ++i) pinMode(IN_PIN[i], INPUT_PULLUP);
  for (uint8_t i = 0; i < 4; ++i) pinMode(BTN_PIN[i], INPUT_PULLUP);

  /* Purger la chaîne avant de valider les relais. */
  for (uint8_t i = 0; i < 4; ++i) shiftFrame(DIGIT_SELECT[i], 0);
  digitalWrite(PIN_OE, LOW);

  Serial.println(F("\n=== pinscan DN22D08 ==="));
  Serial.println(F("Relais : un par un, 1,5 s chacun. Noter le bornier qui colle."));
  Serial.println(F("Entrees : appliquer le signal sur chaque borne IN1..IN8."));
  Serial.println(F("Attendu au repos : 11111111 (actif = 0).\n"));
}

void loop() {
  const uint32_t now = millis();

  /* Rafraîchissement continu : c'est lui qui applique l'état des relais. */
  shiftFrame(DIGIT_SELECT[g_digit], g_relayBuffer);
  g_digit = (g_digit + 1) & 3;

  if (now - g_relayAt >= RELAY_STEP_MS) {
    g_relayAt = now;
    g_relay = (g_relay + 1) % 9;          /* 0..7 = un relais, 8 = tous coupés */
    g_relayBuffer = (g_relay < 8) ? (uint8_t)(1u << RELAY_BIT[g_relay]) : 0;

    Serial.print(F(">>> RELAIS "));
    if (g_relay < 8) {
      Serial.print(F("R"));
      Serial.print(g_relay + 1);
      Serial.print(F("  (bit "));
      Serial.print(RELAY_BIT[g_relay]);
      Serial.print(F(", octet 0x"));
      if (g_relayBuffer < 0x10) Serial.print('0');
      Serial.print(g_relayBuffer, HEX);
      Serial.println(F(")"));
    } else {
      Serial.println(F("TOUS COUPES"));
    }
  }

  if (now - g_reportAt >= REPORT_MS) {
    g_reportAt = now;
    Serial.print(F("    IN1..IN8 = "));
    for (uint8_t i = 0; i < 8; ++i) {
      Serial.print(digitalRead(IN_PIN[i]) == HIGH ? '1' : '0');
    }
    Serial.print(F("   K1..K4 = "));
    for (uint8_t i = 0; i < 4; ++i) {
      Serial.print(digitalRead(BTN_PIN[i]) == HIGH ? '1' : '0');
    }
    Serial.println();
  }
}
