/*
 * pins.h — Brochage DN22D08 (famille Eletechsup IO22/DN22) <-> Arduino Nano.
 *
 * ATTENTION : cette carte n'expose PAS une broche par entrée/sortie.
 *   - les 8 relais sont pilotés par un registre à décalage 74HC595 chaîné
 *     avec les deux registres de l'afficheur ;
 *   - les 8 entrées optocouplées sont, elles, reliées directement au Nano.
 *
 * Bilan de broches qui rend l'architecture obligatoire :
 *   8 entrées + 4 boutons + 8 relais + 8 segments + 4 sélections de digit = 32
 *   Un Nano en offre 20. Le registre à décalage n'est pas un choix, c'est une
 *   nécessité arithmétique.
 *
 * Source : MESURÉ sur la carte avec tools/pinfind (specs/03 §6 bis), et non
 * plus déduit. La bibliothèque af3556/IO22_IO_Board, qui servait de référence,
 * ne couvre que l'IO22D08 : la DN22D08 en diffère sur les boutons et sur deux
 * des huit entrées. Le brochage ci-dessous est celui de la DN22D08 réelle.
 *
 * Reste inconnu : la chaîne à décalage. Voir le bloc en fin de fichier.
 */
#pragma once

#include <Arduino.h>

/* ---- Entrées optocouplées (NPN, déclenchement à l'état bas) --------------
 * Reliées directement au Nano : D2, D3, D4, D5, D6, D7, D9, D11 — mesuré.
 * NPN = une borne s'active en la fermant sur la MASSE. Tous les contacts du
 * faisceau sont donc des contacts secs vers GND — y compris le contacteur de
 * frein avant, qui est unipolaire. Voir hardware/cablage.md §2.           */
enum InIdx : uint8_t {
  IN_TURN_LEFT   = 0,  /* IN1 / D2  — comodo, position gauche       */
  IN_TURN_RIGHT  = 1,  /* IN2 / D3  — comodo, position droite       */
  IN_ACK         = 2,  /* IN3 / D4  — bouton d'acquittement défaut  */
  IN_BRAKE_FRONT = 3,  /* IN4 / D5  — contacteur frein avant        */
  IN_BRAKE_REAR  = 4,  /* IN5 / D6  — contacteur frein arrière      */
  IN_PARK        = 5,  /* IN6 / D7  — inter dédié « veilleuse »     */
  IN_MAIN        = 6,  /* IN7 / D9  — comodo, éclairage fort        */
  IN_HAZARD      = 7,  /* IN8 / D11 — inter dédié détresse (S1)     */
  IN_COUNT       = 8
};

/* ---- Relais, via le registre à décalage ---------------------------------
 * Contacts secs 10 A NO/NC. Aucun étage de puissance externe nécessaire.  */
enum OutIdx : uint8_t {
  OUT_PARK_FRONT = 0,  /* R1 — veilleuse avant                      */
  OUT_MAIN       = 1,  /* R2 — phares (éclairage fort)              */
  OUT_TURN_LEFT  = 2,  /* R3 — clignotants gauche (avant + arrière) */
  OUT_TURN_RIGHT = 3,  /* R4 — clignotants droite (avant + arrière) */
  OUT_TAIL_PARK  = 4,  /* R5 — feux de position arrière (veilleuse) */
  OUT_TAIL_STOP  = 5,  /* R6 — feux stop arrière                    */
  OUT_FAULT      = 6,  /* R7 — voyant rouge de défaut               */
  OUT_MOTOR_CUT  = 7,  /* R8 — coupure moteur (contact sec)         */
  OUT_COUNT      = 8
};

/* ---- La voie IN3 / R7 ----------------------------------------------------
 * Le klaxon est autonome, ce qui a libéré cette voie — la seule marge du
 * montage. Elle porte désormais le diagnostic au poste de conduite :
 *
 *   R7  -> voyant rouge de défaut. Le boîtier est sous la coque et son
 *          afficheur illisible en roulant ; sans ce voyant, RIEN ne signale
 *          un défaut avant l'ouverture de la coque.
 *   IN3 -> bouton d'acquittement. Efface les défauts mémorisés et éteint le
 *          voyant pour ceux qui persistent, sans couper l'alimentation.
 *
 * Le bouton d'acquittement est le seul organe que le conducteur peut
 * actionner en roulant sans effet sur les feux, les freins ou le moteur :
 * il ne touche QUE le module diag. C'est une contrainte, pas un hasard.
 *
 * `HORN_ENABLE` rend la voie à un klaxon si l'on en raccordait un ; les deux
 * usages s'excluent, config.h le vérifie.                                  */

/* ---- Boutons de la carte -------------------------------------------------
 * Quatre poussoirs sur D12, D10, D8, A0 — mesuré, de K1 à K4 de gauche à
 * droite. Ils sont SUR la carte, donc dans le boîtier — et le boîtier est
 * sous la coque : inaccessibles en roulant. Réservés à la maintenance,
 * jamais à une commande de conduite.
 *
 * La question de la sérigraphie est tranchée : le poussoir de gauche, marqué
 * K1, est bien celui que le firmware appelle BTN_PAGE. Ce sont les NUMÉROS DE
 * BROCHE qui décroissent de gauche à droite (12, 10, 8, puis A0 = 14), et
 * c'est cette décroissance qui a fait parler d'une sérigraphie « inversée »
 * sur les cartes voisines. Il n'y a rien à inverser.                       */
enum BtnIdx : uint8_t {
  BTN_PAGE     = 0,   /* K1, D12 — page suivante                    */
  BTN_LAMPTEST = 1,   /* K2, D10 — test des feux                    */
  BTN_SPARE1   = 2,   /* K3, D8                                     */
  BTN_SPARE2   = 3,   /* K4, A0                                     */
  BTN_COUNT    = 4
};

/* ---- Chaîne de registres à décalage — NON CONFIRMÉ -----------------------
 * MESURÉ avec tools/pinchain (specs/03 §6 bis, phase B), triplet 120 sur 120.
 * La signature ne laisse pas de place au doute : en promenant un seul bit à
 * travers la chaîne, on obtient un relais à la fois sur les huit premières
 * positions, puis les segments de l'afficheur. Aucun triplet faux ne produit
 * cela — ils font du bruit, pas de l'ordre.
 *
 * La chaîne a donc pris A4 et A5, qui étaient promises à l'écoute Bafang.
 *
 * Trois 74HC595 en série : U3 et U4 pour l'afficheur, U5 pour les relais.
 * L'ordre d'émission par digit est imposé par le câblage :
 *     octet bas du digit  ->  octet haut du digit  ->  octet relais
 * Pilotage par accès direct aux ports : shiftOut() coûterait ~5 µs/bit, soit
 * 120 µs par digit, contre ~8 µs ici.                                      */
#define PIN_SR_DATA     A5   /* PC5 — mesuré, tools/pinchain triplet 120   */
#define PIN_SR_CLOCK    A4   /* PC4 — mesuré                               */
#define PIN_SR_LATCH    A3   /* PC3 — mesuré                               */

/* OE — MESURÉ. Les huit relais collés, A2 passée au niveau haut les fait
 * retomber ; D13 et A1 sont sans effet. La validation est donc bien active à
 * l'état bas, et c'est bien A2 qui la porte.                              */
#define PIN_RELAY_OE    A2   /* PC2 — validation, ACTIVE À L'ÉTAT BAS      */

/* Masques d'accès direct aux ports, cohérents avec les broches ci-dessus.
 * Les trois lignes sont sur PORTC, ce qui n'était pas le cas du brochage
 * supposé : une seule écriture de port suffirait à les piloter ensemble. */
#define SR_DATA_PORT    PORTC
#define SR_DATA_BIT     PC5
#define SR_CLOCK_PORT   PORTC
#define SR_CLOCK_BIT    PC4
#define SR_LATCH_PORT   PORTC
#define SR_LATCH_BIT    PC3

/* ---- Broches restées libres ---------------------------------------------
 * A4 et A5 sont les seules broches libres capables d'interruption sur
 * changement d'état, donc les seules utilisables en RX logiciel.
 * A6/A7 sont libres mais analogiques seules.
 * D0/D1 : console série. La carte embarque bien un RS485 sur D0/D1, mais un
 * inverseur à glissière « 485_ON / PRO » l'en déconnecte. Position **PRO**
 * en permanence : la console et le téléversement USB fonctionnent alors
 * normalement. Voir specs/03 §2 bis.                                       */
/* A4 et A5 sont parties à la chaîne, A2 à l'OE. Il ne reste donc que D13 et
 * A1 — et l'écoute Bafang survit tout juste : un RX logiciel exige une
 * interruption sur changement d'état, que A1 possède (PCINT9). A6 et A7
 * n'auraient pas pu : analogiques seules, sans PCINT.
 *
 * Le RX va sur A1 et non sur D13, bien que les deux aient un PCINT : la LED
 * intégrée du Nano et sa résistance chargent D13, ce qui n'a aucune
 * importance pour une sortie mais dégrade une entrée qu'on écoute.
 *
 * D13 hérite donc du TX, que SoftwareSerial exige mais que rien ne câble.
 * Conséquence visible : SoftwareSerial met le TX au repos à l'état HAUT, donc
 * la LED du Nano reste ALLUMÉE en permanence. Comme on n'émet jamais, on peut
 * la rendre à l'état d'entrée juste après begin() pour l'éteindre.         */
#define PIN_BAFANG_RX   A1   /* PC1 — écoute passive du contrôleur Bafang  */
#define PIN_BAFANG_TX   13   /* PB5 — réservé par SoftwareSerial, NON CÂBLÉ */
#define PIN_WHEEL       A6   /* capteur de roue (option) — lecture analogique */
