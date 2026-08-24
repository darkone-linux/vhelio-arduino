/*
 * horn.h — Klaxon, avec verrou anti-blocage.
 *
 * DÉSACTIVÉ par défaut (`HORN_ENABLE 0`) : le klaxon du véhicule est autonome,
 * avec sa propre batterie et son propre interrupteur. Le module est conservé
 * intact pour le cas où l'on déciderait de le raccorder à la voie auxiliaire
 * IN3 / R7 ; désactivé, il ne coûte rien.
 */
#pragma once

#include <Arduino.h>
#include "inputs.h"

namespace horn {

void begin();
void update(uint32_t now, const InputState& in);

bool sounding();
/* Bouton maintenu ou fil à la masse depuis plus de HORN_MAX_ON_MS. */
bool stuck();

}  // namespace horn
