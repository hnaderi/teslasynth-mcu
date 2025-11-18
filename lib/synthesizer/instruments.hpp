#pragma once

#include "envelope.hpp"
#include "lfo.hpp"

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
    std::string stream = "envelope " + std::string(envelope) + " vibrato " +
                         std::string(vibrato);
    return stream;
  }
};

const static Instrument instruments[] = {
    // lead
    {.envelope = ADSR::constant(EnvelopeLevel(1)), .vibrato = Vibrato::none()},

    {.envelope = {10_ms, 5_ms, EnvelopeLevel(0.80), 20_ms, CurveType::Exp},
     .vibrato = {2_hz, 1_hz}},
    {.envelope = {5_ms, 10_ms, EnvelopeLevel(0.60), 15_ms, CurveType::Lin},
     .vibrato = {3_hz, 2_hz}},
    {.envelope = {15_ms, 5_ms, EnvelopeLevel(1), 10_ms, CurveType::Const}, // good
     .vibrato = {4_hz, 2_hz}},
    {.envelope = {10_ms, 10_ms, EnvelopeLevel(1), 10_ms, CurveType::Const},
     .vibrato = {5_hz, 1_hz}},
    {.envelope = {10_ms, 5_ms, EnvelopeLevel(1), 5_ms, CurveType::Exp}, // good
     .vibrato = {7_hz, 3_hz}},
    // end lead
    // percussive
    {.envelope = {5_ms, 5_ms, EnvelopeLevel(0.30), 5_ms, CurveType::Lin},
     .vibrato = Vibrato::none()},
    {.envelope = {20_ms, 10_ms, EnvelopeLevel(0.95), 20_ms, CurveType::Const},
     .vibrato = {6_hz, 2_hz}},
    {.envelope = {25_ms, 10_ms, EnvelopeLevel(0.70), 15_ms, CurveType::Const},
     .vibrato = {2_hz, 5_hz}},
    {.envelope = {15_ms, 15_ms, EnvelopeLevel(0.90), 15_ms, CurveType::Lin},
     .vibrato = {3_hz, 1_hz}},
    {.envelope = {5_ms, 15_ms, EnvelopeLevel(0.85), 25_ms, CurveType::Lin},
     .vibrato = {1_hz, 0.5_hz}},
    // end percussive
    // bass
    {.envelope = {50_ms, 10_ms, EnvelopeLevel(0.90), 60_ms, CurveType::Lin},
     .vibrato = {0.5_hz, 1_hz}},
    {.envelope = {35_ms, 25_ms, EnvelopeLevel(0.75), 45_ms, CurveType::Lin},
     .vibrato = {3_hz, 3_hz}},
    {.envelope = {45_ms, 15_ms, EnvelopeLevel(0.55), 35_ms, CurveType::Lin},
     .vibrato = {1_hz, 4_hz}},
    {.envelope = {20_ms, 20_ms, EnvelopeLevel(0.80), 20_ms, CurveType::Exp},
     .vibrato = {4_hz, 4_hz}},
    // end bass
    // pad
    {.envelope = {20_ms, 30_ms, EnvelopeLevel(0.70), 40_ms, CurveType::Exp},
     .vibrato = {1_hz, 3_hz}},
    {.envelope = {60_ms, 40_ms, EnvelopeLevel(0.50), 70_ms, CurveType::Exp},
     .vibrato = {1_hz, 5_hz}},
    {.envelope = {40_ms, 20_ms, EnvelopeLevel(0.60), 30_ms, CurveType::Exp},
     .vibrato = {2_hz, 2_hz}},
    {.envelope = {30_ms, 30_ms, EnvelopeLevel(0.65), 30_ms, CurveType::Exp},
     .vibrato = {2_hz, 6_hz}},
    {.envelope = {25_ms, 20_ms, EnvelopeLevel(0.40), 30_ms, CurveType::Exp},
     .vibrato = {2_hz, 4_hz}},
    // end pad

};

const inline Instrument &default_instrument() { return instruments[0]; }
