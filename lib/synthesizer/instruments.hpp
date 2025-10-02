#pragma once

#include "envelope.hpp"
#include "lfo.hpp"

struct Instrument {
  ADSR envelope;
  Vibrato vibrato;
};

static Instrument instruments[] = {
    {.envelope = {30_ms, 20_ms, EnvelopeLevel(50), 40_ms, CurveType::Exp},
     .vibrato = {}},
    {.envelope = {30_ms, 20_ms, EnvelopeLevel(50), 40_ms, CurveType::Exp},
     .vibrato = {1, 2, true}},
    {.envelope = {30_ms, 20_ms, EnvelopeLevel(50), 40_ms, CurveType::Exp},
     .vibrato = {1, 3, true}},
};
