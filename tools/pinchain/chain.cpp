/*
 * chain.cpp — Phase B de la découverte de brochage : trouver la chaîne.
 *
 * La phase A (tools/pinfind) a mesuré, sur l'exemplaire :
 *     entrées IN1..IN8 : D2 D3 D4 D5 D6 D7 D9 D11
 *     boutons K1..K4   : D12 D10 D8 A0
 *
 * Il reste donc six broches, et six seulement, pour piloter huit relais et un
 * afficheur quatre digits : D13, A1, A2, A3, A4, A5. Ce croquis les balaie.
 *
 * Pourquoi un balayage plutôt qu'une lecture de documentation : le brochage
 * supposé jusqu'ici venait de la bibliothèque af3556/IO22_IO_Board, qui ne
 * couvre que l'IO22D08. La phase A a montré que la DN22D08 en diffère — sur
 * les boutons comme sur deux des huit entrées. Rien ne permet donc de
 * supposer que les lignes de la chaîne, elles, coïncideraient.
 *
 * Méthode. Pour chaque triplet ORDONNÉ (données, horloge, verrou) pris parmi
 * les six candidates, on décale trois octets de 0xFF puis on verrouille : si
 * le triplet est le bon, les huit relais collent d'un coup — bruit
 * impossible à manquer. Les trois broches restantes sont maintenues à un
 * niveau fixe, pour couvrir une éventuelle validation OE parmi elles ; le
 * balayage est donc fait deux fois, une fois à l'état bas, une fois à l'état
 * haut. 6x5x4 = 120 triplets, 240 essais en tout, environ cinq minutes.
 *
 * Innocuité. Les six broches sont pilotées en sortie, ce qui n'est sans
 * risque que parce que la phase A a prouvé que ce sont les seules qui ne
 * portent ni entrée optocouplée ni poussoir — donc les seules qu'aucun organe
 * de la carte ne cherche à imposer.
 *
 * Les huit relais collent ensemble à chaque essai réussi : environ 300 mA sur
 * le 12 V, ce que la carte encaisse sans difficulté. Mais le faisceau ne doit
 * PAS être câblé : huit circuits fermés en même temps, ce n'est un régime
 * prévu nulle part.
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
const uint8_t N_CAND = sizeof(CAND) / sizeof(CAND[0]);

const uint16_t HOLD_MS = 900;   /* relais collés : le temps d'entendre  */
const uint16_t GAP_MS  = 400;   /* relâchés, pour séparer deux essais   */

uint16_t g_try = 0;

void pulse(uint8_t pin) {
  digitalWrite(pin, HIGH);
  digitalWrite(pin, LOW);
}

void tryCombo(uint8_t di, uint8_t ci, uint8_t li, uint8_t otherLevel) {
  const uint8_t dataPin  = CAND[di].pin;
  const uint8_t clockPin = CAND[ci].pin;
  const uint8_t latchPin = CAND[li].pin;

  for (uint8_t i = 0; i < N_CAND; ++i) {
    pinMode(CAND[i].pin, OUTPUT);
    if (i != di && i != ci && i != li) digitalWrite(CAND[i].pin, otherLevel);
  }
  digitalWrite(clockPin, LOW);

  ++g_try;
  Serial.print(F("[ "));
  if (g_try < 100) Serial.print(' ');
  if (g_try < 10)  Serial.print(' ');
  Serial.print(g_try);
  Serial.print(F("/240 ]  donnees="));
  Serial.print(CAND[di].name);
  Serial.print(F("  horloge="));
  Serial.print(CAND[ci].name);
  Serial.print(F("  verrou="));
  Serial.print(CAND[li].name);
  Serial.print(F("  autres="));
  Serial.println(otherLevel == HIGH ? F("HAUT") : F("BAS"));

  /* Trois octets : deux registres d'afficheur et un de relais, quel que soit
   * leur ordre dans la chaîne. Tout à 1 : quelle que soit la position du
   * registre des relais, il reçoit 0xFF. */
  digitalWrite(latchPin, LOW);
  for (uint8_t b = 0; b < 3; ++b) shiftOut(dataPin, clockPin, MSBFIRST, 0xFF);
  digitalWrite(latchPin, HIGH);
  delay(HOLD_MS);

  digitalWrite(latchPin, LOW);
  for (uint8_t b = 0; b < 3; ++b) shiftOut(dataPin, clockPin, MSBFIRST, 0x00);
  digitalWrite(latchPin, HIGH);
  delay(GAP_MS);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println(F("\n=== pinchain : phase B, decouverte de la chaine ==="));
  Serial.println(F("Six broches restantes apres la phase A :"));
  Serial.println(F("    D13  A1  A2  A3  A4  A5"));
  Serial.println();
  Serial.println(F("240 essais, environ 5 min. A chaque essai, trois octets de"));
  Serial.println(F("0xFF sont decales puis verrouilles. Le bon triplet fait"));
  Serial.println(F("COLLER LES HUIT RELAIS D'UN COUP : impossible a manquer."));
  Serial.println();
  Serial.println(F("!! Le faisceau ne doit PAS etre cable : huit circuits"));
  Serial.println(F("   fermes en meme temps n'est un regime prevu nulle part."));
  Serial.println();
  Serial.println(F("Noter le numero de l'essai qui claque, puis me le donner."));
  Serial.println(F("Surveiller aussi l'afficheur : s'il s'allume sans que les"));
  Serial.println(F("relais bougent, la chaine est trouvee mais les relais ont"));
  Serial.println(F("leur propre validation, et c'est le passage autres=BAS ou"));
  Serial.println(F("autres=HAUT qui tranchera.\n"));
}

void loop() {
  for (uint8_t level = 0; level < 2; ++level) {
    const uint8_t other = level ? HIGH : LOW;
    for (uint8_t di = 0; di < N_CAND; ++di) {
      for (uint8_t ci = 0; ci < N_CAND; ++ci) {
        if (ci == di) continue;
        for (uint8_t li = 0; li < N_CAND; ++li) {
          if (li == di || li == ci) continue;
          tryCombo(di, ci, li, other);
        }
      }
    }
  }

  Serial.println(F("\n=== Balayage termine. Reprise depuis le debut. ===\n"));
  g_try = 0;
}
