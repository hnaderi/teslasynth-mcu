#pragma once

#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
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
      Envelope(ADSR{0_ns, 0_ns, EnvelopeLevel(0), 0_ns, CurveType::Lin});
  Vibrato _vibrato;
  NotePulse _pulse;
  Duration _release, _max_on_time, _duty;
  bool _active = false;
  bool _released = false;

public:
  void start(const MidiNote &mnote, Duration time, const Instrument &instrument,
             const Config &config);
  void start(const MidiNote &mnote, Duration time, Envelope env,
             const Config &config);
  void start(const MidiNote &mnote, Duration time, Envelope env,
             Vibrato vibrato, const Config &config);
  void release(Duration time);

  bool next();
  const NotePulse &current() const { return _pulse; }

  bool is_active() const { return _active; }
  bool is_released() const { return _released; }
  const Duration &now() const { return _pulse.end; }
  const Hertz &frequency() const { return _freq; }
};

class Notes {
  size_t _size;
  std::array<Note, Config::max_notes> _notes;
  std::array<uint8_t, Config::max_notes> _numbers;

public:
  Notes();
  Notes(size_t size);
  Notes(const Config &config);
  Note &start(const MidiNote &mnote, Duration time,
              const Instrument &instrument, const Config &config);
  void release(uint8_t number, Duration time);

  Note &next();

  size_t active() const;
  size_t size() const { return _size; }
};

template <class N = Notes> class SynthChannel {
  uint8_t _instrument = 0;
  N &_notes;
  const Config &_config;
  const Instrument *_instruments;
  const size_t _instruments_size;

public:
  SynthChannel(const Config &config, N &notes, const Instrument *instruments,
               size_t instruments_size)
      : _notes(notes), _config(config), _instruments(instruments),
        _instruments_size(instruments_size) {}

  SynthChannel(const Config &config, N &notes)
      : SynthChannel(config, notes, NULL, 0) {}

  void on_note_on(const MidiNote &mnote, Duration time) {
    if (mnote.velocity == 0)
      on_note_off(mnote, time);
    else {
      const Instrument &inst = _instrument < _instruments_size
                                   ? _instruments[_instrument]
                                   : default_instrument();
      _notes.start(mnote, time, inst, _config);
    }
  }
  void on_note_off(const MidiNote &mnote, Duration time) {
    _notes.release(mnote.number, time);
  }
  void on_program_change(uint8_t value) { _instrument = value; }

  uint8_t instrument() const { return _instrument; }
};
