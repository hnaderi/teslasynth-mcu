#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "synth.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

void Note::start(const MidiNote &mnote, Duration time, Envelope env,
                 const Config &config) {
  _freq = mnote.frequency(config);
  _max_on_time = mnote.volume() * config.max_on_time;
  _envelope = env;
  _active = true;
  _duty = _max_on_time * _envelope.update(0_us, true);
  _pulse.end = time;
  next();
}
void Note::start(const MidiNote &mnote, Duration time,
                 const Instrument &instrument, const Config &config) {
  start(mnote, time, instrument.envelope, config);
}

void Note::release(Duration time) {
  _released = true;
  _release = time;
}

bool Note::next() {
  if (_envelope.is_off())
    _active = false;
  if (_active) {
    Duration period = _freq.period();
    _pulse.start = now();
    _pulse.off = now() + _duty;
    _pulse.end = now() + period;
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
    if (notes[i].is_active())
      active++;
  }
  return active;
}

Note &Notes::next() {
  auto out = 0;
  auto min = notes[0].current().start;
  for (size_t i = 1; i < _size; i++) {
    if (!notes[i].is_active())
      continue;
    auto time = notes[i].current().start;
    if (time < min) {
      out = i;
      min = time;
    }
  }
  return notes[out];
}

Note &Notes::start(const MidiNote &mnote, Duration time,
                   const Instrument &instrument) {
  size_t idx = 0;
  // for (uint8_t i = 0; i < _size; i++) {
  //   if (notes[i].is_active() && notes[i].number() != number)
  //     continue;
  //   idx = i;
  //   break;
  // }
  // notes[idx].start(number, velocity, time, instrument);
  return notes[idx];
}

void Notes::release(uint8_t number, Duration time) {
  for (uint8_t i = 0; i < _size; i++) {
    // if (notes[i].is_active() && notes[i].number() == number) {
    notes[i].release(time);
    // return;
    // }
  }
}
