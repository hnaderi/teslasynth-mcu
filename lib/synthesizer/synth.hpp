#pragma once

#include "core.hpp"
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

class Note {
  uint8_t _number = 0, _velocity = 0;
  Duration _on, _release, _now;
  uint8_t instrument;
  bool _started = false;
  bool _active = false;
  bool _released = false;

public:
  Note();
  Note(uint8_t number, uint8_t velocity, uint8_t instrument, Duration time);
  void release(Duration time);
  bool tick(const Config &config, NotePulse &out);

  bool is_active() const { return _active; }
  bool is_released() const { return _released; }
  bool is_started() const { return _started; }
  Duration time() const { return _now; }
  uint8_t number() const { return _number; }
  uint8_t velocity() const { return _velocity; }
};

class Notes {
  const size_t _size;
  std::array<Note, Config::max_notes> notes;

public:
  Notes();
  Notes(size_t size);
  Notes(const Config &config);
  Note &next();
  Note &start(uint8_t number, uint8_t velocity, uint8_t instrument,
              Duration time);
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
  uint16_t render(Pulse *buffer, uint16_t max_size, uint16_t delta);
};
