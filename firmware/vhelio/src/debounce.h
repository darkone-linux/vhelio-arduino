/*
 * debounce.h — Anti-rebond par stabilité.
 *
 * La sortie ne change que si l'entrée brute est restée identique pendant
 * stableMs. Le retard introduit est donc exactement stableMs ; il est pris en
 * compte dans le budget de réaction au freinage (specs/06 §6).
 */
#pragma once

#include <Arduino.h>

class Debouncer {
 public:
  void begin(uint16_t stableMs, bool initial = false);
  void update(bool raw, uint32_t now);

  bool level() const { return stable_; }
  /* Valides uniquement pendant le cycle où la transition a eu lieu. */
  bool rose() const { return rose_; }
  bool fell() const { return fell_; }
  uint32_t changedAt() const { return changedAt_; }

 private:
  uint32_t candidateSince_ = 0;
  uint32_t changedAt_ = 0;
  uint16_t stableMs_ = 20;
  bool stable_ = false;
  bool candidate_ = false;
  bool rose_ = false;
  bool fell_ = false;
};
