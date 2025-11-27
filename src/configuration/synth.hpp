#pragma once

#include "notes.hpp"

namespace teslasynth::app::configuration {

const synth::Config &load_config();
const synth::Config &get_config();
void update_config(const synth::Config &config);
void reset_config();
void save_config();

} // namespace teslasynth::app::configuration
