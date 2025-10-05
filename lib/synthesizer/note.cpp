#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "synth.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

Note::Note()
    : _envelope(
          Envelope(ADSR{0_ns, 0_ns, EnvelopeLevel(0), 0_ns, CurveType::Lin})) {}
Note::Note(uint8_t number, uint8_t velocity, Duration time, Envelope env)
    : _number(number), _velocity(velocity), _on(time), _now(time),
      _envelope(env), _started(true), _active(true) {}

Note::Note(uint8_t number, uint8_t velocity, Duration time, uint8_t instrument)
    : Note(number, velocity, time, instruments[instrument].envelope) {}

void Note::release(Duration time) {
  _released = true;
  _release = time;
}
bool Note::tick(const Config &config, NotePulse &out) {
  bool active = _active;
  if (_active) {
    Hertz freq = config.a440 * std::expf((_number - 69) / 12.0f);
    Duration period = freq.period();
    Duration level = config.max_on_time * (_velocity / 127.f);
    Duration duty = std::min(config.max_on_time, level);

    out.start = _now;
    out.off = _now + duty;
    out.end = _now + period;

    _now += period;
  }
  return active;
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
  auto min = notes[0].time();
  for (int i = 1; i < _size; i++) {
    if (!notes[i].is_active())
      continue;
    auto time = notes[i].time();
    if (time < min) {
      out = i;
      min = time;
    }
  }
  return notes[out];
}

Note &Notes::start(uint8_t number, uint8_t velocity, uint8_t instrument,
                   Duration time) {
  size_t idx = 0;
  for (uint8_t i = 0; i < _size; i++) {
    if (notes[i].is_active() && notes[i].number() != number)
      continue;
    idx = i;
    break;
  }
  notes[idx] = Note(number, velocity, time, instrument);
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
