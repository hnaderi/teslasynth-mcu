#pragma once

#include "../midi/midi_core.hpp"
#include "../synthesizer/synth.hpp"
#include <cstdint>

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

  void handle(MidiChannelMessage msg, Duration time) {
    switch (msg.type) {
    case MidiMessageType::NoteOff:
      _notes.release(msg.data0, time);
      break;
    case MidiMessageType::NoteOn:
      _notes.start({msg.data0, msg.data1}, time, instrument(), _config);
      break;
    case MidiMessageType::AfterTouchPoly:
      break;
    case MidiMessageType::ControlChange:
      break;
    case MidiMessageType::ProgramChange:
      _instrument = msg.data0;
      break;
    case MidiMessageType::AfterTouchChannel:
      break;
    case MidiMessageType::PitchBend:
      break;
    }
  }

  uint8_t instrument_number() const { return _instrument; }

  inline const Instrument &instrument() const {
    return _instrument < _instruments_size ? _instruments[_instrument]
                                           : default_instrument();
  }
};

template <class N = Notes> class Sequencer {
  N &notes_;
  Duration clock_ = Duration::zero();
  const Config &config_;

public:
  Sequencer(const Config &config, N &notes) : config_(config), notes_(notes) {}

  constexpr Duration clock() const { return clock_; }

  NotePulse sample(Duration max) {
    NotePulse res;
    res.start = clock_;

    Note *note = &notes_.next();
    Duration next_edge = note->current().start;
    Duration target = clock_ + max;
    if (!note->is_active() || next_edge > target) {
      res.period = max;
      clock_ = target;
    } else if (next_edge == clock_) {
      res = note->current();
      note->next();
      res.period = res.duty + config_.min_deadtime;
      clock_ += res.period;
    } else if (next_edge < target) {
      res.period = next_edge;
      clock_ = next_edge;
    }
    return res;
  }
};
