#pragma once

#include "core.hpp"

struct Vibrato {
  Hertz freq = 0_hz;
  Hertz depth = 0_hz;

  Hertz offset(const Duration &now);
};
