#pragma once

#include "core.hpp"
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace teslasynth::synth {
using namespace teslasynth::core;

constexpr float epsilon = 0.001;
enum CurveType { Lin, Exp, Const };

class EnvelopeLevel {
  float _value;

public:
  constexpr explicit EnvelopeLevel() : _value(0) {}
  constexpr explicit EnvelopeLevel(float level)
      : _value(level > 1   ? 1.f
               : level < 0 ? 0.f
                           : level) {}

  constexpr static EnvelopeLevel zero() { return EnvelopeLevel(); }
  constexpr static EnvelopeLevel max() { return EnvelopeLevel(1); }
  constexpr static EnvelopeLevel logscale(uint8_t value) {
    return EnvelopeLevel(log2f(1.f + value) / 8.f);
  }

  constexpr bool is_zero() const { return _value == 0; }
  constexpr EnvelopeLevel operator+(const EnvelopeLevel &b) const {
    return EnvelopeLevel(_value + b._value);
  }
  constexpr EnvelopeLevel operator+(float b) const {
    return EnvelopeLevel(_value + b);
  }
  EnvelopeLevel &operator+=(const EnvelopeLevel &b) {
    if (1.f - _value < b._value)
      _value = 1.f;
    else
      _value += b._value;
    return *this;
  }
  EnvelopeLevel &operator+=(float b) {
    _value += b;
    if (_value > 1.f)
      _value = 1.f;
    else if (_value < 0)
      _value = 0.f;
    return *this;
  }
  float operator-(const EnvelopeLevel &b) const { return _value - b._value; }

  template <typename T>
  constexpr SimpleDuration<T> operator*(const SimpleDuration<T> &b) const {
    return b * _value;
  }
  constexpr EnvelopeLevel operator*(const EnvelopeLevel &b) const {
    return EnvelopeLevel(b._value * _value);
  }
  constexpr bool operator<(const EnvelopeLevel &b) const {
    return _value < b._value;
  }
  constexpr bool operator>(const EnvelopeLevel &b) const {
    return _value > b._value;
  }
  constexpr bool operator==(const EnvelopeLevel &b) const {
    return std::fabs(_value - b._value) < 1e-3f;
  }
  constexpr bool operator!=(const EnvelopeLevel &b) const {
    return _value != b._value;
  }
  constexpr bool operator<=(const EnvelopeLevel &b) const {
    return _value <= b._value;
  }
  constexpr bool operator>=(const EnvelopeLevel &b) const {
    return _value >= b._value;
  }

  constexpr operator float() const { return _value; }
  inline operator std::string() const { return std::to_string(_value); }
};

struct ADSR {
  Duration attack = Duration::zero();
  Duration decay = Duration::zero();
  EnvelopeLevel sustain = EnvelopeLevel(1);
  Duration release = Duration::zero();
  enum CurveType type = CurveType::Const;

  constexpr static ADSR constant(EnvelopeLevel level) {
    return {0_us, 0_us, level, 0_us, Const};
  }
  constexpr static ADSR linear(Duration attack, Duration decay,
                               EnvelopeLevel sustain, Duration release) {
    return {attack, decay, sustain, release, Lin};
  }
  constexpr static ADSR exponential(Duration attack, Duration decay,
                                    EnvelopeLevel sustain, Duration release) {
    return {attack, decay, sustain, release, Exp};
  }

  constexpr bool operator==(ADSR b) const {
    return attack == b.attack && decay == b.decay && sustain == b.sustain &&
           release == b.release && type == b.type;
  }

  constexpr bool operator!=(ADSR b) const {
    return attack != b.attack || decay != b.decay || sustain != b.sustain ||
           release != b.release || type != b.type;
  }

  inline operator std::string() const {
    std::string stream;
    switch (type) {
    case Lin:
      stream = "lin";
      break;
    case Exp:
      stream = "exp";
      break;
    case Const:
      stream = "const";
      break;
    }

    stream += " A: " + std::string(attack) + " D: " + std::string(decay) +
              " S: " + std::string(sustain) + " R: " + std::string(release);

    return stream;
  }
};

union CurveState {
  float tau;   // Exp
  float slope; // Lin
};

class Curve {
  EnvelopeLevel _target;
  CurveType _type;
  Duration _total;

  Duration _elapsed;
  EnvelopeLevel _current;
  CurveState _state;
  bool _target_reached = false;

public:
  Curve(EnvelopeLevel start, EnvelopeLevel target, Duration total,
        CurveType type);
  Curve(EnvelopeLevel constant);
  EnvelopeLevel update(Duration delta);
  bool is_target_reached() const { return _target_reached; }
  std::optional<Duration> will_reach_target(const Duration &dt) const;
};

class Envelope {
  ADSR _configs;
  Curve _current;

  Duration progress(Duration delta, bool on);

public:
  enum Stage { Attack, Decay, Sustain, Release, Off };

  Envelope(ADSR configs);
  Envelope(EnvelopeLevel level);
  EnvelopeLevel update(Duration delta, bool on);
  Stage stage() const { return _stage; }
  bool is_off() const { return _stage == Off; }

private:
  Stage _stage;
};

} // namespace teslasynth::synth
