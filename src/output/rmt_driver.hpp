#pragma once

#include "midi_synth.hpp"

namespace teslasynth::app::devices::rmt {
void pulse_write(const midisynth::Pulse *pulse, size_t len);
} // namespace teslasynth::devices::rmt
