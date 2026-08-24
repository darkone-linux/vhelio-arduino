/*
 * config.h — Tous les réglages du firmware VHélio.
 *
 * Rien d'autre que des #define ici : c'est le seul fichier à toucher pour
 * adapter le comportement sans lire le code. Le brochage est dans pins.h.
 */
#pragma once

#define VHELIO_FW_VERSION "0.1.0"

/* ======================================================================
 * Variantes de câblage
 * ====================================================================== */

/* 1 = les deux freins sont en parallèle sur la ligne frein du contrôleur,
 *     l'Arduino ne lit qu'une seule entrée (IN_BRAKE_FRONT). IN_BRAKE_REAR
 *     est alors libre.
 * 2 = freins avant et arrière lus séparément (défaut).
 * Voir specs/03-affectation-es.md §4. */
#define BRAKE_WIRING_VARIANT      2

/* Mettre à 1 si l'entrée frein correspondante passe par l'interface
 * transistor de la variante A : la logique est alors inversée, et une
 * rupture de fil est interprétée comme un freinage (état sûr). */
#define IN_INVERT_BRAKE_FRONT     0
#define IN_INVERT_BRAKE_REAR      0

/* Rôle de OUT8 : 1 = buzzer de retour clignotants, 0 = relais accessoires. */
#define OUT8_ROLE_BUZZER          1

/* ======================================================================
 * Éclairage
 * ====================================================================== */

#define HIGHBEAM_REQUIRES_LOWBEAM 1   /* le feu de route exige le croisement  */
#define HIGHBEAM_KEEPS_LOWBEAM    1   /* le croisement reste allumé avec route */
#define TAIL_ALWAYS_ON            0   /* veilleuse arrière permanente (DRL)   */

#define TAIL_PWM_PARK             52  /* ~20 % — veilleuse                    */
#define TAIL_PWM_BRAKE            255 /* 100 % — feu stop                     */
#define TAIL_SOFTSTART_MS         200 /* rampe d'allumage de la veilleuse     */

/* ======================================================================
 * Freinage
 * ====================================================================== */

#define BRAKE_HOLD_MS             300   /* maintien de la coupure après relâche */
#define BRAKE_STUCK_MS            120000UL /* freinage continu => défaut       */

/* Flash d'attaque du feu stop. Laisser à 0 : un feu stop clignotant n'est pas
 * conforme au code de la route français (specs/07-securite.md §6). */
#define BRAKE_FLASH_ENABLE        0
#define BRAKE_FLASH_COUNT         3
#define BRAKE_FLASH_ON_MS         60
#define BRAKE_FLASH_OFF_MS        60

/* ======================================================================
 * Clignotants
 * ====================================================================== */

#define BLINK_PERIOD_MS           750   /* 1,33 Hz = 80 cycles/min           */
#define BLINK_ON_MS               375   /* rapport cyclique 50 %             */
#define BLINK_REMINDER_MS         45000UL /* rappel d'oubli : durée          */
#define BLINK_REMINDER_MM         300000UL /* rappel d'oubli : 300 m en mm   */
#define BUZZ_CLICK_MS             25    /* clic normal                       */
#define BUZZ_REMINDER_MS          150   /* bip long de rappel                */

/* ======================================================================
 * Klaxon
 * ====================================================================== */

#define HORN_MAX_ON_MS            10000UL /* anti-blocage                    */

/* ======================================================================
 * Anti-rebond (ms)
 * ====================================================================== */

#define DEBOUNCE_BRAKE_MS         15
#define DEBOUNCE_HORN_MS          20
#define DEBOUNCE_COMODO_MS        30

/* Seuil de basculement pour A6/A7, lues en analogique (0..1023). */
#define ANALOG_INPUT_THRESHOLD    512

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

/* Source de vitesse alternative : capteur de roue scruté sur PIN_WHEEL. */
#define SPEED_SOURCE_WHEEL        0
#define WHEEL_MIN_PULSE_GAP_MS    20      /* ~ vitesse max plausible         */
#define WHEEL_TIMEOUT_MS          3000UL  /* sans impulsion => vitesse nulle */

#define WHEEL_CIRCUMFERENCE_MM    2200    /* roue 700C ; à ajuster           */

/* ======================================================================
 * Diagnostic
 * ====================================================================== */

#define DEBUG_SERIAL              1
#define DEBUG_BAUD                115200
#define DEBUG_PERIOD_MS           1000

#define SELFTEST_ENABLE           1
#define SELFTEST_STEP_MS          150

#define WATCHDOG_ENABLE           1
#define LOOP_SLOW_US              10000UL /* seuil de défaut sur le cycle    */
#define BRAKE_NEVER_MM            2000000UL /* 2 km sans freinage => défaut  */

#define HEARTBEAT_OK_MS           500     /* LED 1 Hz  : nominal             */
#define HEARTBEAT_FAULT_MS        100     /* LED 5 Hz  : défaut actif        */

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

#if TAIL_PWM_PARK >= TAIL_PWM_BRAKE
#error "La veilleuse doit etre moins lumineuse que le feu stop"
#endif

#if BLINK_ON_MS >= BLINK_PERIOD_MS
#error "BLINK_ON_MS doit etre inferieur a BLINK_PERIOD_MS"
#endif
