#include "debounce.h"

void Debouncer::begin(uint16_t stableMs, bool initial) {
  stableMs_ = stableMs;
  stable_ = initial;
  candidate_ = initial;
  candidateSince_ = 0;
  changedAt_ = 0;
  rose_ = false;
  fell_ = false;
}

void Debouncer::update(bool raw, uint32_t now) {
  rose_ = false;
  fell_ = false;

  if (raw != candidate_) {
    candidate_ = raw;
    candidateSince_ = now;
    return;
  }
  if (candidate_ == stable_) {
    return;
  }
  /* Soustraction non signée : correcte au repliement de millis(). */
  if (now - candidateSince_ < stableMs_) {
    return;
  }
  stable_ = candidate_;
  changedAt_ = now;
  if (stable_) {
    rose_ = true;
  } else {
    fell_ = true;
  }
}
