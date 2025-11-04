#pragma once

#include "midi_synth.hpp"

void rmt_driver(void);
void pulse_write(const Pulse  *pulse, size_t len);
