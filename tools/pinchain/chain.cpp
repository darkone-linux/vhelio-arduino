/*
 * chain.cpp — Phase B de la découverte de brochage : trouver la chaîne.
 *
 * La phase A (tools/pinfind) a mesuré, sur l'exemplaire :
 *     entrées IN1..IN8 : D2 D3 D4 D5 D6 D7 D9 D11
 *     boutons K1..K4   : D12 D10 D8 A0
 *
 * Il reste six broches, et six seulement, pour huit relais et un afficheur
 * quatre digits : D13, A1, A2, A3, A4, A5.
 *
 * BALAYAGE MANUEL, ET C'EST TOUT L'INTÉRÊT. Une première version enchaînait
 * les 240 essais toute seule, à la seconde : inutilisable. Plusieurs triplets
 * produisent un effet partiel — décaler des bits par une horloge et des
 * données interverties fait quand même bouger quelque chose — si bien que la
 * console défile trop vite pour qu'on note quoi que ce soit.
 *
 * Les boutons de la carte, que la phase A vient d'identifier, règlent le
 * problème : on avance d'un triplet par appui, le montage reste dans l'état
 * choisi indéfiniment, et l'on lit la console à tête reposée.
 *
 *     K1 (D12) : triplet suivant
 *     K2 (D10) : triplet précédent
 *     K3 (D8)  : bascule le niveau des trois autres broches (BAS <-> HAUT),
 *                pour couvrir une validation OE parmi elles
 *     K4 (A0)  : bascule le motif (voir ci-dessous)
 *
 * DEUX MOTIFS, et le second est le juge. « TOUS » alterne 0xFF et 0x00 : fort,
 * repérable de loin, bon pour dégrossir. Mais il ne prouve rien, puisqu'un
 * triplet faux fait souvent claquer quelque chose. « BIT A BIT » promène un
 * seul 1 à travers les 24 bits : sur le BON triplet, et sur lui seul, on
 * entend exactement un relais à la fois, proprement, huit fois sur vingt-
 * quatre positions. C'est le critère qui distingue le triplet correct des
 * triplets qui font seulement du bruit.
 *
 * Et il donne gratuitement RELAY_BIT[] : la position annoncée quand tel
 * relais claque EST son bit dans la chaîne.
 *
 * Innocuité. Les six broches sont pilotées en sortie, ce qui n'est sans
 * risque que parce que la phase A a prouvé que ce sont les seules qui ne
 * portent ni entrée optocouplée ni poussoir — donc les seules qu'aucun organe
 * de la carte ne cherche à imposer.
 *
 * Le faisceau ne doit PAS être câblé : en motif « TOUS », huit circuits sont
 * fermés en même temps, ce qui n'est un régime prévu nulle part.
 */

#include <Arduino.h>

struct Candidate {
  uint8_t pin;
  const char* name;
};

/* Les six broches que la phase A n'a pas attribuées. */
const Candidate CAND[] = {
  { 13, "D13" }, { A1, "A1" }, { A2, "A2" },
  { A3, "A3" },  { A4, "A4" }, { A5, "A5" },
};
const uint8_t N_CAND = 6;
const uint8_t N_COMBO = 120;          /* 6 x 5 x 4 triplets ordonnés */

const uint8_t BTN_NEXT  = 12;         /* K1 */
const uint8_t BTN_PREV  = 10;         /* K2 */
const uint8_t BTN_LEVEL = 8;          /* K3 */
const uint8_t BTN_MODE  = A0;         /* K4 */

const uint16_t ALL_MS  = 800;         /* motif TOUS : demi-période      */
const uint16_t WALK_MS = 700;         /* motif BIT A BIT : par position */
const uint8_t  DEBOUNCE_MS = 30;

uint8_t  g_combo = 0;
uint8_t  g_other = LOW;
bool     g_walk = false;

uint8_t  g_bit = 0;                   /* position courante en BIT A BIT */
bool     g_on = false;                /* état courant en TOUS           */
uint32_t g_stepAt = 0;

uint8_t g_btnState[4];
uint32_t g_btnAt[4];
const uint8_t BTN[4] = { BTN_NEXT, BTN_PREV, BTN_LEVEL, BTN_MODE };

/* Décode un numéro de triplet en (données, horloge, verrou). Fait par
 * énumération plutôt que par arithmétique : c'est appelé une fois par appui
 * de bouton, et une boucle lisible vaut mieux qu'un calcul d'indices à
 * relire trois fois. */
void decode(uint8_t combo, uint8_t* di, uint8_t* ci, uint8_t* li) {
  uint8_t n = 0;
  for (uint8_t d = 0; d < N_CAND; ++d)
    for (uint8_t c = 0; c < N_CAND; ++c) {
      if (c == d) continue;
      for (uint8_t l = 0; l < N_CAND; ++l) {
        if (l == d || l == c) continue;
        if (n++ == combo) { *di = d; *ci = c; *li = l; return; }
      }
    }
  *di = 0; *ci = 1; *li = 2;
}

void applyPins() {
  uint8_t di, ci, li;
  decode(g_combo, &di, &ci, &li);
  for (uint8_t i = 0; i < N_CAND; ++i) {
    pinMode(CAND[i].pin, OUTPUT);
    if (i != di && i != ci && i != li) digitalWrite(CAND[i].pin, g_other);
  }
  digitalWrite(CAND[ci].pin, LOW);
}

/* Envoie trois octets puis verrouille. La chaîne compte trois registres —
 * deux d'afficheur, un de relais — quel que soit leur ordre. */
void sendFrame(uint32_t bits24) {
  uint8_t di, ci, li;
  decode(g_combo, &di, &ci, &li);
  const uint8_t dataPin  = CAND[di].pin;
  const uint8_t clockPin = CAND[ci].pin;
  const uint8_t latchPin = CAND[li].pin;

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, (uint8_t)(bits24 >> 16));
  shiftOut(dataPin, clockPin, MSBFIRST, (uint8_t)(bits24 >> 8));
  shiftOut(dataPin, clockPin, MSBFIRST, (uint8_t)(bits24));
  digitalWrite(latchPin, HIGH);
}

void printState() {
  uint8_t di, ci, li;
  decode(g_combo, &di, &ci, &li);

  Serial.print(F("\n[ "));
  if (g_combo + 1 < 100) Serial.print(' ');
  if (g_combo + 1 < 10)  Serial.print(' ');
  Serial.print(g_combo + 1);
  Serial.print(F("/120 ]  donnees="));
  Serial.print(CAND[di].name);
  Serial.print(F("  horloge="));
  Serial.print(CAND[ci].name);
  Serial.print(F("  verrou="));
  Serial.print(CAND[li].name);
  Serial.print(F("  autres="));
  Serial.print(g_other == HIGH ? F("HAUT") : F("BAS"));
  Serial.print(F("  motif="));
  Serial.println(g_walk ? F("BIT A BIT") : F("TOUS"));
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(BTN[i], INPUT_PULLUP);
    g_btnState[i] = HIGH;
    g_btnAt[i] = 0;
  }
  applyPins();

  Serial.println(F("\n=== pinchain : phase B, decouverte de la chaine ==="));
  Serial.println(F("Six broches restantes : D13 A1 A2 A3 A4 A5."));
  Serial.println(F("120 triplets ordonnes. On avance A LA MAIN, avec les"));
  Serial.println(F("boutons de la carte : rien ne defile, rien a noter au vol."));
  Serial.println();
  Serial.println(F("  K1 (D12) : triplet suivant"));
  Serial.println(F("  K2 (D10) : triplet precedent"));
  Serial.println(F("  K3 (D8)  : niveau des autres broches, BAS <-> HAUT"));
  Serial.println(F("  K4 (A0)  : motif, TOUS <-> BIT A BIT"));
  Serial.println();
  Serial.println(F("Motif TOUS : alterne 0xFF et 0x00. Fort, bon pour"));
  Serial.println(F("degrossir, mais ne PROUVE rien : un triplet faux fait"));
  Serial.println(F("souvent claquer quelque chose."));
  Serial.println();
  Serial.println(F("Motif BIT A BIT : promene un seul 1 sur 24 positions. Sur"));
  Serial.println(F("le BON triplet, et sur lui seul, on entend UN SEUL relais"));
  Serial.println(F("a la fois, huit fois sur vingt-quatre. C'est le juge."));
  Serial.println(F("La position annoncee quand un relais claque EST son bit."));
  Serial.println();
  Serial.println(F("!! Faisceau NON cable : en motif TOUS, huit circuits sont"));
  Serial.println(F("   fermes en meme temps."));
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
    if (v != LOW) continue;           /* n'agir qu'à l'appui */

    switch (i) {
      case 0: g_combo = (uint8_t)((g_combo + 1) % N_COMBO); break;
      case 1: g_combo = (uint8_t)((g_combo + N_COMBO - 1) % N_COMBO); break;
      case 2: g_other = (g_other == LOW) ? HIGH : LOW; break;
      case 3: g_walk = !g_walk; break;
    }
    g_bit = 0;
    g_on = false;
    applyPins();
    printState();
  }
}

void loop() {
  pollButtons();

  const uint32_t now = millis();
  const uint16_t step = g_walk ? WALK_MS : ALL_MS;
  if (now - g_stepAt < step) return;
  g_stepAt = now;

  if (g_walk) {
    sendFrame(1UL << g_bit);
    Serial.print(F("    bit "));
    if (g_bit < 10) Serial.print('0');
    Serial.println(g_bit);
    g_bit = (uint8_t)((g_bit + 1) % 24);
  } else {
    g_on = !g_on;
    sendFrame(g_on ? 0x00FFFFFFUL : 0UL);
  }
}
