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
 * L'ordre n'est pas séquentiel : le relais 8 occupe le bit 0, les relais
 * 1 à 7 les bits 1 à 7. C'est le câblage de la carte, pas une erreur. */
const uint8_t RELAY_BIT[OUT_COUNT] = { 1, 2, 3, 4, 5, 6, 7, 0 };

/* Afficheur à anode commune : un bit à 0 allume le segment.
 * Table issue de la bibliothèque de référence af3556/IO22_IO_Board. */
const uint16_t GLYPH[17] = {
  0x2008, /* 0 */ 0x7A08, /* 1 */ 0xE000, /* 2 */ 0x6200, /* 3 */
  0x3A00, /* 4 */ 0x2210, /* 5 */ 0x2010, /* 6 */ 0x6A08, /* 7 */
  0x2000, /* 8 */ 0x2200, /* 9 */ 0xFA18, /* vide */ 0x2008, /* O */
  0x7810, /* n */ 0xA810, /* F */ 0xA010, /* E */ 0xF810, /* r */
  0xF218  /* _ */
};

/* Bit de sélection du digit, mêlé au motif de segments. */
const uint16_t DIGIT_SELECT[4] = { 0x0400, 0x0002, 0x0004, 0x0020 };

/* Seuls DP2 et DP3 sont câblés, en guise de deux-points. */
const uint16_t DP_SEGMENT = 0xDFFF;

uint16_t g_disp[4];
uint8_t  g_relays = 0;
uint8_t  g_digit = 0;
bool     g_colon = false;
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

/* Reconstruit le tampon d'un digit : segments, sélection, deux-points. */
void rebuildDigit(uint8_t n) {
  uint16_t v = GLYPH[g_glyphs[n]];
  if (n == 1 || n == 2) {
    if (g_colon) {
      v &= DP_SEGMENT;
    } else {
      v |= (uint16_t)~DP_SEGMENT;
    }
  }
  v |= DIGIT_SELECT[n];
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
  g_colon = false;
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

void board::setColon(bool on) {
  if (on == g_colon) return;
  g_colon = on;
  rebuildDigit(1);
  rebuildDigit(2);
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
  shiftByteFast((uint8_t)(d & 0xFF));   /* U4 */
  shiftByteFast((uint8_t)(d >> 8));     /* U3 */
  shiftByteFast(g_relays);              /* U5 */
  SR_LATCH_PORT |= _BV(SR_LATCH_BIT);
  g_digit = (uint8_t)((g_digit + 1) & 3);
}
