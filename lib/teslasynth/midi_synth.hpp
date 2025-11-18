#pragma once

#include "../midi/midi_core.hpp"
#include "../synthesizer/notes.hpp"
#include "core.hpp"
#include <algorithm>
#include <cstdint>

class TrackState {
  Duration _started, _received, _played;
  bool _playing = false;

public:
  constexpr bool is_playing() const { return _playing; }
  constexpr Duration received_time() const { return _received; }
  constexpr Duration started_time() const { return _started; }
  constexpr Duration played_time() const { return _played; }

  /**
   * Stops and resets both track's clocks
   */
  void stop() {
    _playing = false;
    _started = _received = _played = Duration::zero();
  }

  /**
   * Advances the receive clock, starts playing if not already playing
   *
   * @param time Absolute current time
   * @return the absolute time of relative to track start time
   */
  Duration on_receive(Duration time) {
    if (!_playing) {
      _playing = true;
      _started = time;
    }

    if (auto d = time - _started) {
      _received = *d;
      return _received;
    }
    return Duration::zero();
  }

  /**
   * Advances the playback clock if the track is already playing
   *
   * @param delta The time to add to the current clock
   * @return the amount of time that added to the clock
   */
  Duration on_play(Duration delta) {
    if (!_playing) {
      return Duration::zero();
    }

    _played += delta;
    return delta;
  }
};

template <class N = Notes, class TR = TrackState> class SynthChannel {
  uint8_t _instrument = 0;
  N &_notes;
  TR &_track;
  const Config &_config;
  const Instrument *_instruments;
  const size_t _instruments_size;

public:
  SynthChannel(const Config &config, N &notes, TR &track,
               const Instrument *instruments, size_t instruments_size)
      : _notes(notes), _track(track), _config(config),
        _instruments(instruments), _instruments_size(instruments_size) {}

  SynthChannel(const Config &config, N &notes, TR &track)
      : SynthChannel(config, notes, track, NULL, 0) {}

  void handle(MidiChannelMessage msg, Duration time) {
    switch (msg.type) {
    case MidiMessageType::NoteOff: {
      if (_track.is_playing()) {
        Duration delta = _track.on_receive(time);
        _notes.release(msg.data0, delta);
      }
    } break;
    case MidiMessageType::NoteOn: {
      Duration delta = _track.on_receive(time);
      _notes.start({msg.data0, msg.data1}, delta, instrument(), _config);
    } break;
    case MidiMessageType::AfterTouchPoly:
      break;
    case MidiMessageType::ControlChange:
      switch (static_cast<ControlChange>(msg.data0.value)) {
      case ControlChange::ALL_SOUND_OFF:
      case ControlChange::RESET_ALL_CONTROLLERS:
      case ControlChange::ALL_NOTES_OFF:
        _track.stop();
        _notes.off();
        break;
      default:
        break;
      }
      break;
    case MidiMessageType::ProgramChange:
      change_instrument(msg.data0);
      break;
    case MidiMessageType::AfterTouchChannel:
      break;
    case MidiMessageType::PitchBend:
      break;
    }
  }

  inline void change_instrument(uint8_t n) {
    _instrument = std::min<uint8_t>(_instruments_size, std::max<uint8_t>(0, n));
  }

  inline constexpr uint8_t instrument_number() const {
    return _config.instrument.value_or(_instrument);
  }

  inline const Instrument &instrument() const {
    if (_instruments != nullptr)
      return _instruments[instrument_number()];
    else
      return default_instrument();
  }
};

struct Pulse {
  Duration32 on, off;

  constexpr bool is_zero() const { return on.is_zero(); }
  constexpr Duration32 length() const { return on + off; }

  inline operator std::string() const {
    return std::string("Pulse[on:") + std::string(on) +
           ", off:" + std::string(off) + "]";
  }
};

template <class N = Notes, class TR = TrackState> class Sequencer {
  const Config &config_;
  N &notes_;
  TrackState &track_;

public:
  Sequencer(const Config &config, N &notes, TR &track)
      : config_(config), notes_(notes), track_(track) {}

  Pulse sample(Duration max) {
    Pulse res;

    Note *note = &notes_.next();
    Duration next_edge = note->current().start;
    while (next_edge < track_.played_time() && note->is_active()) {
      note->next();
      note = &notes_.next();
      next_edge = note->current().start;
    }

    Duration target = track_.played_time() + max;
    if (!note->is_active() || next_edge > target || !track_.is_playing()) {
      res.off = max;
      track_.on_play(max);
    } else if (next_edge == track_.played_time()) {
      res.on = note->current().volume * config_.max_on_time;
      res.off = config_.min_deadtime;
      note->next();
      track_.on_play(res.on + res.off);
    } else if (next_edge <= target && next_edge >= track_.played_time()) {
      res.off = *(next_edge - track_.played_time());
      track_.on_play(res.off);
    }
    return res;
  }
};
