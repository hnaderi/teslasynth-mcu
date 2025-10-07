#pragma once

#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

struct Config {
  static constexpr size_t max_notes = 4;

  Duration min_on_time = Duration(), max_on_time = 100_us,
           min_deadtime = 100_us;
  Hertz a440 = 440_hz;
  size_t notes = max_notes;
};

struct NotePulse {
  Duration start, off, end;
};

struct MidiNote {
  uint8_t number;
  uint8_t velocity;

  constexpr Hertz frequency(const Config &config) const {
    return config.a440 * std::exp2f((number - 69) / 12.0f);
  }

  constexpr EnvelopeLevel volume() const {
    return EnvelopeLevel(velocity / 127.f);
  }
};

class Note {
  Hertz _freq = Hertz(0);
  Envelope _envelope =
      Envelope(ADSR{0_ns, 0_ns, EnvelopeLevel(0), 0_ns, CurveType::Lin});
  NotePulse _pulse;
  Duration _release, _max_on_time, _duty;
  bool _active = false;
  bool _released = false;

public:
  void start(const MidiNote &mnote, Duration time, const Instrument &instrument,
             const Config &config);
  void start(const MidiNote &mnote, Duration time, Envelope env,
             const Config &config);
  void release(Duration time);

  bool next();
  const NotePulse &current() const { return _pulse; }

  bool is_active() const { return _active; }
  bool is_released() const { return _released; }
  const Duration &now() const { return _pulse.end; }
};

class Notes {
  const size_t _size;
  std::array<Note, Config::max_notes> notes;

public:
  Notes();
  Notes(size_t size);
  Notes(const Config &config);
  Note &next();
  Note &start(const MidiNote &mnote, Duration time,
              const Instrument &instrument);
  void release(uint8_t number, Duration time);
  size_t active() const;
};

class SynthChannel {
  uint8_t instrument = 0;
  Notes notes;
  uint8_t playing_notes = 0;
  Duration time;
  const Config &_config;

public:
  SynthChannel(const Config &config);
  void on_note_on(uint8_t number, uint8_t velocity, Duration time);
  void on_note_off(uint8_t number, uint8_t velocity, Duration time);
  void on_program_change(uint8_t value);
  void on_control_change(uint8_t value);
  // uint16_t render(Pulse *buffer, uint16_t max_size, uint16_t delta);
};
