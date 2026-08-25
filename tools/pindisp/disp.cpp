/*
 * disp.cpp — Phase D : trouver segments et sélection de digits.
 *
 * Acquis par la mesure (phases A à C) :
 *     données A5, horloge A4, verrou A3, OE A2 active à l'état bas
 *     octet des RELAIS = le DERNIER des trois émis, bits 0..7, CH1..CH8
 *
 * Restent les seize bits de l'afficheur : les deux premiers octets émis,
 * bits 8..23 de la trame. On sait qu'ils portent les segments et la sélection
 * des quatre digits ; on ne sait ni lequel fait quoi, ni dans quel ordre.
 *
 * Une documentation officieuse propose : premier octet émis = sélection des
 * digits, deuxième = segments, codage DP-G-F-E-D-C-B-A. C'est une hypothèse
 * cohérente avec ce qu'on a vu — en promenant un bit, les segments
 * s'allumaient sur les quatre digits à la fois, ce qui est le comportement
 * attendu si la sélection valait alors zéro et les activait tous. Mais la
 * même source donne un brochage de chaîne faux pour cette carte, réfuté par
 * le balayage. Elle sert donc à ORIENTER la mesure, pas à la remplacer.
 *
 * MÉTHODE. Un seul bit d'afficheur allumé à la fois, l'octet des relais tenu
 * à zéro — l'afficheur se règle à l'œil, et des relais qui claquent
 * n'ajouteraient que du bruit. On avance à la main, aux boutons de la carte,
 * pour la même raison qu'à la phase B : rien ne doit défiler.
 *
 *     K1 (D12) : bit suivant
 *     K2 (D10) : bit précédent
 *     K3 (D8)  : l'AUTRE octet d'afficheur, 0x00 <-> 0xFF
 *     K4 (A0)  : avance automatique, 1,5 s par bit
 *
 * K3 EST LE TEST DÉCISIF. Avec l'autre octet à 0xFF, si l'octet qu'on promène
 * est celui des segments, on voit un segment s'allumer partout ; si c'est
 * celui de la sélection, on voit UN DIGIT ENTIER s'allumer. Un digit entier
 * qui s'allume seul désigne sans ambiguïté un bit de sélection, et son rang
 * donne la position du digit.
 */

#include <Arduino.h>

/* ---- Mesuré (phases B et C) ---------------------------------------------- */
const uint8_t PIN_DATA  = A5;
const uint8_t PIN_CLOCK = A4;
const uint8_t PIN_LATCH = A3;
const uint8_t PIN_OE    = A2;

/* ---- Mesuré (phase A) ----------------------------------------------------- */
const uint8_t BTN_NEXT = 12;   /* K1 */
const uint8_t BTN_PREV = 10;   /* K2 */
const uint8_t BTN_FILL = 8;    /* K3 */
const uint8_t BTN_AUTO = A0;   /* K4 */

const uint8_t  N_BIT = 16;     /* les deux octets d'afficheur */
const uint16_t AUTO_MS = 1500;
const uint8_t  DEBOUNCE_MS = 30;

uint8_t g_bit = 0;
uint8_t g_fill = 0x00;
bool    g_auto = false;
uint32_t g_autoAt = 0;

uint8_t g_btnState[4];
uint32_t g_btnAt[4];
const uint8_t BTN[4] = { BTN_NEXT, BTN_PREV, BTN_FILL, BTN_AUTO };

/* Ordre d'émission : premier octet, deuxième octet, puis les relais. Les
 * relais restent à zéro : cette phase est muette, et c'est voulu. */
void sendFrame(uint8_t first, uint8_t second) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, first);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, second);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, 0x00);
  digitalWrite(PIN_LATCH, HIGH);
}

void apply() {
  /* g_bit 0..7  -> deuxième octet émis, bits 8..15 de la trame
   * g_bit 8..15 -> premier octet émis,  bits 16..23 de la trame */
  if (g_bit < 8) sendFrame(g_fill, (uint8_t)(1u << g_bit));
  else           sendFrame((uint8_t)(1u << (g_bit - 8)), g_fill);
}

void printState() {
  Serial.print(F("\n[ "));
  if (g_bit + 1 < 10) Serial.print(' ');
  Serial.print(g_bit + 1);
  Serial.print(F("/16 ]  trame bit "));
  Serial.print(8 + g_bit);
  Serial.print(F("  ->  "));
  if (g_bit < 8) {
    Serial.print(F("2e octet emis, bit "));
    Serial.print(g_bit);
  } else {
    Serial.print(F("1er octet emis, bit "));
    Serial.print(g_bit - 8);
  }
  Serial.print(F("   autre octet=0x"));
  if (g_fill < 0x10) Serial.print('0');
  Serial.print(g_fill, HEX);
  Serial.println(g_auto ? F("   AUTO") : F("   manuel"));
  Serial.println(F("      que voit-on ? un segment partout, un digit entier,"));
  Serial.println(F("      le deux-points, ou rien ?"));
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(BTN[i], INPUT_PULLUP);
    g_btnState[i] = HIGH;
    g_btnAt[i] = 0;
  }

  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  digitalWrite(PIN_LATCH, HIGH);
  digitalWrite(PIN_CLOCK, LOW);

  /* BAS avant OUTPUT : OE est active à l'état bas, on la veut au bon niveau
   * avant qu'elle ne devienne une sortie. */
  digitalWrite(PIN_OE, LOW);
  pinMode(PIN_OE, OUTPUT);

  Serial.println(F("\n=== pindisp : phase D, brochage de l'afficheur ==="));
  Serial.println(F("Chaine mesuree : donnees=A5 horloge=A4 verrou=A3 OE=A2."));
  Serial.println(F("Les relais restent a ZERO : cette phase est muette."));
  Serial.println();
  Serial.println(F("  K1 (D12) : bit suivant"));
  Serial.println(F("  K2 (D10) : bit precedent"));
  Serial.println(F("  K3 (D8)  : l'AUTRE octet, 0x00 <-> 0xFF"));
  Serial.println(F("  K4 (A0)  : avance automatique, 1,5 s par bit"));
  Serial.println();
  Serial.println(F("K3 EST LE TEST DECISIF. L'autre octet a 0xFF :"));
  Serial.println(F("  - un SEGMENT s'allume partout -> octet des segments ;"));
  Serial.println(F("  - un DIGIT ENTIER s'allume    -> octet de selection,"));
  Serial.println(F("    et le rang du bit donne la position du digit."));
  Serial.println();
  Serial.println(F("Noter pour chacun des 16 bits ce qui s'allume. C'est"));
  Serial.println(F("fastidieux mais fini : 16 lignes, et l'afficheur est le"));
  Serial.println(F("dernier organe non mesure de la carte.\n"));

  apply();
  printState();
}

void pollButtons() {
  const uint32_t now = millis();
  for (uint8_t i = 0; i < 4; ++i) {
    const uint8_t v = digitalRead(BTN[i]);
    if (v == g_btnState[i]) { g_btnAt[i] = now; continue; }
    if (now - g_btnAt[i] < DEBOUNCE_MS) continue;

    g_btnState[i] = v;
    g_btnAt[i] = now;
    if (v != LOW) continue;

    switch (i) {
      case 0: g_bit = (uint8_t)((g_bit + 1) % N_BIT); break;
      case 1: g_bit = (uint8_t)((g_bit + N_BIT - 1) % N_BIT); break;
      case 2: g_fill = (uint8_t)(g_fill ? 0x00 : 0xFF); break;
      case 3: g_auto = !g_auto; g_autoAt = now; break;
    }
    apply();
    printState();
  }
}

void loop() {
  pollButtons();

  if (!g_auto) return;
  const uint32_t now = millis();
  if (now - g_autoAt < AUTO_MS) return;
  g_autoAt = now;
  g_bit = (uint8_t)((g_bit + 1) % N_BIT);
  apply();
  printState();
}
