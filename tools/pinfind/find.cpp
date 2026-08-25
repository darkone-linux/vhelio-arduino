/*
 * find.cpp — Phase A de la découverte de brochage : trouver les entrées.
 *
 * pinscan VÉRIFIE un brochage supposé ; pinfind le DÉCOUVRE. La distinction
 * compte : le brochage de pins.h vient de la bibliothèque af3556/IO22_IO_Board,
 * qui ne couvre que l'IO22D08 et l'IO22C04. La DN22D08 est un produit voisin
 * mais distinct — rail DIN, RS485, 12/24 V — et rien ne garantit qu'elle
 * partage le brochage de l'IO22D08. Quand pinscan ne montre ni bouton ni
 * afficheur ni relais alors que le Nano est bien enfiché et la carte
 * alimentée, c'est l'hypothèse qui reste.
 *
 * Ce croquis ne suppose donc RIEN. Il met toutes les broches utilisables en
 * INPUT_PULLUP et signale chaque changement d'état. On appuie sur un bouton,
 * on relie une borne d'entrée à la masse, et la console dit quelle broche a
 * bougé. C'est tout, et c'est suffisant pour reconstruire IN_PIN[] et
 * BTN_PIN[] sans documentation.
 *
 * INPUT_PULLUP sur tout : aucune broche n'est pilotée en sortie, donc aucun
 * conflit possible avec ce que la carte pourrait imposer. Ce croquis ne peut
 * rien abîmer, quelle que soit la carte.
 *
 * D0 et D1 sont exclues : c'est la console série.
 * A6 et A7 sont exclues : sur un Nano elles sont analogiques seules et
 * n'ont ni entrée numérique ni tirage interne.
 *
 * D13 lit `0` en permanence, et ce n'est PAS un signal de la carte : la LED
 * intégrée du Nano et sa résistance série chargent le tirage interne, qui ne
 * fait que ~30 kΩ. C'est vrai sur n'importe quel Arduino, carte ou pas. La
 * broche reste utilisable en sortie, donc candidate pour la chaîne.
 */

#include <Arduino.h>

struct Candidate {
  uint8_t pin;
  const char* name;
};

/* Toutes les broches d'un Nano capables d'entrée numérique avec tirage. */
const Candidate CAND[] = {
  {  2, "D2"  }, {  3, "D3"  }, {  4, "D4"  }, {  5, "D5"  },
  {  6, "D6"  }, {  7, "D7"  }, {  8, "D8"  }, {  9, "D9"  },
  { 10, "D10" }, { 11, "D11" }, { 12, "D12" }, { 13, "D13" },
  { A0, "A0"  }, { A1, "A1"  }, { A2, "A2"  }, { A3, "A3"  },
  { A4, "A4"  }, { A5, "A5"  },
};
const uint8_t N_CAND = sizeof(CAND) / sizeof(CAND[0]);

/* Anti-rebond : un contact mécanique rebondit sur quelques millisecondes, et
 * sans filtrage la console serait noyée sous les transitions parasites. */
const uint8_t  DEBOUNCE_MS = 25;
const uint16_t MAP_MS = 3000;

uint8_t  g_state[N_CAND];      /* dernier état stable    */
uint8_t  g_raw[N_CAND];        /* dernier état lu        */
uint32_t g_changedAt[N_CAND];  /* date du dernier écart  */
uint32_t g_mapAt = 0;

void printMap() {
  Serial.print(F("    "));
  for (uint8_t i = 0; i < N_CAND; ++i) {
    Serial.print(CAND[i].name);
    Serial.print('=');
    Serial.print(g_state[i] ? '1' : '0');
    Serial.print(' ');
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < N_CAND; ++i) {
    pinMode(CAND[i].pin, INPUT_PULLUP);
    g_state[i] = HIGH;
    g_raw[i] = HIGH;
    g_changedAt[i] = 0;
  }
  /* Laisser les tirages internes remonter avant la première lecture. */
  delay(50);
  for (uint8_t i = 0; i < N_CAND; ++i) {
    g_state[i] = digitalRead(CAND[i].pin);
    g_raw[i] = g_state[i];
  }

  Serial.println(F("\n=== pinfind : phase A, decouverte des entrees ==="));
  Serial.println(F("Aucune hypothese de brochage. Toutes les broches sont en"));
  Serial.println(F("INPUT_PULLUP et rien n'est pilote en sortie : ce croquis ne"));
  Serial.println(F("peut rien abimer, quelle que soit la carte."));
  Serial.println();
  Serial.println(F("1. Appuyer sur les poussoirs de la carte, un par un."));
  Serial.println(F("   -> donne BTN_PIN[]. Ne demande pas le 12 V."));
  Serial.println(F("2. Carte ALIMENTEE EN 12 V, relier chaque borne d'entree a"));
  Serial.println(F("   la masse, une par une.  -> donne IN_PIN[]."));
  Serial.println(F("   Si rien ne bouge, reessayer avec du +12 V : la carte"));
  Serial.println(F("   serait alors PNP."));
  Serial.println();
  Serial.println(F("Noter, pour chaque organe, la broche annoncee. Les broches"));
  Serial.println(F("qui ne bougent JAMAIS sont les candidates de la chaine de"));
  Serial.println(F("registres : c'est la phase B (specs/03 §6 bis)."));
  Serial.println(F("D13 lit 0 en permanence : c'est la LED integree du Nano"));
  Serial.println(F("qui charge le tirage interne, pas un signal de la carte."));
  Serial.println(F("Etat courant toutes les 3 s, changements en direct.\n"));
  printMap();
}

void loop() {
  const uint32_t now = millis();

  for (uint8_t i = 0; i < N_CAND; ++i) {
    const uint8_t v = digitalRead(CAND[i].pin);

    if (v != g_raw[i]) {
      g_raw[i] = v;
      g_changedAt[i] = now;
      continue;
    }
    if (v == g_state[i]) continue;
    if (now - g_changedAt[i] < DEBOUNCE_MS) continue;

    g_state[i] = v;
    Serial.print(F(">>> "));
    Serial.print(CAND[i].name);
    Serial.println(v == LOW ? F("  ACTIVE   (1 -> 0)") : F("  relache  (0 -> 1)"));
    g_mapAt = now;   /* repousser la carte d'etat, pour ne pas la melanger */
  }

  if (now - g_mapAt >= MAP_MS) {
    g_mapAt = now;
    printMap();
  }
}
