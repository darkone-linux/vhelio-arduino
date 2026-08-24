/*
 * horn.h — Klaxon, avec verrou anti-blocage.
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
