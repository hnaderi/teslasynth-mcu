#pragma once

#include "core.hpp"
#include <algorithm>
#include <cstdint>
#include <stdint.h>

enum CurveType { Lin, Exp };

class EnvelopeLevel {
  uint8_t value;

public:
  explicit EnvelopeLevel(uint8_t level)
      : value(std::min<uint8_t>(level, 100)) {}

  EnvelopeLevel operator+(const EnvelopeLevel &b) const {
    return EnvelopeLevel(value + b.value);
  }
  constexpr bool operator<(const EnvelopeLevel &b) const {
    return value < b.value;
  }
  constexpr bool operator==(const EnvelopeLevel &b) const {
    return value == b.value;
  }
  constexpr bool operator<=(const EnvelopeLevel &b) const {
    return value <= b.value;
  }
};

struct ADSR {
  Duration attack, decay;
  EnvelopeLevel sustain;
  Duration release;
  enum CurveType type;
};

class Envelope {};
class Curve {};
