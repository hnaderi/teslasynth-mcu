#include "envelope.hpp"
#include "core.hpp"
#include <cmath>
#include <optional>

using namespace teslasynth::core;

namespace teslasynth::synth {

// -log_e(0.001)
constexpr float logfactor = 6.907755278982137;

Curve::Curve(EnvelopeLevel start, EnvelopeLevel target, Duration32 total,
             CurveType type)
    : _target(target), _type(type), _total(total), _current(start) {
  const auto t = total.micros();
  if (t <= 0) {
    _target_reached = true;
    _current = target;
  } else
    switch (type) {
    case Exp:
      _state.tau = (float)t / logfactor;
      break;
    case Lin:
      _state.slope = (target - start) / t;
      break;
    case Const:
      _target_reached = true;
      _current = target;
      break;
    }
}
Curve::Curve(EnvelopeLevel constant)
    : _target(constant), _type(Const), _current(constant),
      _target_reached(true) {}

std::optional<Duration32> Curve::will_reach_target(const Duration32 &dt) const {
  if (_type != Const)
    return (dt + _elapsed) - _total;
  else
    return dt;
}

EnvelopeLevel Curve::update(Duration32 delta) {
  if (_target_reached || _type == Const) {
    // noop
  } else if (_elapsed + delta >= _total) {
    _target_reached = true;
    _elapsed = _total;
    _current = _target;
  } else {
    const auto dt = delta.micros();
    if (_type == Exp)
      _current += (_target - _current) * (1 - expf(-(float)dt / _state.tau));
    else if (_type == Lin)
      _current += _state.slope * dt;

    _elapsed += delta;
    _target_reached = _current == _target || _elapsed >= _total;
  }
  return _current;
}

Envelope::Envelope(ADSR configs)
    : _configs(configs),
      _current(configs.type == Const ? Curve(configs.sustain)
                                     : Curve(EnvelopeLevel(0), EnvelopeLevel(1),
                                             configs.attack, configs.type)),
      _stage(Attack) {}

Envelope::Envelope(EnvelopeLevel level) : Envelope(ADSR::constant(level)) {}

Duration32 Envelope::progress(Duration32 delta, bool on) {
  Duration32 remained = delta;
  auto dt = _current.will_reach_target(remained);
  while (dt && (!remained.is_zero() || !on)) {
    switch (_stage) {
    case Attack:
      _current = Curve(EnvelopeLevel(1), _configs.sustain, _configs.decay,
                       _configs.type);
      _stage = Decay;
      break;
    case Decay:
      _current = Curve(_configs.sustain);
      _stage = Sustain;
      break;
    case Sustain:
      if (!on) {
        _current = Curve(_configs.sustain, EnvelopeLevel(0), _configs.release,
                         _configs.type);
        _stage = Release;
      } else {
        dt = 0_us;
      }
      break;
    case Release:
      _current = Curve(EnvelopeLevel(0));
      _stage = Off;
      break;
    case Off:
      return 0_us;
    }

    remained = *dt;
    dt = _current.will_reach_target(remained);
  }
  return remained;
}

EnvelopeLevel Envelope::update(Duration32 delta, bool on) {
  if (is_off())
    return EnvelopeLevel(0);
  auto remained = progress(delta, on);
  return _current.update(remained);
}

}; // namespace teslasynth::synth
