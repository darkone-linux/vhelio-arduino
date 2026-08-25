/*
 * config.h — Tous les réglages du firmware VHélio.
 *
 * Rien d'autre que des #define ici : c'est le seul fichier à toucher pour
 * adapter le comportement sans lire le code. Le brochage est dans pins.h.
 */
#pragma once

#define VHELIO_FW_VERSION "0.3.0"

/* ======================================================================
 * Variantes de câblage
 * ====================================================================== */

/* 1 = une seule entrée frein est câblée (IN_BRAKE_FRONT). IN_BRAKE_REAR est
 *     alors forcée inactive par le firmware.
 * 2 = freins avant et arrière lus séparément (défaut).
 * Voir specs/03-affectation-es.md §5. */
#define BRAKE_WIRING_VARIANT      2

/* Mettre à 1 si l'entrée frein correspondante passe par l'interface
 * transistor de lecture de la ligne frein Bafang : la logique est alors
 * inversée, et une rupture de fil est interprétée comme un freinage (état
 * sûr). Ne concerne que IN_BRAKE_REAR dans le câblage retenu — le contacteur
 * avant est un contact sec direct vers la masse. */
#define IN_INVERT_BRAKE_FRONT     0
#define IN_INVERT_BRAKE_REAR      0

/* ======================================================================
 * Éclairage
 * ====================================================================== */

/* Deux niveaux, deux commandes physiquement distinctes :
 *   IN_PARK  = interrupteur dédié « veilleuse »  -> R1 (veilleuse avant)
 *   IN_MAIN  = interrupteur du comodo, plein feu -> R2 (phares)
 *
 * MAIN_REQUIRES_PARK vaut 0 volontairement : l'éclairage fort ne doit JAMAIS
 * être bloqué par un interrupteur de veilleuse resté ouvert. */
#define MAIN_REQUIRES_PARK        0   /* 1 = le phare exige la veilleuse       */
#define MAIN_KEEPS_PARK           1   /* la veilleuse reste allumée avec phare */
#define TAIL_ALWAYS_ON            0   /* veilleuse arrière permanente (DRL)    */

/* Le feu de position ARRIÈRE suit toujours l'éclairage, quel que soit le
 * niveau demandé et quelles que soient les deux options ci-dessus : rouler
 * éclairé à l'avant sans feu arrière est la faute la plus dangereuse que ce
 * montage puisse commettre. Ce n'est délibérément pas configurable.
 *
 * Les feux arrière sont sur DEUX relais distincts (veilleuse et stop) : un
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

/* Comment le rappel d'oubli se manifeste (Q12 : le boîtier est sous la coque,
 * l'afficheur n'est pas lisible en roulant, et il n'y a pas de buzzer).
 *
 * Le rappel change le RAPPORT CYCLIQUE, pas la période. La cadence reste à
 * 80 cycles/min — donc toujours dans la plage réglementaire 60-120 — mais le
 * rythme du claquement passe de régulier (375/375) à syncopé (200/550). Le
 * conducteur l'entend, le code de la route est respecté, et cela ne coûte ni
 * composant ni sortie.
 *
 * Contrepartie assumée : le feu est allumé 27 % du temps au lieu de 50 %,
 * donc légèrement moins visible. C'est acceptable précisément parce qu'après
 * 45 s ou 300 m, le clignotant est très probablement resté allumé pour rien.
 * Mettre 0 pour désactiver le rappel sonore et ne garder que le journal. */
#define BLINK_REMINDER_ON_MS      200

/* ======================================================================
 * Voie IN3 / R7 — voyant de défaut et acquittement
 * ====================================================================== */

#define FAULT_LAMP_ENABLE         1

/* QUELS défauts allument le voyant. Ce masque est la décision la plus
 * importante de tout ce fichier : un voyant qui s'allume pour rien est un
 * voyant qu'on cesse de regarder, et il vaut alors moins que pas de voyant
 * du tout.
 *
 *   bit 0  conflit clignotants ....... OUI, la signalisation est perdue
 *   bit 1  voie auxiliaire bloquée ... OUI (sans objet si pas de klaxon)
 *   bit 2  lien Bafang perdu ......... NON — principe P2, voir ci-dessous
 *   bit 3  frein collé ............... OUI, assistance coupée en continu
 *   bit 4  cycle lent ................ NON, information de maintenance
 *   bit 5  reset chien de garde ...... OUI, les feux se sont éteints ~1,5 s
 *   bit 6  aucun freinage vu en 2 km . OUI, fil de contacteur probablement coupé
 *
 * Le lien Bafang est délibérément exclu : la télémétrie est un confort, pas
 * une fonction de sécurité (P2). Afficheur d'origine débranché, bus muet ou
 * trames non reconnues, le voyant resterait allumé en permanence et ne
 * voudrait plus rien dire. Ces défauts restent lisibles sur la page
 * « défauts » de l'afficheur et au journal série. */
#define FAULT_LAMP_MASK           0x6B

/* Le klaxon du véhicule est AUTONOME : batterie et interrupteur propres. Le
 * module est conservé, désactivé, pour le cas où l'on raccorderait un klaxon
 * à cette même voie — il ne coûte alors rien en flash. Les deux usages
 * s'excluent : R7 ne peut pas être à la fois un voyant et un klaxon. */
#define HORN_ENABLE               0

#define HORN_MAX_ON_MS            10000UL   /* anti-blocage                  */

/* ======================================================================
 * Anti-rebond (ms)
 * ====================================================================== */

#define DEBOUNCE_BRAKE_MS         15
#define DEBOUNCE_ACK_MS           20
#define DEBOUNCE_COMODO_MS        30
#define DEBOUNCE_BUTTON_MS        40

/* ======================================================================
 * Afficheur 4 digits
 * ====================================================================== */

/* Le boîtier est monté sous la coque (Q12) : l'afficheur n'est PAS lisible en
 * roulant. C'est un outil de maintenance, pas un tableau de bord — d'où la
 * page « défauts » par défaut : c'est ce qu'on veut voir en ouvrant la coque.
 * Le diagnostic de conduite repose entièrement sur le journal série. */
#define DISPLAY_ENABLE            1
#define DISPLAY_DEFAULT_PAGE      2    /* 0=vitesse 1=charge 2=défauts 3=odomètre */
#define DISPLAY_BLINK_MS          500  /* point décimal = battement, 1 Hz        */
#define DISPLAY_BLINK_FAULT_MS    120  /* battement rapide = défaut actif        */

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
 * Banc d'essai — simulation des entrées depuis la console
 * ====================================================================== */

/* 1 = les huit entrées peuvent être forcées au clavier, borne par borne, pour
 * dérouler la campagne 1 du plan de tests avant que le faisceau soit serti.
 * Une entrée que la console n'a pas réquisitionnée reste lue sur son
 * optocoupleur : les deux méthodes d'injection — clavier et fil volant vers la
 * masse — cohabitent dans la même compilation et se recoupent.
 *
 * DOIT VALOIR 0 SUR LE VÉHICULE. Ce n'est pas qu'une précaution de principe :
 * un caractère parasite sur la ligne série suffirait à fermer un contact de
 * frein ou à allumer une détresse. Trois garde-fous rendent l'oubli difficile
 * — un #warning à la compilation, une bannière au démarrage, et la colonne
 * `sim=` que le journal porte à chaque ligne. Voir src/simconsole.h.
 *
 * On ne le met PAS à 1 ici. Le banc se compile avec
 *     VHELIO_SIM=1 ./tools/build-nix.sh
 * qui définit -DSIM_INPUTS=1 et compile dans .build/vhelio-sim, séparé du
 * binaire de route. Le fichier versionné reste ainsi toujours à 0 : on ne peut
 * pas oublier de le remettre, puisqu'on ne l'a pas touché. */
#ifndef SIM_INPUTS
#define SIM_INPUTS                0
#endif

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

#if HORN_ENABLE && FAULT_LAMP_ENABLE
#error "R7 ne peut pas etre a la fois voyant de defaut et klaxon"
#endif

#if FAULT_LAMP_ENABLE && !FAULT_LAMP_MASK
#warning "FAULT_LAMP_MASK vide : le voyant ne s'allumera jamais"
#endif

#if BLINK_ON_MS >= BLINK_PERIOD_MS
#error "BLINK_ON_MS doit etre inferieur a BLINK_PERIOD_MS"
#endif

#if BLINK_REMINDER_ON_MS >= BLINK_PERIOD_MS
#error "BLINK_REMINDER_ON_MS doit etre inferieur a BLINK_PERIOD_MS"
#endif

#if DISPLAY_ENABLE && DISPLAY_DEFAULT_PAGE > 3
#error "DISPLAY_DEFAULT_PAGE doit etre compris entre 0 et 3"
#endif

#if BRAKE_FLASH_ENABLE
#warning "Flash d'attaque actif : usure mecanique acceleree du relais de stop"
#endif

#if SIM_INPUTS && !DEBUG_SERIAL
#error "SIM_INPUTS exige DEBUG_SERIAL 1 : les commandes arrivent par la console"
#endif

#if SIM_INPUTS
#warning "SIM_INPUTS actif : entrees pilotables au clavier. NE PAS ROULER AVEC."
#endif
