#include "lfo.hpp"
#include <cmath>

constexpr float _2pi = 6.2831853071795864769;

Hertz Vibrato::offset(const Duration &now) {
  return depth * std::sinf(freq * _2pi * now.seconds<float>());
}
