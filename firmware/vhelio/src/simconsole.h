/*
 * simconsole.h — Injection des entrées depuis la console série (banc d'essai).
 *
 * Permet de dérouler la campagne 1 du plan de tests AVANT que le faisceau soit
 * serti. Une touche ferme ou ouvre un contact ; tout ce qui est en aval —
 * anti-rebond, machines à états, relais, afficheur, journal — est exercé
 * exactement comme avec un vrai contacteur. Rien n'est court-circuité : la
 * simulation remplace la LECTURE de la borne, pas le traitement.
 *
 * Le masque est PAR ENTRÉE. Une entrée que la console n'a pas réquisitionnée
 * continue d'être lue sur son optocoupleur : on peut donc simuler la détresse
 * au clavier pendant qu'un fil volant ferme IN4 sur la masse. C'est ce qui
 * permet de recouper les deux méthodes — un écart entre elles désigne le
 * câblage, jamais le firmware.
 *
 * UNE LIMITE CONNUE : afficher l'aide ('?') tient la boucle ~19 ms, le temps
 * d'écouler quatre lignes dans un tampon série de 64 octets. C'est au-dessus
 * de LOOP_SLOW_US, et cela mémorise donc un FLT_LOOP_SLOW qui n'a rien à voir
 * avec le firmware de route. L'acquittement (touche 3) l'efface. On ne
 * cherche pas à le masquer : un mécanisme qui excuse une boucle longue
 * finirait par en excuser une vraie.
 *
 * SIM_INPUTS vaut 0 en production et ce module ne compile alors AUCUN code.
 * Il ne peut pas non plus être activé en silence : config.h exige
 * DEBUG_SERIAL, émet un #warning à la compilation, et le journal porte une
 * colonne `sim=` à chaque ligne tant qu'une entrée est réquisitionnée.
 */
#pragma once

#include <Arduino.h>
#include "config.h"

namespace simconsole {

void begin();

/* Vide la file série et applique les commandes. Non bloquant. */
void poll();

/* true = cette entrée est pilotée par la console, pas par son optocoupleur. */
bool active(uint8_t idx);

/* Niveau imposé. N'a de sens que si active(idx). */
bool level(uint8_t idx);

}  // namespace simconsole
