#include "lfo.hpp"
#include <cmath>

namespace teslasynth::synth {
using namespace teslasynth::core;

constexpr float _2pi = 6.2831853071795864769;

Hertz Vibrato::offset(const Duration &now) {
  return depth * sinf(freq * _2pi * (now.micros() / 1e6f));
}

} // namespace teslasynth::synth
