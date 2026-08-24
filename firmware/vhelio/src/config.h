/*
 * config.h — Tous les réglages du firmware VHélio.
 *
 * Rien d'autre que des #define ici : c'est le seul fichier à toucher pour
 * adapter le comportement sans lire le code. Le brochage est dans pins.h.
 */
#pragma once

#define VHELIO_FW_VERSION "0.2.0"

/* ======================================================================
 * Variantes de câblage
 * ====================================================================== */

/* 1 = les deux freins sont en parallèle sur la ligne frein du contrôleur,
 *     l'Arduino ne lit qu'une seule entrée (IN_BRAKE_FRONT). IN_BRAKE_REAR
 *     est alors libre.
 * 2 = freins avant et arrière lus séparément (défaut).
 * Voir specs/03-affectation-es.md §5. */
#define BRAKE_WIRING_VARIANT      2

/* Mettre à 1 si l'entrée frein correspondante passe par l'interface
 * transistor de la variante A : la logique est alors inversée, et une
 * rupture de fil est interprétée comme un freinage (état sûr). */
#define IN_INVERT_BRAKE_FRONT     0
#define IN_INVERT_BRAKE_REAR      0

/* ======================================================================
 * Éclairage
 * ====================================================================== */

#define HIGHBEAM_REQUIRES_LOWBEAM 1   /* le feu de route exige le croisement   */
#define HIGHBEAM_KEEPS_LOWBEAM    1   /* le croisement reste allumé avec route */
#define TAIL_ALWAYS_ON            0   /* veilleuse arrière permanente (DRL)    */

/* Les feux arrière sont sur DEUX relais distincts (veilleuse et stop) : un
 * relais ne module pas, la variante « circuit unique à intensité variable »
 * est matériellement impossible sur cette carte. Voir specs/03 §4. */

/* ======================================================================
 * Freinage
 * ====================================================================== */

#define BRAKE_HOLD_MS             300      /* maintien de la coupure après relâche */
#define BRAKE_STUCK_MS            120000UL /* freinage continu => défaut           */

/* Flash d'attaque du feu stop. Laisser à 0 : non conforme au code de la route
 * français, et surtout destructeur pour un relais (specs/07 §6). */
#define BRAKE_FLASH_ENABLE        0
#define BRAKE_FLASH_COUNT         3
#define BRAKE_FLASH_ON_MS         60
#define BRAKE_FLASH_OFF_MS        60

/* ======================================================================
 * Clignotants
 * ====================================================================== */

/* 1,33 Hz = 80 cycles/min, dans la plage réglementaire 60-120.
 * Les clignotants sont portés par des relais : chaque cycle est une
 * manoeuvre mécanique. Ralentir la cadence allonge la durée de vie, mais
 * sortir de la plage réglementaire n'est pas une option. */
#define BLINK_PERIOD_MS           750
#define BLINK_ON_MS               375
#define BLINK_REMINDER_MS         45000UL   /* rappel d'oubli : durée        */
#define BLINK_REMINDER_MM         300000UL  /* rappel d'oubli : 300 m en mm  */

/* ======================================================================
 * Klaxon
 * ====================================================================== */

#define HORN_MAX_ON_MS            10000UL   /* anti-blocage                  */

/* ======================================================================
 * Anti-rebond (ms)
 * ====================================================================== */

#define DEBOUNCE_BRAKE_MS         15
#define DEBOUNCE_HORN_MS          20
#define DEBOUNCE_COMODO_MS        30
#define DEBOUNCE_BUTTON_MS        40

/* ======================================================================
 * Afficheur 4 digits
 * ====================================================================== */

#define DISPLAY_ENABLE            1
#define DISPLAY_DEFAULT_PAGE      0    /* 0=vitesse 1=charge 2=défauts 3=odomètre */
#define DISPLAY_BLINK_MS          500  /* deux-points = battement de cœur, 1 Hz  */
#define DISPLAY_BLINK_FAULT_MS    120  /* deux-points rapide = défaut actif      */

/* ======================================================================
 * Télémétrie
 * ====================================================================== */

#define BAFANG_ENABLE             1
#define BAFANG_BAUD               1200
#define BAFANG_FRAME_GAP_MS       30      /* silence séparateur de trames    */
#define BAFANG_LINK_TIMEOUT_MS    2000UL  /* sans trame valide => lien perdu */
#define BAFANG_LEARN_MODE         0       /* 1 = dump hexa des trames        */

/* Interprétation du champ vitesse (specs/05-protocole-bafang.md §5) :
 *   0 = valeur directe en dixièmes de km/h
 *   1 = période de rotation de roue en millisecondes  */
#define BAFANG_SPEED_FORMULA      1

/* Source de vitesse alternative : capteur de roue sur PIN_WHEEL.
 * PIN_WHEEL est A6, analogique seule : pas de tirage interne possible, il
 * faut une résistance de 10 kΩ vers +5 V à l'extérieur. */
#define SPEED_SOURCE_WHEEL        0
#define WHEEL_MIN_PULSE_GAP_MS    20      /* ~ vitesse max plausible         */
#define WHEEL_TIMEOUT_MS          3000UL  /* sans impulsion => vitesse nulle */

#define WHEEL_CIRCUMFERENCE_MM    2200    /* roue 700C ; à ajuster           */

/* Seuil de basculement pour les broches lues en analogique (0..1023). */
#define ANALOG_INPUT_THRESHOLD    512

/* ======================================================================
 * Diagnostic
 * ====================================================================== */

#define DEBUG_SERIAL              1
#define DEBUG_BAUD                115200
#define DEBUG_PERIOD_MS           1000

#define SELFTEST_ENABLE           1
#define SELFTEST_STEP_MS          200     /* audible : un relais par pas      */

#define WATCHDOG_ENABLE           1
#define LOOP_SLOW_US              10000UL /* seuil de défaut sur le cycle     */
#define BRAKE_NEVER_MM            2000000UL /* 2 km sans freinage => défaut   */

/* ======================================================================
 * Cohérence des options
 * ====================================================================== */

#if BRAKE_WIRING_VARIANT != 1 && BRAKE_WIRING_VARIANT != 2
#error "BRAKE_WIRING_VARIANT doit valoir 1 ou 2"
#endif

#if BAFANG_LEARN_MODE && !BAFANG_ENABLE
#error "BAFANG_LEARN_MODE exige BAFANG_ENABLE 1"
#endif

#if BAFANG_LEARN_MODE && !DEBUG_SERIAL
#error "BAFANG_LEARN_MODE exige DEBUG_SERIAL 1 pour publier les trames"
#endif

#if BLINK_ON_MS >= BLINK_PERIOD_MS
#error "BLINK_ON_MS doit etre inferieur a BLINK_PERIOD_MS"
#endif

#if BRAKE_FLASH_ENABLE
#warning "Flash d'attaque actif : usure mecanique acceleree du relais de stop"
#endif
