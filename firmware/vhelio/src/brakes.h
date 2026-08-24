/*
 * brakes.h — Détection de freinage et coupure moteur redondante.
 *
 * Rappel du principe P1 : la coupure d'assistance est assurée MATÉRIELLEMENT
 * par le câblage des contacteurs sur la ligne frein du contrôleur Bafang.
 * OUT_MOTOR_CUT est un troisième chemin, redondant. Un firmware planté ne
 * doit pas empêcher de freiner, et ne doit pas non plus immobiliser le vélo.
 */
#pragma once

#include <Arduino.h>
#include "config.h"   /* BRAKE_FLASH_ENABLE conditionne une declaration */
#include "inputs.h"

namespace brakes {

void begin();
void update(uint32_t now, const InputState& in);

/* Au moins un frein actionné (état débouncé). Consommé par lights. */
bool braking();

/* État de la sortie de coupure, maintien de 300 ms inclus. */
bool motorCut();

/* Contacteur vraisemblablement collé : freinage continu depuis > 2 min. */
bool stuck();

/* Vrai dès qu'un freinage a été vu au moins une fois depuis le démarrage.
 * Sert à détecter un fil de frein coupé (defaut FLT_BRAKE_NEVER). */
bool everBraked();

#if BRAKE_FLASH_ENABLE
/* Demande d'extinction transitoire du stop pendant le flash d'attaque. */
bool flashBlanking();
#endif

}  // namespace brakes
