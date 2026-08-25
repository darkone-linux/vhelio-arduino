/*
 * board_io.h — Couche d'abstraction matérielle de la carte DN22D08.
 *
 * Elle masque aux modules métier trois particularités lourdes de cette carte :
 *   - les relais ne sont pas des broches, mais des bits d'un registre à
 *     décalage chaîné avec l'afficheur ;
 *   - l'afficheur est multiplexé et doit être rafraîchi en permanence ;
 *   - les entrées optocouplées sont actives à l'état bas.
 *
 * Un module métier écrit setOutput(OUT_TAIL_STOP, true) et ne sait rien de tout ça.
 */
#pragma once

#include <Arduino.h>
#include "pins.h"

namespace board {

/* Glyphes disponibles sur l'afficheur 4 digits.
 *
 * Les lettres sont celles que sept segments savent rendre sans ambiguïté. Une
 * substitution d'usage : `U` tient lieu de V, impossible à dessiner. Le `l`
 * minuscule est proscrit — une barre verticale seule ne se distingue pas d'un
 * `1` — d'où `G` et `d` pour gauche et droite.
 *
 * Plusieurs de ces lettres ne diffèrent d'un chiffre que par UN segment : `U`
 * et `0` par celui du haut, `G` et `0` par celui d'en haut à droite, `A` et `8`
 * par celui du bas. Un segment mort ne rendrait donc pas l'afficheur illisible,
 * il le rendrait MENTEUR — c'est ce que GL_TEST attrape au démarrage.
 *
 * GL_TEST allume TOUT, point compris : c'est le test de l'afficheur au
 * démarrage, seul moyen de découvrir un segment mort. */
enum Glyph : uint8_t {
  GL_0 = 0, GL_1, GL_2, GL_3, GL_4, GL_5, GL_6, GL_7, GL_8, GL_9,
  GL_BLANK = 10, GL_O = 11, GL_n = 12, GL_F = 13, GL_E = 14, GL_r = 15,
  GL_UNDER = 16, GL_DASH = 17, GL_P = 18, GL_h = 19, GL_U = 20,
  GL_C = 21, GL_L = 22, GL_A = 23, GL_G = 24, GL_d = 25,
  GL_TEST = 26
};

void begin();

/* --- Entrées ----------------------------------------------------------- */

/* État LOGIQUE de l'entrée (true = signal présent sur la borne), quelle que
 * soit la polarité électrique. Aucun anti-rebond : c'est le rôle d'inputs. */
bool readInputRaw(uint8_t idx);

/* Boutons de la carte, true = appuyé. */
bool readButton(uint8_t idx);

/* --- Relais ------------------------------------------------------------ */

/* Prend effet au prochain refresh(), soit moins d'un tour de boucle. */
void setOutput(uint8_t idx, bool on);
bool outputState(uint8_t idx);
void allOff();

/* Validation globale des relais (broche OE du registre).
 * outputsEnabled(false) coupe TOUS les relais en un cycle d'horloge, sans
 * toucher au registre : c'est l'arrêt d'urgence, et l'état antérieur est
 * restitué tel quel à la réactivation. */
void outputsEnabled(bool en);

/* --- Afficheur --------------------------------------------------------- */

void showNumber(uint16_t n, bool blankLeadingZeros = true);
void showGlyphs(const uint8_t g[4]);
/* L'afficheur n'a pas de deux-points : le battement de cœur est le point
 * décimal du digit de gauche. Voir board_io.cpp. */
void setHeartbeat(bool on);

/* --- Rafraîchissement -------------------------------------------------- */

/* Émet un digit et l'octet des relais sur la chaîne de registres.
 * À appeler à CHAQUE tour de boucle : quatre appels forment une trame
 * complète d'affichage, et c'est aussi ce qui applique l'état des relais. */
void refresh();

}  // namespace board
