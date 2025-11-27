#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdint.h>
#include <string>

namespace teslasynth::core {

template <typename T = uint64_t> class SimpleDuration {
  T _value;

  static constexpr uint32_t _coef_milli = 1000;
  static constexpr uint32_t _coef_sec = 1000 * _coef_milli;

  explicit constexpr SimpleDuration(T v) : _value(v) {}

public:
  constexpr SimpleDuration() : _value(0) {}

  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  constexpr SimpleDuration(const SimpleDuration<U> &other)
      : _value(static_cast<T>(other.micros())) {}

  static constexpr SimpleDuration micros(T v) { return SimpleDuration(v); }
  static constexpr SimpleDuration millis(T v) {
    return SimpleDuration(v * _coef_milli);
  }
  static constexpr SimpleDuration seconds(T v) {
    return SimpleDuration(v * _coef_sec);
  }

  constexpr T micros() const { return _value; }
  constexpr T millis() const { return _value / _coef_milli; }
  constexpr T seconds() const { return _value / _coef_sec; }

  inline static constexpr SimpleDuration zero() {
    return SimpleDuration(static_cast<T>(0));
  }
  inline static constexpr SimpleDuration max() {
    return SimpleDuration(std::numeric_limits<T>::max());
  }

  constexpr SimpleDuration operator+(const SimpleDuration &b) const {
    return SimpleDuration(_value + b._value);
  }
  constexpr std::optional<SimpleDuration>
  operator-(const SimpleDuration &b) const {
    if (_value >= b._value)
      return SimpleDuration(_value - b._value);
    else
      return {};
  }
  constexpr SimpleDuration operator*(const int b) const {
    return SimpleDuration(_value * b);
  }
  constexpr SimpleDuration operator*(const float b) const {
    return SimpleDuration(static_cast<T>(_value * b));
  }

  SimpleDuration &operator+=(const SimpleDuration &b) {
    _value += b._value;
    return *this;
  }
  SimpleDuration &operator*=(const SimpleDuration &b) {
    _value *= b._value;
    return *this;
  }

  constexpr bool operator<(const SimpleDuration &b) const {
    return _value < b._value;
  }
  constexpr bool operator>(const SimpleDuration &b) const {
    return _value > b._value;
  }
  constexpr bool operator==(const SimpleDuration &b) const {
    return _value == b._value;
  }
  constexpr bool operator!=(const SimpleDuration &b) const {
    return _value != b._value;
  }
  constexpr bool operator<=(const SimpleDuration &b) const {
    return _value <= b._value;
  }
  constexpr bool operator>=(const SimpleDuration &b) const {
    return _value >= b._value;
  }
  constexpr bool is_zero() const { return _value == 0; }

  inline operator std::string() const {
    if (_value > _coef_sec) {
      return std::to_string(_value / static_cast<float>(_coef_sec)) + "S";
    } else if (_value > _coef_milli) {
      return std::to_string(_value / static_cast<float>(_coef_milli)) + "ms";
    } else {
      return std::to_string(_value) + "us";
    }
  }
};

typedef SimpleDuration<uint64_t> Duration;
typedef SimpleDuration<uint32_t> Duration32;

inline constexpr Duration operator""_us(unsigned long long l) {
  return Duration::micros(static_cast<uint64_t>(l));
}
inline constexpr Duration operator""_ms(unsigned long long l) {
  return Duration::millis(static_cast<uint64_t>(l));
}
inline constexpr Duration operator""_s(unsigned long long l) {
  return Duration::seconds(static_cast<uint64_t>(l));
}

class Hertz {
  float _value;

  static constexpr uint32_t _coef_kilo = 1000;
  static constexpr uint32_t _coef_mega = 1000 * _coef_kilo;

public:
  explicit constexpr Hertz(float v) : _value(v) {}
  static constexpr Hertz kilohertz(uint32_t v) { return Hertz(v * 1000); }
  static constexpr Hertz megahertz(uint32_t v) { return Hertz(v * 1'000'000); }

  constexpr Hertz operator+(const Hertz b) const {
    return Hertz(_value + b._value);
  }
  constexpr Hertz operator-(const Hertz b) const {
    return Hertz(_value - b._value);
  }
  constexpr Hertz operator-() const { return Hertz(-_value); }
  constexpr Hertz operator*(const int b) const { return Hertz(_value * b); }
  constexpr Hertz operator*(const float b) const { return Hertz(_value * b); }
  constexpr operator float() const { return _value; }

  constexpr bool operator<(const Hertz &b) const { return _value < b._value; }
  constexpr bool operator>(const Hertz &b) const { return _value > b._value; }
  constexpr bool operator==(const Hertz &b) const {
    return fabsf(_value - b._value) < 0.001;
  }
  constexpr bool operator!=(const Hertz &b) const {
    return fabsf(_value - b._value) > 0.001;
  }
  constexpr bool operator<=(const Hertz &b) const { return _value <= b._value; }
  constexpr bool operator>=(const Hertz &b) const { return _value >= b._value; }
  constexpr bool is_zero() const { return _value == 0; }
  constexpr Duration32 period() const {
    return Duration32::micros(1e6 / _value);
  }

  inline operator std::string() const {
    if (_value > _coef_mega) {
      return std::to_string(_value / static_cast<float>(_coef_mega)) + "MHz";
    } else if (_value > _coef_kilo) {
      return std::to_string(_value / static_cast<float>(_coef_kilo)) + "KHz";
    } else {
      return std::to_string(_value) + "Hz";
    }
  }
};

inline constexpr Hertz operator""_hz(unsigned long long n) {
  return Hertz(static_cast<float>(n));
}
inline constexpr Hertz operator""_khz(unsigned long long n) {
  return Hertz::kilohertz(static_cast<float>(n));
}
inline constexpr Hertz operator""_mhz(unsigned long long n) {
  return Hertz::megahertz(static_cast<float>(n));
}

inline constexpr Hertz operator""_hz(long double n) {
  return Hertz(static_cast<float>(n));
}
inline constexpr Hertz operator""_khz(long double n) {
  return Hertz::kilohertz(static_cast<float>(n));
}
inline constexpr Hertz operator""_mhz(long double n) {
  return Hertz::megahertz(static_cast<float>(n));
}

} // namespace teslasynth::core
