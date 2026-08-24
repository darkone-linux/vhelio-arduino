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
 * Source : bibliothèque de référence af3556/IO22_IO_Board, recoupée avec les
 * spécifications Eletechsup. À confirmer avec tools/pinscan (specs/03 §6).
 */
#pragma once

#include <Arduino.h>

/* ---- Entrées optocouplées (NPN, déclenchement à l'état bas) --------------
 * Reliées directement au Nano : D2, D3, D4, D5, D6, A0, D12, D11.
 * NPN = une borne s'active en la fermant sur la MASSE. Tous les contacts du
 * faisceau sont donc des contacts secs vers GND — y compris le contacteur de
 * frein avant, qui est unipolaire. Voir hardware/cablage.md §2.           */
enum InIdx : uint8_t {
  IN_TURN_LEFT   = 0,  /* IN1 / D2  — comodo, position gauche       */
  IN_TURN_RIGHT  = 1,  /* IN2 / D3  — comodo, position droite       */
  IN_HORN        = 2,  /* IN3 / D4  — comodo, bouton klaxon         */
  IN_BRAKE_FRONT = 3,  /* IN4 / D5  — contacteur frein avant        */
  IN_BRAKE_REAR  = 4,  /* IN5 / D6  — contacteur frein arrière      */
  IN_PARK        = 5,  /* IN6 / A0  — inter dédié « veilleuse »     */
  IN_MAIN        = 6,  /* IN7 / D12 — comodo, éclairage fort        */
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
  OUT_HORN       = 6,  /* R7 — klaxon                               */
  OUT_MOTOR_CUT  = 7,  /* R8 — coupure moteur (contact sec)         */
  OUT_COUNT      = 8
};

/* ---- Boutons de la carte -------------------------------------------------
 * Quatre poussoirs sur D7, D8, D9, D10. Ils sont SUR la carte, donc dans le
 * boîtier — et le boîtier est sous la coque : inaccessibles en roulant.
 * Réservés à la maintenance, jamais à une commande de conduite.
 *
 * Attention à la sérigraphie : sur cette famille de cartes, les repères K1..K4
 * sont imprimés dans l'ORDRE INVERSE du câblage. Le bouton marqué « K4 » est
 * celui qui est relié à D7, donc celui que le firmware appelle BTN_PAGE. Le
 * pinscan tranche (specs/03 §6, étape 5).                                  */
enum BtnIdx : uint8_t {
  BTN_PAGE     = 0,   /* D7 (sérigraphié K4) — page suivante        */
  BTN_LAMPTEST = 1,   /* D8 (sérigraphié K3) — test des feux        */
  BTN_SPARE1   = 2,   /* D9 (sérigraphié K2)                        */
  BTN_SPARE2   = 3,   /* D10 (sérigraphié K1)                       */
  BTN_COUNT    = 4
};

/* ---- Chaîne de registres à décalage --------------------------------------
 * Trois 74HC595 en série : U3 et U4 pour l'afficheur, U5 pour les relais.
 * L'ordre d'émission par digit est imposé par le câblage :
 *     octet bas du digit  ->  octet haut du digit  ->  octet relais
 * Pilotage par accès direct aux ports : shiftOut() coûterait ~5 µs/bit, soit
 * 120 µs par digit, contre ~8 µs ici.                                      */
#define PIN_SR_DATA     13   /* PB5 — aussi la LED intégrée du Nano        */
#define PIN_SR_CLOCK    A3   /* PC3                                        */
#define PIN_SR_LATCH    A2   /* PC2                                        */
#define PIN_RELAY_OE    A1   /* PC1 — validation des relais, ACTIVE À L'ÉTAT BAS */

/* Masques d'accès direct aux ports, cohérents avec les broches ci-dessus. */
#define SR_DATA_PORT    PORTB
#define SR_DATA_BIT     PB5
#define SR_CLOCK_PORT   PORTC
#define SR_CLOCK_BIT    PC3
#define SR_LATCH_PORT   PORTC
#define SR_LATCH_BIT    PC2

/* ---- Broches restées libres ---------------------------------------------
 * A4 et A5 sont les seules broches libres capables d'interruption sur
 * changement d'état, donc les seules utilisables en RX logiciel.
 * A6/A7 sont libres mais analogiques seules.
 * D0/D1 : console série. La carte embarque bien un RS485 sur D0/D1, mais un
 * inverseur à glissière « 485_ON / PRO » l'en déconnecte. Position **PRO**
 * en permanence : la console et le téléversement USB fonctionnent alors
 * normalement. Voir specs/03 §2 bis.                                       */
#define PIN_BAFANG_RX   A4   /* écoute passive du contrôleur Bafang        */
#define PIN_BAFANG_TX   A5   /* réservé par SoftwareSerial, NON CÂBLÉ      */
#define PIN_WHEEL       A6   /* capteur de roue (option) — lecture analogique */
