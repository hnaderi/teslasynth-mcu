#pragma once

#include "../midi/midi_core.hpp"
#include "../synthesizer/notes.hpp"
#include "core.hpp"
#include "instruments.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace teslasynth::midisynth {
using TrackStateCallback = std::function<void(bool)>;

using namespace teslasynth::synth;
using namespace teslasynth::midi;

template <unsigned int OUTPUTS = 1> class TrackState {
  Duration _started;
  std::array<Duration, OUTPUTS> _received, _played;
  bool _playing = false;
  TrackStateCallback _cb;

public:
  TrackState(TrackStateCallback cb = [](bool) {}) : _cb(cb) {}
  constexpr bool is_playing() const { return _playing; }
  constexpr Duration started_time() const { return _started; }
  constexpr Duration received_time(uint8_t ch) const { return _received[ch]; }
  constexpr Duration played_time(uint8_t ch) const { return _played[ch]; }

  /**
   * Stops and resets both track's clocks
   */
  void stop() {
    _playing = false;
    _started = Duration::zero();
    for (uint8_t i = 0; i < OUTPUTS; i++) {
      _received[i] = _played[i] = Duration::zero();
    }
    _cb(_playing);
  }

  /**
   * Advances the receive clock, starts playing if not already playing
   *
   * @param time Absolute current time
   * @return the absolute time of relative to track start time
   */
  Duration on_receive(uint8_t ch, Duration time) {
    if (!_playing) {
      _playing = true;
      _started = time;
      _cb(_playing);
    }

    if (auto d = time - _started) {
      _received[ch] = *d;
      return _received[ch];
    }
    return Duration::zero();
  }

  /**
   * Advances the playback clock if the track is already playing
   *
   * @param delta The time to add to the current clock
   * @return the amount of time that added to the clock
   */
  Duration on_play(uint8_t ch, Duration delta) {
    if (!_playing) {
      return Duration::zero();
    }

    _played[ch] += delta;
    return delta;
  }
};

struct Pulse {
  Duration16 on, off;

  constexpr bool is_zero() const { return on.is_zero(); }
  constexpr Duration32 length() const { return on + off; }

  inline operator std::string() const {
    return std::string("Pulse[on:") + std::string(on) +
           ", off:" + std::string(off) + "]";
  }
};

template <std::uint8_t OUTPUTS = 1, std::size_t SIZE = 10> struct PulseBuffer {
  std::array<uint8_t, OUTPUTS> written{};
  std::array<Pulse, SIZE * OUTPUTS> pulses;

  inline void clean() {
    for (uint8_t ch = 0; ch < OUTPUTS; ch++) {
      written[ch] = 0;
    }
  }
  inline Pulse &at(uint8_t ch, uint8_t idx) {
    assert(ch < OUTPUTS);
    return pulses[ch * SIZE + idx];
  }
  inline Pulse &data(uint8_t ch) {
    assert(ch < OUTPUTS);
    return pulses[ch * SIZE];
  }
  inline uint8_t &data_size(uint8_t ch) {
    assert(ch < OUTPUTS);
    return written[ch];
  }
};

template <std::uint8_t OUTPUTS = 1> struct Configuration {
  SynthConfig synth_config;
  std::array<Config, OUTPUTS> channel_configs{};

  Configuration() {}
  Configuration(const SynthConfig &synth_config,
                const std::array<Config, OUTPUTS> &channel_configs)
      : synth_config(synth_config), channel_configs(channel_configs) {}
  Configuration(const SynthConfig &synth_config) : synth_config(synth_config) {}
  Configuration(const std::array<Config, OUTPUTS> &channel_configs)
      : channel_configs(channel_configs) {}

  const SynthConfig &synth() const { return synth_config; }
  const Config &channel(uint8_t ch) const { return channel_configs[ch]; }
};

template <std::uint8_t OUTPUTS = 1, class N = Voice<>> class Teslasynth {
  Configuration<OUTPUTS> config_;
  TrackState<OUTPUTS> _track;
  Instrument const *_instruments = instruments.begin();
  size_t _instruments_size = instruments.size();
  std::array<uint8_t, OUTPUTS> current_instrument_{};
  std::array<N, OUTPUTS> _voices;

public:
  Teslasynth(TrackStateCallback onPlaybackChanged = [](bool) {})
      : Teslasynth(Configuration<>(), onPlaybackChanged) {}

  Teslasynth(
      const Configuration<OUTPUTS> &config,
      TrackStateCallback onPlaybackChanged = [](bool) {})
      : config_(config), _track(onPlaybackChanged) {
    for (auto i = 0; i < OUTPUTS; i++) {
      _voices[i].adjust_size(config.channel(i).notes);
    }
  }

  Configuration<> &configuration() { return config_; }

  template <std::size_t INSTRUMENTS>
  void use_instruments(const std::array<Instrument, INSTRUMENTS> &instruments) {
    _instruments = instruments.begin();
    _instruments_size = instruments.size();
  }

  void handle(MidiChannelMessage msg, Duration time) {
    switch (msg.type) {
    case MidiMessageType::NoteOff:
      note_off(msg.channel, msg.data0, time);
      break;
    case MidiMessageType::NoteOn:
      note_on(msg.channel, msg.data0, msg.data1, time);
      break;
    case MidiMessageType::AfterTouchPoly:
      break;
    case MidiMessageType::ControlChange:
      switch (static_cast<ControlChange>(msg.data0.value)) {
      case ControlChange::ALL_SOUND_OFF:
      case ControlChange::RESET_ALL_CONTROLLERS:
      case ControlChange::ALL_NOTES_OFF:
        _track.stop();
        for (auto &note : _voices) {
          note.off();
        }
        break;
      default:
        break;
      }
      break;
    case MidiMessageType::ProgramChange:
      change_instrument(msg.channel, msg.data0);
      break;
    case MidiMessageType::AfterTouchChannel:
      break;
    case MidiMessageType::PitchBend:
      break;
    }
  }

  inline void change_instrument(uint8_t ch, uint8_t n) {
    if (ch < OUTPUTS) {
      current_instrument_[ch] =
          std::min<uint8_t>(_instruments_size, std::max<uint8_t>(0, n));
    }
  }

  inline constexpr uint8_t instrument_number(uint8_t ch) const {
    assert(ch < OUTPUTS);
    return config_.synth_config.instrument.value_or(current_instrument_[ch]);
  }

  inline const Instrument &instrument(uint8_t ch) const {
    return _instruments[instrument_number(ch)];
  }

  inline void note_off(uint8_t ch, uint8_t number, Duration time) {
    if (_track.is_playing()) {
      if (ch < OUTPUTS) {
        Duration delta = _track.on_receive(ch, time);
        _voices[ch].release(number, delta);
      }
    }
  }
  inline void note_off(uint8_t ch, MidiNote mnote, Duration time) {
    note_off(ch, mnote.number, time);
  }

  inline void note_on(uint8_t ch, uint8_t number, uint8_t velocity,
                      Duration time) {
    if (ch < OUTPUTS) {
      Duration delta = _track.on_receive(ch, time);
      _voices[ch].start({number, velocity}, delta, instrument(ch),
                        config_.synth().a440);
    }
  }
  inline void note_on(uint8_t ch, MidiNote mnote, Duration time) {
    note_on(ch, mnote.number, mnote.velocity, time);
  }

  Pulse sample(uint8_t ch, Duration16 max) {
    Pulse res;

    Note *note = &_voices[ch].next();
    Duration next_edge = note->current().start;
    while (next_edge < _track.played_time(ch) && note->is_active()) {
      note->next();
      note = &_voices[ch].next();
      next_edge = note->current().start;
    }

    Duration target = _track.played_time(ch) + max;
    if (!note->is_active() || next_edge > target || !_track.is_playing()) {
      res.off = max;
      _track.on_play(ch, max);
    } else if (next_edge == _track.played_time(ch)) {
      res.on = note->current().volume * config_.channel_configs[ch].max_on_time;
      res.off = config_.channel_configs[ch].min_deadtime;
      note->next();
      _track.on_play(ch, res.on + res.off);
    } else if (next_edge <= target && next_edge >= _track.played_time(ch)) {
      res.off = Duration16::micros(next_edge.micros() -
                                   _track.played_time(ch).micros());
      _track.on_play(ch, res.off);
    }
    return res;
  }

  template <size_t BUFSIZE>
  void sample_all(Duration16 max, PulseBuffer<OUTPUTS, BUFSIZE> &output) {
    if (!_track.is_playing()) {
      output.clean();
      return;
    }
    int16_t now = max.micros();
    for (uint8_t ch = 0; ch < OUTPUTS; ch++) {
      int16_t processed = 0;
      uint8_t i = 0, start = ch * BUFSIZE;
      for (; processed < now && i < BUFSIZE; i++) {
        auto left = Duration16::micros(now - processed);
        output.pulses[start + i] = sample(ch, left);
        processed += output.pulses[start + i].length().micros();
      }
      output.written[ch] = i;
    }
  }

  const TrackState<OUTPUTS> &track() const { return _track; }
  const N &voice(uint8_t i = 0) const {
    assert(i < OUTPUTS);
    return _voices[i];
  }
};

} // namespace teslasynth::midisynth
