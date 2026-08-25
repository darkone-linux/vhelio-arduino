/*
 * scan.cpp — Vérification du brochage de la carte DN22D08.
 *
 * À téléverser AVANT tout câblage définitif.
 * Procédure complète : ../../specs/03-affectation-es.md §6.
 *
 * Ce croquis est autonome : il ne dépend d'aucun fichier du firmware, pour
 * pouvoir servir de référence même si le firmware est en cours de refonte.
 *
 * Tout ce qu'il utilise a été MESURÉ, par tools/pinfind puis tools/pinchain
 * (§6 bis). Il ne reste que deux inconnues, et ce croquis existe pour les
 * lever :
 *
 *   1. QUEL BIT PILOTE QUEL RELAIS. On sait que les huit relais occupent les
 *      huit bits du dernier octet de la trame ; on ne sait pas dans quel
 *      ordre. Le croquis promène donc un seul bit et ANNONCE SA POSITION,
 *      sans prétendre savoir à quel relais elle correspond. C'est à l'oreille
 *      — ou à la LED de voie — de compléter le tableau.
 *
 *   2. OÙ EST L'OE. Le balayage a réussi avec D13, A1 et A2 maintenues à
 *      l'état bas. Si l'une des trois est la validation du registre relais,
 *      active à l'état bas, elle était validée sans qu'on le sache. Le
 *      croquis colle les huit relais, puis met chacune des trois au niveau
 *      haut à son tour : celle qui les fait retomber est l'OE. Si aucune ne
 *      les fait retomber, OE est câblée à la masse sur la carte et les trois
 *      broches sont libres.
 *
 * L'afficheur est laissé ÉTEINT — deux octets à zéro. Son brochage interne
 * (sélection des digits, ordre des segments) est encore inconnu, et le faire
 * clignoter ici ne ferait qu'ajouter du bruit à un test qui se juge à
 * l'oreille. C'est une phase séparée, pas encore écrite.
 *
 * Le code est ici et non dans le .ino pour la même raison que dans le
 * firmware : l'IDE Arduino réécrit les .ino et y insère des prototypes
 * générés, ce qui casse selon la version de ctags disponible.
 */

#include <Arduino.h>

/* ---- Mesuré (tools/pinchain, triplet 120) -------------------------------- */
const uint8_t PIN_DATA  = A5;
const uint8_t PIN_CLOCK = A4;
const uint8_t PIN_LATCH = A3;

/* Les trois broches restantes. L'une est peut-être l'OE. */
const uint8_t OE_CAND[3]      = { 13, A1, A2 };
const char* const OE_NAME[3]  = { "D13", "A1", "A2" };

/* ---- Mesuré (tools/pinfind) ---------------------------------------------- */
const uint8_t IN_PIN[8]  = { 2, 3, 4, 5, 6, 7, 9, 11 };
const uint8_t BTN_PIN[4] = { 12, 10, 8, A0 };

const uint16_t STEP_MS = 1500;
const uint16_t REPORT_MS = 400;

/* 0..7 : un bit relais ; 8 : tout coupé ; 9..11 : chasse à l'OE. */
const uint8_t N_STEP = 12;

uint8_t g_step = 8;
uint32_t g_stepAt = 0;
uint32_t g_reportAt = 0;

/* L'octet des relais est le DERNIER émis : c'est ce que le balayage a montré,
 * les huit premières positions du bit promené étant celles qui claquent. */
void shiftFrame(uint8_t relays) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, 0x00);   /* afficheur, éteint */
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, 0x00);   /* afficheur, éteint */
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

  /* BAS avant OUTPUT : si l'une de ces broches est l'OE, active à l'état bas,
   * on veut qu'elle soit déjà au bon niveau au moment où elle devient une
   * sortie. L'ordre inverse produirait une impulsion indéterminée. */
  for (uint8_t i = 0; i < 3; ++i) {
    digitalWrite(OE_CAND[i], LOW);
    pinMode(OE_CAND[i], OUTPUT);
  }

  for (uint8_t i = 0; i < 8; ++i) pinMode(IN_PIN[i], INPUT_PULLUP);
  for (uint8_t i = 0; i < 4; ++i) pinMode(BTN_PIN[i], INPUT_PULLUP);

  shiftFrame(0);

  Serial.println(F("\n=== pinscan DN22D08 ==="));
  Serial.println(F("Chaine MESUREE : donnees=A5 horloge=A4 verrou=A3."));
  Serial.println();
  Serial.println(F("Relais : un bit a la fois, 1,5 s chacun. NOTER, POUR"));
  Serial.println(F("         CHAQUE BIT, LE NUMERO DU RELAIS QUI CLAQUE."));
  Serial.println(F("         L'ordre des bits n'est pas connu : c'est ce"));
  Serial.println(F("         releve qui donnera RELAY_BIT[]."));
  Serial.println(F("OE     : les huit relais collent, puis D13, A1 et A2"));
  Serial.println(F("         passent au haut a tour de role. Celle qui les"));
  Serial.println(F("         fait RETOMBER est l'OE. Si aucune, OE est"));
  Serial.println(F("         cablee a la masse et les trois sont libres."));
  Serial.println(F("Entrees: relier chaque borne a la MASSE, une par une."));
  Serial.println(F("Boutons: K1..K4 sont D12 D10 D8 A0, de gauche a droite."));
  Serial.println(F("Attendu au repos : 11111111 (actif = 0)."));
  Serial.println(F("!! Faisceau NON cable : la chasse a l'OE ferme les huit"));
  Serial.println(F("   circuits en meme temps.\n"));
}

void loop() {
  const uint32_t now = millis();

  if (now - g_stepAt >= STEP_MS) {
    g_stepAt = now;
    g_step = (uint8_t)((g_step + 1) % N_STEP);

    /* Par défaut les trois candidates restent basses. */
    for (uint8_t i = 0; i < 3; ++i) digitalWrite(OE_CAND[i], LOW);

    if (g_step < 8) {
      shiftFrame((uint8_t)(1u << g_step));
      Serial.print(F(">>> BIT "));
      Serial.print(g_step);
      Serial.println(F("  -> quel relais claque ?"));
    } else if (g_step == 8) {
      shiftFrame(0);
      Serial.println(F(">>> TOUS COUPES"));
    } else {
      const uint8_t k = (uint8_t)(g_step - 9);
      shiftFrame(0xFF);
      digitalWrite(OE_CAND[k], HIGH);
      Serial.print(F(">>> OE ? les huit collent, "));
      Serial.print(OE_NAME[k]);
      Serial.println(F(" au HAUT -> retombent-ils ?"));
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
