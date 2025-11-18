#pragma once

#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#ifndef CONFIG_MAX_NOTES
#define CONFIG_MAX_NOTES 4
#endif

struct Config {
  static constexpr uint8_t max_notes = CONFIG_MAX_NOTES;

  Duration32 min_on_time = Duration(), max_on_time = 100_us,
             min_deadtime = 100_us;
  Hertz a440 = 440_hz;
  uint8_t notes = max_notes;
  std::optional<uint8_t> instrument = {};

  inline operator std::string() const {
    return std::string("Concurrent notes: ") + std::to_string(notes) +
           "\nTuning: " + std::string(a440) +
           "\nMin on time: " + std::string(min_on_time) +
           "\nMax on time: " + std::string(max_on_time) +
           "\nMin deadtime: " + std::string(min_deadtime) +
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

  constexpr Hertz frequency(const Config &config) const {
    return frequency(config.a440);
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
             const Config &config);
  void start(const MidiNote &mnote, Duration time, Envelope env,
             const Config &config);
  void start(const MidiNote &mnote, Duration time, Envelope env,
             Vibrato vibrato, const Config &config);
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

class Notes {
  size_t _size;
  std::array<Note, Config::max_notes> _notes;
  std::array<uint8_t, Config::max_notes> _numbers;

public:
  Notes();
  Notes(uint8_t size);
  Notes(const Config &config);
  Note &start(const MidiNote &mnote, Duration time,
              const Instrument &instrument, const Config &config);
  void release(uint8_t number, Duration time);
  inline void release(const MidiNote &mnote, Duration time) {
    release(mnote.number, time);
  }
  void off();

  Note &next();

  size_t active() const;
  size_t size() const { return _size; }
};
