/*
 * check.cpp — L'afficheur, en conditions réelles.
 *
 * Les phases A à D ont mesuré le brochage bit par bit. Ce croquis vérifie que
 * l'ASSEMBLAGE de ces mesures produit quelque chose de lisible — ce qui n'est
 * pas la même chose. Un relevé bit à bit peut être juste et une table de
 * glyphes fausse quand même : il suffit d'un segment mal nommé à l'œil, et le
 * 6 devient un 5 sans que rien ne le signale.
 *
 * Il reprend donc les tables du firmware À L'IDENTIQUE, mais sans dépendre de
 * lui : ce croquis doit rester utilisable si le firmware est en refonte. Toute
 * divergence entre ce fichier et board_io.cpp est un bug de l'un des deux.
 *
 *   K1 (D12) : mode — compteur, défilé des glyphes, tout allumé
 *   K2 (D10) : vitesse du compteur — 10 ms, 100 ms, 1 ms
 *   K3 (D8)  : zéros de tête, affichés ou masqués
 *   K4 (A0)  : battement de cœur, marche/arrêt
 *
 * TROIS MODES, parce qu'ils ne prouvent pas la même chose.
 *
 * COMPTEUR : les dix chiffres passent dans les quatre positions. C'est le
 * test des glyphes ET du multiplexage — un digit mal sélectionné se voit
 * immédiatement comme un chiffre fantôme sur la mauvaise position.
 *
 * DÉFILÉ : les dix chiffres puis les glyphes de service (O n F E r _), les
 * quatre digits ensemble. Les glyphes de service ne servent qu'aux codes de
 * défaut, donc n'apparaîtraient jamais dans un compteur — et un code de
 * défaut illisible est pire qu'inutile.
 *
 * TOUT ALLUMÉ : les huit segments des quatre digits, points compris. Un
 * segment mort ou un digit mort se voit là, et nulle part ailleurs.
 *
 * Les relais restent à ZÉRO : ce croquis est muet.
 */

#include <Arduino.h>

/* ---- Chaîne, mesurée (phases B et C) ------------------------------------- */
const uint8_t PIN_DATA  = A5;
const uint8_t PIN_CLOCK = A4;
const uint8_t PIN_LATCH = A3;
const uint8_t PIN_OE    = A2;

/* ---- Boutons, mesurés (phase A) ------------------------------------------ */
const uint8_t BTN[4] = { 12, 10, 8, A0 };

/* ---- Afficheur, mesuré (phase D) ----------------------------------------- *
 * Segments ACTIFS À L'ÉTAT HAUT, sélection ACTIVE À L'ÉTAT BAS.
 * Bits 0..7 du mot -> deuxième octet émis ; bits 8..15 -> premier.          */
#define SEG_A   (1u <<  4)
#define SEG_B   (1u << 12)
#define SEG_C   (1u <<  7)
#define SEG_D   (1u <<  3)
#define SEG_E   (1u <<  1)
#define SEG_F   (1u <<  6)
#define SEG_G   (1u << 11)
#define SEG_DP  (1u <<  5)

const uint16_t GLYPH[17] = {
  SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,        /* 0     */
  SEG_B|SEG_C,                                /* 1     */
  SEG_A|SEG_B|SEG_G|SEG_E|SEG_D,              /* 2     */
  SEG_A|SEG_B|SEG_G|SEG_C|SEG_D,              /* 3     */
  SEG_F|SEG_G|SEG_B|SEG_C,                    /* 4     */
  SEG_A|SEG_F|SEG_G|SEG_C|SEG_D,              /* 5     */
  SEG_A|SEG_F|SEG_G|SEG_E|SEG_D|SEG_C,        /* 6     */
  SEG_A|SEG_B|SEG_C,                          /* 7     */
  SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G,  /* 8     */
  SEG_A|SEG_B|SEG_C|SEG_D|SEG_F|SEG_G,        /* 9     */
  0,                                          /* vide  */
  SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,        /* O     */
  SEG_E|SEG_G|SEG_C,                          /* n     */
  SEG_A|SEG_F|SEG_G|SEG_E,                    /* F     */
  SEG_A|SEG_F|SEG_G|SEG_E|SEG_D,              /* E     */
  SEG_E|SEG_G,                                /* r     */
  SEG_D                                       /* _     */
};
const uint8_t GL_BLANK = 10;
const uint8_t N_GLYPH = 17;

const uint16_t DIGIT_SELECT[4] = { 1u << 2, 1u << 9, 1u << 10, 1u << 13 };
const uint16_t DIGIT_ALL = (1u << 2) | (1u << 9) | (1u << 10) | (1u << 13);

/* Le battement de cœur est le point du digit de gauche : l'afficheur n'a pas
 * de deux-points, et aucun format numérique ne réclame un point après le
 * chiffre des milliers. */
const uint8_t HEARTBEAT_DIGIT = 0;

const uint16_t SPEED_MS[3] = { 10, 100, 1 };
const uint16_t BEAT_MS = 500;
const uint8_t  DEBOUNCE_MS = 30;

uint8_t  g_mode = 0;          /* 0 compteur, 1 défilé, 2 tout allumé */
uint8_t  g_speed = 0;
bool     g_blank = false;     /* masquer les zéros de tête */
bool     g_beatOn = true;

uint16_t g_count = 0;
uint8_t  g_parade = 0;
uint8_t  g_digit = 0;
bool     g_beat = false;

uint32_t g_countAt = 0, g_beatAt = 0, g_logAt = 0;
uint8_t  g_btnState[4];
uint32_t g_btnAt[4];

void sendWord(uint16_t word) {
  digitalWrite(PIN_LATCH, LOW);
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, (uint8_t)(word >> 8));
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, (uint8_t)(word & 0xFF));
  shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, 0x00);   /* relais : muet */
  digitalWrite(PIN_LATCH, HIGH);
}

/* Ce que chaque digit doit montrer, selon le mode. */
uint8_t glyphFor(uint8_t n) {
  if (g_mode == 2) return 8;
  if (g_mode == 1) return g_parade;

  uint16_t v = g_count;
  uint8_t g[4];
  for (uint8_t i = 4; i-- > 0;) { g[i] = (uint8_t)(v % 10); v /= 10; }
  if (g_blank) {
    for (uint8_t i = 0; i < 3 && g[i] == 0; ++i) g[i] = GL_BLANK;
  }
  return g[n];
}

void refresh() {
  uint16_t w = GLYPH[glyphFor(g_digit)];
  if (g_mode == 2) w |= SEG_DP;
  else if (g_beat && g_beatOn && g_digit == HEARTBEAT_DIGIT) w |= SEG_DP;

  w |= DIGIT_ALL;
  w &= (uint16_t)~DIGIT_SELECT[g_digit];
  sendWord(w);
  g_digit = (uint8_t)((g_digit + 1) & 3);
}

void printState() {
  Serial.print(F("\nmode="));
  if (g_mode == 0)      Serial.print(F("COMPTEUR"));
  else if (g_mode == 1) Serial.print(F("DEFILE"));
  else                  Serial.print(F("TOUT ALLUME"));
  Serial.print(F("   vitesse="));
  Serial.print(SPEED_MS[g_speed]);
  Serial.print(F(" ms   zeros de tete="));
  Serial.print(g_blank ? F("masques") : F("affiches"));
  Serial.print(F("   battement="));
  Serial.println(g_beatOn ? F("marche") : F("arret"));
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

  /* BAS avant OUTPUT : OE est active à l'état bas. */
  digitalWrite(PIN_OE, LOW);
  pinMode(PIN_OE, OUTPUT);

  Serial.println(F("\n=== dispcheck : l'afficheur en conditions reelles ==="));
  Serial.println(F("Les relais restent a zero : ce croquis est muet."));
  Serial.println();
  Serial.println(F("  K1 (D12) : mode -- compteur / defile / tout allume"));
  Serial.println(F("  K2 (D10) : vitesse -- 10 ms, 100 ms, 1 ms"));
  Serial.println(F("  K3 (D8)  : zeros de tete, affiches ou masques"));
  Serial.println(F("  K4 (A0)  : battement de coeur, marche/arret"));
  Serial.println();
  Serial.println(F("COMPTEUR : les dix chiffres dans les quatre positions."));
  Serial.println(F("  Un digit mal selectionne se voit tout de suite : le"));
  Serial.println(F("  chiffre apparait sur la mauvaise position."));
  Serial.println(F("DEFILE : 0..9 puis les glyphes de service O n F E r _,"));
  Serial.println(F("  qui ne servent qu'aux codes de defaut et n'apparaitraient"));
  Serial.println(F("  jamais dans un compteur."));
  Serial.println(F("TOUT ALLUME : huit segments sur quatre digits, points"));
  Serial.println(F("  compris. Un segment mort ne se voit que la."));
  Serial.println();
  Serial.println(F("Le point du digit de GAUCHE bat a 1 Hz : c'est le"));
  Serial.println(F("battement de coeur. L'afficheur n'a pas de deux-points."));
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
      case 0: g_mode = (uint8_t)((g_mode + 1) % 3); g_parade = 0; break;
      case 1: g_speed = (uint8_t)((g_speed + 1) % 3); break;
      case 2: g_blank = !g_blank; break;
      case 3: g_beatOn = !g_beatOn; break;
    }
    printState();
  }
}

void loop() {
  refresh();          /* un digit par tour : le multiplexage vit ici */
  pollButtons();

  const uint32_t now = millis();

  if (now - g_beatAt >= BEAT_MS) {
    g_beatAt = now;
    g_beat = !g_beat;
  }

  if (g_mode == 0 && now - g_countAt >= SPEED_MS[g_speed]) {
    g_countAt = now;
    g_count = (uint16_t)((g_count + 1) % 10000);
  } else if (g_mode == 1 && now - g_countAt >= 700) {
    g_countAt = now;
    g_parade = (uint8_t)((g_parade + 1) % N_GLYPH);
  }

  /* Journal rare : chaque Serial.print bloque le multiplexage. */
  if (now - g_logAt >= 2000) {
    g_logAt = now;
    if (g_mode == 0) {
      Serial.print(F("    compteur = "));
      Serial.println(g_count);
    } else if (g_mode == 1) {
      Serial.print(F("    glyphe = "));
      Serial.println(g_parade);
    }
  }
}
