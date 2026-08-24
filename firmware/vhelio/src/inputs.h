/*
 * inputs.h — Agrégation des huit entrées : anti-rebond, fronts, inversions.
 *
 * Les modules métier ne lisent jamais board::readInputRaw() directement : ils
 * consomment cette structure, déjà filtrée et normalisée.
 */
#pragma once

#include <Arduino.h>
#include "pins.h"

struct InputState {
  bool level[IN_COUNT];      /* état stable                                  */
  bool rose[IN_COUNT];       /* front montant sur ce cycle                   */
  bool fell[IN_COUNT];       /* front descendant sur ce cycle                */
  uint32_t changedAt[IN_COUNT]; /* date du dernier changement stable         */
};

namespace inputs {

void begin();
void update(uint32_t now);
const InputState& state();

}  // namespace inputs
