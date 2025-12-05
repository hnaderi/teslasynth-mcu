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

#ifndef CONFIG_DEFAULT_MAX_DUTY
#define CONFIG_DEFAULT_MAX_DUTY 100
#endif

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

template <std::uint8_t OUTPUTS = 1, std::size_t SIZE = 64> struct PulseBuffer {
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

/**
 * A numeric value representing duty cycle.
 * Max value: 100%
 * Resolution: 0.5%
 */
class DutyCycle {
  constexpr static auto max_duty = 100, max_value = 200;
  uint8_t value_;

  template <typename T> constexpr static uint8_t validate(T v) {
    if (v >= max_duty)
      return max_value;
    if (v <= 0)
      return 0;
    return static_cast<float>(v) / max_duty * max_value;
  }

public:
  constexpr DutyCycle() : value_(0) {}

  template <typename T>
  explicit constexpr DutyCycle(T value) : value_(validate(value)) {}

  constexpr bool is_max() const { return value_ == max_value; }
  constexpr bool is_zero() const { return value_ == 0; }
  constexpr static DutyCycle max() { return DutyCycle(max_duty); }
  constexpr static DutyCycle min() { return DutyCycle(0); }
  constexpr uint8_t value() const { return value_; }
  constexpr uint8_t inverse() const { return max_value - value_; }
  constexpr operator float() const {
    return value_ / static_cast<float>(max_value);
  }
  inline operator std::string() const {
    return std::to_string(static_cast<float>(value_) / max_value * max_duty) +
           "%";
  }
};

struct Config {
  static constexpr uint8_t max_notes = CONFIG_MAX_NOTES;
  static constexpr float default_max_duty = CONFIG_DEFAULT_MAX_DUTY;

  Duration16 max_on_time = 100_us, min_deadtime = 100_us, duty_window = 10_ms;
  uint8_t notes = max_notes;
  DutyCycle max_duty = DutyCycle(CONFIG_DEFAULT_MAX_DUTY);
  std::optional<uint8_t> instrument = {};

  inline operator std::string() const {
    return std::string("Concurrent notes: ") + std::to_string(notes) +
           "\nMax on time: " + std::string(max_on_time) +
           "\nMin deadtime: " + std::string(min_deadtime) +
           "\nMax duty: " + std::string(max_duty) +
           "\nDuty window: " + std::string(duty_window) +
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

template <std::uint8_t OUTPUTS = 1> struct Configuration {
  SynthConfig synth_config;
  std::array<Config, OUTPUTS> channel_configs{};

  constexpr Configuration() {}
  constexpr Configuration(const SynthConfig &synth_config,
                          const std::array<Config, OUTPUTS> &channel_configs)
      : synth_config(synth_config), channel_configs(channel_configs) {}
  constexpr Configuration(const SynthConfig &synth_config)
      : synth_config(synth_config) {}
  constexpr Configuration(const std::array<Config, OUTPUTS> &channel_configs)
      : channel_configs(channel_configs) {}

  SynthConfig &synth() { return synth_config; }
  Config &channel(uint8_t ch) { return channel_configs[ch]; }
};

class DutyLimiter {
  uint16_t max_budget_ = 0, budget_ = 0, replenishing_ = 0;
  DutyCycle duty_;

public:
  DutyLimiter() : duty_(DutyCycle::max()) {}
  DutyLimiter(const DutyCycle &duty, const Duration16 window = 10_ms)
      : max_budget_(window.micros() * duty), budget_(max_budget_), duty_(duty) {
  }
  DutyLimiter(const Config &config) : DutyLimiter(config.max_duty) {}

  bool can_use(const Duration16 &on) {
    if (duty_.is_max())
      return true;
    if (on.micros() <= budget_) {
      budget_ -= on.micros();
      return true;
    }
    return false;
  }

  void replenish(const Duration16 &off) {
    uint32_t total = replenishing_ + (off.micros() * duty_);
    if (total >= max_budget_) {
      budget_ = max_budget_;
      replenishing_ = 0;
    } else {
      replenishing_ = total;
    }
  }

  constexpr Duration16 budget() const { return Duration16::micros(budget_); }
};

template <std::uint8_t OUTPUTS = 1, class N = Voice<>> class Teslasynth {
  Configuration<OUTPUTS> config_;
  TrackState<OUTPUTS> _track;
  Instrument const *_instruments = instruments.begin();
  size_t _instruments_size = instruments.size();
  std::array<uint8_t, OUTPUTS> current_instrument_{};
  std::array<N, OUTPUTS> _voices;
  std::array<DutyLimiter, OUTPUTS> _limiters;

public:
  Teslasynth(
      const Configuration<OUTPUTS> &config,
      TrackStateCallback onPlaybackChanged = [](bool) {})
      : config_(config), _track(onPlaybackChanged) {
    reload_config();
  }

  Teslasynth(TrackStateCallback onPlaybackChanged = [](bool) {})
      : Teslasynth(Configuration<OUTPUTS>(), onPlaybackChanged) {}

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
        off();
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

  inline void off() {
    _track.stop();
    for (auto &note : _voices) {
      note.off();
    }
  }

  inline void reload_config() {
    if (_track.is_playing())
      off();
    for (auto i = 0; i < OUTPUTS; i++) {
      _voices[i].adjust_size(config_.channel(i).notes);
      _limiters[i] = DutyLimiter(config_.channel(i).max_duty,
                                 config_.channel(i).duty_window);
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
    return config_.channel_configs[ch].instrument.value_or(
        config_.synth_config.instrument.value_or(current_instrument_[ch]));
  }

  inline const Instrument &instrument(uint8_t ch) const {
    const auto nr = instrument_number(ch);
    return nr < _instruments_size ? _instruments[nr] : default_instrument();
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
    } else if (next_edge == _track.played_time(ch)) {
      res.on = note->current().volume * config_.channel_configs[ch].max_on_time;
      res.off = config_.channel_configs[ch].min_deadtime;
      note->next();
    } else if (next_edge <= target && next_edge >= _track.played_time(ch)) {
      res.off = Duration16::micros(next_edge.micros() -
                                   _track.played_time(ch).micros());
    }

    if (!_limiters[ch].can_use(res.on)) {
      res.off += res.on;
      res.on = 0_us;
    }

    _limiters[ch].replenish(res.off);
    _track.on_play(ch, res.length());

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
