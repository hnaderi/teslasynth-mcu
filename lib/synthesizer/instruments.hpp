#pragma once

#include "envelope.hpp"
#include "lfo.hpp"

struct Instrument {
  ADSR envelope;
  Vibrato vibrato;
};

const static Instrument instruments[] = {
    {.envelope = {30_ms, 20_ms, EnvelopeLevel(50), 40_ms, CurveType::Exp},
     .vibrato = {}},
    {.envelope = {30_ms, 20_ms, EnvelopeLevel(50), 40_ms, CurveType::Exp},
     .vibrato = {1_hz, 2_hz}},
    {.envelope = {30_ms, 20_ms, EnvelopeLevel(50), 40_ms, CurveType::Exp},
     .vibrato = {1_hz, 3_hz}},
};
