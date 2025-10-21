#pragma once

#include "core.hpp"

struct Vibrato {
  Hertz freq = 0_hz;
  Hertz depth = 0_hz;

  Hertz offset(const Duration &now);

  constexpr static Vibrato none() { return {}; }

  constexpr bool operator==(Vibrato b) const {
    return freq == b.freq && depth == b.depth;
  }

  constexpr bool operator!=(Vibrato b) const {
    return freq != b.freq || depth != b.depth;
  }

  inline operator std::string() const {
    return std::string("F: ") + std::string(freq) + std::string(" D: ") +
           std::string(depth);
  }
};
