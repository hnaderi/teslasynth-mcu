#pragma once

#include "core.hpp"
#include <cmath>
#include <optional>
#include <string>

constexpr float epsilon = 0.001;
enum CurveType { Lin, Exp, Const };

class EnvelopeLevel {
  float _value;

public:
  explicit EnvelopeLevel(float level) {
    if (level > 1)
      _value = 1.f;
    else if (level < 0)
      _value = 0.f;
    else
      _value = level;
  }

  EnvelopeLevel operator+(const EnvelopeLevel &b) const {
    return EnvelopeLevel(_value + b._value);
  }
  EnvelopeLevel &operator+=(const EnvelopeLevel &b) {
    if (1.f - _value < b._value)
      _value = 1.f;
    else
      _value += b._value;
    return *this;
  }
  float operator-(const EnvelopeLevel &b) const { return _value - b._value; }
  EnvelopeLevel operator+(float b) const { return EnvelopeLevel(_value + b); }
  EnvelopeLevel &operator+=(float b) {
    _value += b;
    if (_value > 1.f)
      _value = 1.f;
    else if (_value < 0)
      _value = 0.f;
    return *this;
  }

  Duration operator*(const Duration &b) const { return b * _value; }
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
  Duration attack;
  Duration decay;
  EnvelopeLevel sustain;
  Duration release;
  enum CurveType type;

  static ADSR constant(EnvelopeLevel level) {
    return {0_ns, 0_ns, level, 0_ns, Const};
  }
  static ADSR linear(Duration attack, Duration decay, EnvelopeLevel sustain,
                     Duration release) {
    return {attack, decay, sustain, release, Lin};
  }
  static ADSR exponential(Duration attack, Duration decay,
                          EnvelopeLevel sustain, Duration release) {
    return {attack, decay, sustain, release, Exp};
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
