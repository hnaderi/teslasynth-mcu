#pragma once

#include "application.hpp"
#include "midi_synth.hpp"

namespace teslasynth::app::configuration {
using teslasynth::midisynth::Config;

AppConfig read();
void persist(UIHandle &handle);

} // namespace teslasynth::app::configuration
