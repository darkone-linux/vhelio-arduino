#include "board_io.h"
#include "config.h"

namespace {

/* ---------------------------------------------------------------------
 * Brochage physique. C'EST ICI qu'on corrige si la carte diffère.
 * Procédure de vérification : specs/03-affectation-es.md §6.
 * ------------------------------------------------------------------ */
/* Mesuré avec tools/pinfind, pas déduit : specs/03 §6 bis. */
const uint8_t IN_PIN[IN_COUNT]   = { 2, 3, 4, 5, 6, 7, 9, 11 };
const uint8_t BTN_PIN[BTN_COUNT] = { 12, 10, 8, A0 };

/* Entrée optocouplée NPN : un signal sur la borne fait conduire
 * l'optocoupleur, qui tire la broche du Nano à l'état bas. */
const bool IN_ACTIVE_LOW[IN_COUNT] = {
  true, true, true, true, true, true, true, true
};

/* Position du bit de chaque relais dans l'octet du registre.
 * Mesuré : bit 0 -> CH1, bit 1 -> CH2, ... bit 7 -> CH8. La correspondance
 * est DIRECTE. Le décalage de l'IO22D08, où le relais 8 occupait le bit 0,
 * n'existe pas sur la DN22D08. Le tableau est conservé quand même : il coûte
 * huit octets de flash et garde le reste du firmware indifférent à la
 * question, ce qui est exactement ce qui a permis d'encaisser tous les
 * changements de brochage de cette carte sans toucher un module métier. */
const uint8_t RELAY_BIT[OUT_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7 };

/* ---- Afficheur — MESURÉ (specs/03 §6 ter) --------------------------------
 * Tout ce bloc venait de l'IO22D08. La mesure l'a contredit sur les trois
 * points qui comptent : la polarité des segments, celle de la sélection, et
 * la répartition des bits.
 *
 * Le mot d'afficheur fait seize bits. Les bits 0 à 7 partent dans le DEUXIÈME
 * octet émis, les bits 8 à 15 dans le PREMIER. Les segments sont répartis sur
 * les deux — six d'un côté, deux de l'autre — ce qui interdit de raisonner
 * « un octet segments, un octet digits ».
 *
 *   SEGMENTS : ACTIFS À L'ÉTAT HAUT. Un bit à 1 allume.
 *   SÉLECTION : ACTIVE À L'ÉTAT BAS. Un bit à 0 valide le digit.
 *
 * La sélection à l'état bas explique ce qui déroutait pendant la mesure : mot
 * à zéro, les quatre digits sont validés, et un segment isolé s'allume donc
 * sur les quatre à la fois.                                                */
#define SEG_A   (1u <<  4)
#define SEG_B   (1u << 12)
#define SEG_C   (1u <<  7)
#define SEG_D   (1u <<  3)
#define SEG_E   (1u <<  1)
#define SEG_F   (1u <<  6)
#define SEG_G   (1u << 11)
#define SEG_DP  (1u <<  5)

const uint16_t GLYPH[24] = {
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
  SEG_D,                                      /* _     */
  SEG_G,                                      /* -     */
  SEG_A|SEG_B|SEG_E|SEG_F|SEG_G,              /* P     */
  SEG_C|SEG_E|SEG_F|SEG_G,                    /* h     */
  SEG_B|SEG_C|SEG_D|SEG_E|SEG_F,              /* U     */
  SEG_A|SEG_D|SEG_E|SEG_F,                    /* C     */
  SEG_D|SEG_E|SEG_F,                          /* L     */
  SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G|SEG_DP  /* test */
};

/* Sélection, active à l'état bas : on part de DIGIT_ALL — les quatre digits
 * inhibés — et on efface le bit du digit à allumer. */
const uint16_t DIGIT_SELECT[4] = { 1u << 2, 1u << 9, 1u << 10, 1u << 13 };
const uint16_t DIGIT_ALL = (1u << 2) | (1u << 9) | (1u << 10) | (1u << 13);

/* L'afficheur n'a PAS de deux-points : rien que des points décimaux, un par
 * digit. Le battement de cœur se porte donc sur le point du digit 1, à
 * gauche — celui qu'aucun format numérique ne réclamera : un point après le
 * chiffre des milliers ne veut rien dire, alors que les formats usuels
 * (12.5, 1.234) le placent après le deuxième ou le troisième.             */
const uint8_t HEARTBEAT_DIGIT = 0;

uint16_t g_disp[4];
uint8_t  g_relays = 0;
uint8_t  g_digit = 0;
bool     g_heartbeat = false;
uint8_t  g_glyphs[4] = { board::GL_BLANK, board::GL_BLANK,
                         board::GL_BLANK, board::GL_BLANK };

/* shiftOut() de la bibliothèque Arduino coûte ~5 µs par bit : 120 µs par
 * digit, appelé à chaque tour de boucle, ce serait le poste de calcul
 * dominant du firmware. En accès direct au port, on tombe à ~8 µs. */
inline void shiftByteFast(uint8_t v) {
  for (uint8_t i = 0; i < 8; ++i) {
    if (v & 0x80) {
      SR_DATA_PORT |= _BV(SR_DATA_BIT);
    } else {
      SR_DATA_PORT &= (uint8_t)~_BV(SR_DATA_BIT);
    }
    SR_CLOCK_PORT |= _BV(SR_CLOCK_BIT);
    SR_CLOCK_PORT &= (uint8_t)~_BV(SR_CLOCK_BIT);
    v <<= 1;
  }
}

/* Reconstruit le tampon d'un digit : segments, point, sélection. */
void rebuildDigit(uint8_t n) {
  uint16_t v = GLYPH[g_glyphs[n]];
  if (g_heartbeat && n == HEARTBEAT_DIGIT) v |= SEG_DP;
  /* Tous les digits inhibés sauf celui-ci : la sélection est active bas. */
  v |= DIGIT_ALL;
  v &= (uint16_t)~DIGIT_SELECT[n];
  g_disp[n] = v;
}

void rebuildAll() {
  for (uint8_t n = 0; n < 4; ++n) rebuildDigit(n);
}

}  // namespace

void board::begin() {
  pinMode(PIN_SR_LATCH, OUTPUT);
  pinMode(PIN_SR_CLOCK, OUTPUT);
  pinMode(PIN_SR_DATA, OUTPUT);
  digitalWrite(PIN_SR_LATCH, HIGH);
  digitalWrite(PIN_SR_CLOCK, LOW);

  /* Ordre volontaire : on écrit HIGH AVANT de passer la broche en sortie.
   * Sur une entrée, digitalWrite(HIGH) active le tirage interne ; la broche
   * est donc déjà haute quand elle devient une sortie. L'inverse
   * produirait une impulsion basse — donc tous les relais collés — pendant
   * quelques microsecondes au démarrage. */
  digitalWrite(PIN_RELAY_OE, HIGH);   /* OE actif bas : haut = relais coupés */
  pinMode(PIN_RELAY_OE, OUTPUT);

  for (uint8_t i = 0; i < IN_COUNT; ++i) pinMode(IN_PIN[i], INPUT_PULLUP);
  for (uint8_t i = 0; i < BTN_COUNT; ++i) pinMode(BTN_PIN[i], INPUT_PULLUP);

  g_relays = 0;
  g_heartbeat = false;
  rebuildAll();

  /* Purger la chaîne avant de valider les sorties, pour ne pas activer des
   * relais au hasard sur le contenu résiduel des registres. */
  for (uint8_t i = 0; i < 4; ++i) refresh();
  outputsEnabled(true);
}

bool board::readInputRaw(uint8_t idx) {
  const bool high = (digitalRead(IN_PIN[idx]) == HIGH);
  return IN_ACTIVE_LOW[idx] ? !high : high;
}

bool board::readButton(uint8_t idx) {
  return digitalRead(BTN_PIN[idx]) == LOW;
}

void board::setOutput(uint8_t idx, bool on) {
  const uint8_t bit = _BV(RELAY_BIT[idx]);
  if (on) {
    g_relays |= bit;
  } else {
    g_relays &= (uint8_t)~bit;
  }
}

bool board::outputState(uint8_t idx) {
  return (g_relays & _BV(RELAY_BIT[idx])) != 0;
}

void board::allOff() {
  g_relays = 0;
}

void board::outputsEnabled(bool en) {
  digitalWrite(PIN_RELAY_OE, en ? LOW : HIGH);
}

void board::setHeartbeat(bool on) {
  if (on == g_heartbeat) return;
  g_heartbeat = on;
  rebuildDigit(HEARTBEAT_DIGIT);
}

void board::showGlyphs(const uint8_t g[4]) {
  for (uint8_t n = 0; n < 4; ++n) {
    if (g[n] != g_glyphs[n]) {
      g_glyphs[n] = g[n];
      rebuildDigit(n);
    }
  }
}

void board::showNumber(uint16_t n, bool blankLeadingZeros) {
  uint8_t g[4];
  for (uint8_t i = 4; i-- > 0;) {
    g[i] = (uint8_t)(n % 10);
    n /= 10;
  }
  if (blankLeadingZeros) {
    for (uint8_t i = 0; i < 3 && g[i] == 0; ++i) g[i] = GL_BLANK;
  }
  showGlyphs(g);
}

void board::refresh() {
  const uint16_t d = g_disp[g_digit];
  SR_LATCH_PORT &= (uint8_t)~_BV(SR_LATCH_BIT);
  /* L'ordre est celui de la mesure : bits 8..15 du mot d'abord, bits 0..7
   * ensuite, les relais en dernier. */
  shiftByteFast((uint8_t)(d >> 8));
  shiftByteFast((uint8_t)(d & 0xFF));
  shiftByteFast(g_relays);
  SR_LATCH_PORT |= _BV(SR_LATCH_BIT);
  g_digit = (uint8_t)((g_digit + 1) & 3);
}
