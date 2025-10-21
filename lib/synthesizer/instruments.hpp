#pragma once

#include "envelope.hpp"
#include "lfo.hpp"
#include <sstream>

struct Instrument {
  ADSR envelope;
  Vibrato vibrato;

  constexpr bool operator==(Instrument b) const {
    return envelope == b.envelope && vibrato == b.vibrato;
  }
  constexpr bool operator!=(Instrument b) const {
    return envelope != b.envelope || vibrato != b.vibrato;
  }

  inline operator std::string() const {
    std::stringstream stream;
    stream << "envelope " << std::string(envelope) << " vibrato "
           << std::string(vibrato);
    return stream.str();
  }
};

const static Instrument instruments[] = {
    {.envelope = ADSR::constant(EnvelopeLevel(1)), .vibrato = Vibrato::none()},
    {.envelope = {30_ms, 20_ms, EnvelopeLevel(50), 40_ms, CurveType::Exp},
     .vibrato = {}},
    {.envelope = {30_ms, 20_ms, EnvelopeLevel(50), 40_ms, CurveType::Exp},
     .vibrato = {1_hz, 2_hz}},
    {.envelope = {30_ms, 20_ms, EnvelopeLevel(50), 40_ms, CurveType::Exp},
     .vibrato = {1_hz, 3_hz}},
};

const inline Instrument &default_instrument() { return instruments[0]; }
