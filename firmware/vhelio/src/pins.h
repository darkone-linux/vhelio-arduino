/*
 * pins.h — Brochage DN22D08 <-> Arduino Nano, et index logiques.
 *
 * Le brochage physique est dans les tableaux OUT_PIN[] / IN_PIN[] de
 * board_io.cpp. Ce fichier ne contient que la nomenclature logique, qui ne
 * change pas quand la carte change.
 *
 * !! Vérifier le brochage réel avant le premier câblage :
 *    specs/03-affectation-es.md §5.
 */
#pragma once

#include <Arduino.h>

/* Entrées — IN1..IN8 de la carte. */
enum InIdx : uint8_t {
  IN_TURN_LEFT   = 0,  /* IN1 / A0 — comodo, position gauche       */
  IN_TURN_RIGHT  = 1,  /* IN2 / A1 — comodo, position droite       */
  IN_HORN        = 2,  /* IN3 / A2 — comodo, bouton klaxon         */
  IN_BRAKE_FRONT = 3,  /* IN4 / A3 — contacteur frein avant        */
  IN_BRAKE_REAR  = 4,  /* IN5 / A4 — contacteur frein arrière      */
  IN_LOWBEAM     = 5,  /* IN6 / A5 — comodo, croisement            */
  IN_HIGHBEAM    = 6,  /* IN7 / A6 — comodo, route (analogique)    */
  IN_HAZARD      = 7,  /* IN8 / A7 — inter détresse (analogique)   */
  IN_COUNT       = 8
};

/* Sorties — OUT1..OUT8 de la carte. */
enum OutIdx : uint8_t {
  OUT_LOWBEAM    = 0,  /* OUT1 / D2 — feu de croisement            */
  OUT_HIGHBEAM   = 1,  /* OUT2 / D3 — feu de route                 */
  OUT_TURN_LEFT  = 2,  /* OUT3 / D4 — clignotants gauche           */
  OUT_TURN_RIGHT = 3,  /* OUT4 / D5 — clignotants droite           */
  OUT_TAIL       = 4,  /* OUT5 / D6 — feux arrière (PWM requis)    */
  OUT_HORN       = 5,  /* OUT6 / D7 — klaxon (via relais)          */
  OUT_MOTOR_CUT  = 6,  /* OUT7 / D8 — coupure moteur (via opto)    */
  OUT_AUX        = 7,  /* OUT8 / D9 — buzzer ou relais accessoires */
  OUT_COUNT      = 8
};

/* Broches hors carte d'E/S. */
#define PIN_BAFANG_RX   10   /* RX logiciel, écoute passive            */
#define PIN_BAFANG_TX   11   /* réservé par SoftwareSerial, NON CÂBLÉ  */
#define PIN_WHEEL       12   /* capteur de roue (option)               */
#define PIN_HEARTBEAT   13   /* LED intégrée                           */
