#pragma once

#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#ifndef CONFIG_MAX_NOTES
#define CONFIG_MAX_NOTES 4
#endif

namespace teslasynth::synth {
using namespace teslasynth::core;

struct Config {
  static constexpr uint8_t max_notes = CONFIG_MAX_NOTES;

  Duration16 max_on_time = 100_us, min_deadtime = 100_us;
  uint8_t notes = max_notes;
  std::optional<uint8_t> instrument = {};

  inline operator std::string() const {
    return std::string("Concurrent notes: ") + std::to_string(notes) +
           "\nMax on time: " + std::string(max_on_time) +
           "\nMin deadtime: " + std::string(min_deadtime) +
           "\nInstrument: " + (instrument ? std::to_string(*instrument) : "-");
  }
};

struct SynthConfig {
  Hertz a440 = 440_hz;
  std::optional<uint8_t> instrument = {};

  inline operator std::string() const {
    return std::string("Tuning: ") + std::string(a440) +
           "\nInstrument: " + (instrument ? std::to_string(*instrument) : "-");
  }
};

struct NotePulse {
  Duration start;
  Duration32 period;
  EnvelopeLevel volume;

  constexpr bool is_zero() const { return volume.is_zero(); }
  inline operator std::string() const {
    return std::string("Note[start:") + std::string(start) +
           ", vol:" + std::string(volume) + ", period:" + std::string(period) +
           "]";
  }
};

struct MidiNote {
  uint8_t number;
  uint8_t velocity;

  constexpr Hertz frequency(Hertz tuning = 440_hz) const {
    return tuning * exp2f((number - 69) / 12.0f);
  }

  constexpr EnvelopeLevel volume() const {
    return EnvelopeLevel(velocity / 127.f);
  }
  constexpr bool operator==(MidiNote b) const {
    return number == b.number && velocity == b.velocity;
  }
  constexpr bool operator!=(MidiNote b) const {
    return number != b.number || velocity != b.velocity;
  }
};

class Note {
  Hertz _freq = Hertz(0);
  Envelope _envelope =
      Envelope(ADSR{0_us, 0_us, EnvelopeLevel(0), 0_us, CurveType::Lin});
  Vibrato _vibrato;
  NotePulse _pulse;
  EnvelopeLevel _level, _volume;
  Duration _release, _now;
  bool _active = false;
  bool _released = false;

public:
  void start(const MidiNote &mnote, Duration time, Envelope env,
             Vibrato vibrato, Hertz tuning);
  void start(const MidiNote &mnote, Duration time, const Instrument &instrument,
             Hertz tuning);
  void start(const MidiNote &mnote, Duration time, Envelope env, Hertz tuning);
  void release(Duration time);

  void off();

  bool next();
  const NotePulse &current() const { return _pulse; }

  bool is_active() const { return _active; }
  bool is_released() const { return _released; }
  const Duration &now() const { return _now; }
  const Hertz &frequency() const { return _freq; }
  const EnvelopeLevel &max_volume() const { return _volume; }
};

template <std::uint8_t MAX_NOTES = Config::max_notes> class Voice {
  uint8_t _size;
  std::array<Note, MAX_NOTES> _notes;
  std::array<uint8_t, MAX_NOTES> _numbers;

public:
  Voice() : _size(Config::max_notes) {}
  Voice(uint8_t size) : _size(std::min(size, Config::max_notes)) {}
  Voice(const Config &config)
      : _size(std::min(config.notes, config.max_notes)) {}
  Note &start(const MidiNote &mnote, Duration time,
              const Instrument &instrument, Hertz tuning) {
    uint8_t idx = 0;
    for (uint8_t i = 0; i < _size; i++) {
      if (_notes[i].is_active() && _numbers[i] != mnote.number)
        continue;
      idx = i;
      break;
    }
    _notes[idx].start(mnote, time, instrument, tuning);
    _numbers[idx] = mnote.number;
    return _notes[idx];
  }
  void release(uint8_t number, Duration time) {
    for (uint8_t i = 0; i < _size; i++) {
      if (_notes[i].is_active() && _numbers[i] == number) {
        _notes[i].release(time);
        return;
      }
    }
  }
  inline void release(const MidiNote &mnote, Duration time) {
    release(mnote.number, time);
  }
  void off() {
    for (uint8_t i = 0; i < _size; i++)
      _notes[i].off();
  }

  Note &next() {
    uint8_t out = 0;
    Duration min = Duration::max();
    for (uint8_t i = 0; i < _size; i++) {
      if (!_notes[i].is_active())
        continue;
      Duration time = _notes[i].current().start;
      if (time < min) {
        out = i;
        min = time;
      }
    }
    return _notes[out];
  }

  void adjust_size(uint8_t size) {
    if (size <= Config::max_notes && size > 0)
      _size = size;
  }
  uint8_t active() const {
    uint8_t active = 0;
    for (uint8_t i = 0; i < _size; i++) {
      if (_notes[i].is_active())
        active++;
    }
    return active;
  }
  uint8_t size() const { return _size; }
  constexpr uint8_t max_size() const { return MAX_NOTES; }
};

} // namespace teslasynth::synth
