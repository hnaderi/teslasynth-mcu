#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "synth.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

void Note::start(const MidiNote &mnote, Duration time, Envelope env,
                 Vibrato vibrato, const Config &config) {
  if (_active && mnote.velocity == 0)
    return release(time);
  _freq = mnote.frequency(config);
  _max_on_time = mnote.volume() * config.max_on_time;
  _envelope = env;
  _vibrato = vibrato;
  _active = true;
  _duty = _max_on_time * _envelope.update(0_us, true);
  _now = time;
  next();
}

void Note::start(const MidiNote &mnote, Duration time, Envelope env,
                 const Config &config) {
  start(mnote, time, env, Vibrato::none(), config);
}
void Note::start(const MidiNote &mnote, Duration time,
                 const Instrument &instrument, const Config &config) {
  start(mnote, time, instrument.envelope, instrument.vibrato, config);
}

void Note::release(Duration time) {
  _released = true;
  _release = time;
}

void Note::off() { _active = false; }

bool Note::next() {
  if (_envelope.is_off())
    _active = false;
  if (_active) {
    Duration period = (_freq + _vibrato.offset(now())).period();
    _pulse.start = _now;
    _pulse.duty = _duty;
    _pulse.period = period;
    _now += period;
    _duty = _max_on_time *
            _envelope.update(period, !_released || now() <= _release);
  }
  return _active;
}

Notes::Notes() : _size(Config::max_notes) {}
Notes::Notes(size_t size) : _size(std::min(size, Config::max_notes)) {}
Notes::Notes(const Config &config)
    : _size(std::min(config.notes, config.max_notes)) {}

size_t Notes::active() const {
  size_t active = 0;
  for (size_t i = 0; i < _size; i++) {
    if (_notes[i].is_active())
      active++;
  }
  return active;
}

Note &Notes::next() {
  auto out = 0;
  auto min = _notes[0].current().start;
  for (size_t i = 1; i < _size; i++) {
    if (!_notes[i].is_active())
      continue;
    auto time = _notes[i].current().start;
    if (time < min) {
      out = i;
      min = time;
    }
  }
  return _notes[out];
}

Note &Notes::start(const MidiNote &mnote, Duration time,
                   const Instrument &instrument, const Config &config) {
  size_t idx = 0;
  for (uint8_t i = 0; i < _size; i++) {
    if (_notes[i].is_active() && _numbers[i] != mnote.number)
      continue;
    idx = i;
    break;
  }
  _notes[idx].start(mnote, time, instrument, config);
  _numbers[idx] = mnote.number;
  return _notes[idx];
}

void Notes::release(uint8_t number, Duration time) {
  for (uint8_t i = 0; i < _size; i++) {
    if (_notes[i].is_active() && _numbers[i] == number) {
      _notes[i].release(time);
      return;
    }
  }
}

void Notes::off() {
  for (uint8_t i = 0; i < _size; i++)
    _notes[i].off();
}
