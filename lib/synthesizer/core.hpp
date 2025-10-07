#pragma once

#include <cmath>
#include <cstdint>
#include <optional>
#include <stdint.h>
#include <string>

class Duration {
  uint32_t _value;

  static constexpr uint32_t _coef_micro = 10;
  static constexpr uint32_t _coef_milli = 1000 * _coef_micro;
  static constexpr uint32_t _coef_sec = 1000 * _coef_milli;

  explicit constexpr Duration(uint32_t v) : _value(v) {}
  explicit Duration(int v) = delete;

public:
  constexpr Duration() : _value(0) {}

  static constexpr Duration nanos(uint32_t v) { return Duration(v / 100); }
  static constexpr Duration micros(uint32_t v) {
    return Duration(v * _coef_micro);
  }
  static constexpr Duration millis(uint32_t v) {
    return Duration(v * _coef_milli);
  }
  static constexpr Duration seconds(uint32_t v) {
    return Duration(v * _coef_sec);
  }
  constexpr uint32_t value() const { return _value; }

  template <typename T> constexpr T micros() const {
    return static_cast<T>(_value) / _coef_micro;
  }
  template <typename T> constexpr T millis() const {
    return static_cast<T>(_value) / _coef_milli;
  }
  template <typename T> constexpr T seconds() const {
    return static_cast<T>(_value) / _coef_sec;
  }

  inline static constexpr Duration zero() {
    return Duration(static_cast<uint32_t>(0));
  }
  inline static constexpr Duration max() { return Duration(UINT32_MAX); }

  constexpr Duration operator+(const Duration &b) const {
    return Duration(_value + b._value);
  }
  constexpr std::optional<Duration> operator-(const Duration &b) const {
    if (_value >= b._value)
      return Duration(_value - b._value);
    else
      return {};
  }
  constexpr Duration operator*(const int b) const {
    return Duration(_value * b);
  }
  constexpr Duration operator*(const float b) const {
    return Duration(static_cast<uint32_t>(_value * b));
  }

  Duration &operator+=(const Duration &b) {
    _value += b._value;
    return *this;
  }
  Duration &operator*=(const Duration &b) {
    _value *= b._value;
    return *this;
  }

  constexpr bool operator<(const Duration &b) const {
    return _value < b._value;
  }
  constexpr bool operator>(const Duration &b) const {
    return _value > b._value;
  }
  constexpr bool operator==(const Duration &b) const {
    return _value == b._value;
  }
  constexpr bool operator!=(const Duration &b) const {
    return _value != b._value;
  }
  constexpr bool operator<=(const Duration &b) const {
    return _value <= b._value;
  }
  constexpr bool operator>=(const Duration &b) const {
    return _value >= b._value;
  }
  constexpr bool is_zero() const { return _value == 0; }

  inline operator std::string() const {
    if (_value > _coef_sec) {
      return std::to_string(_value / static_cast<float>(_coef_sec)) + "S";
    } else if (_value > _coef_milli) {
      return std::to_string(_value / static_cast<float>(_coef_milli)) + "ms";
    } else if (_value > _coef_micro) {
      return std::to_string(_value / static_cast<float>(_coef_micro)) + "us";
    } else {
      return std::to_string(_value) + "00ns";
    }
  }
};

inline constexpr Duration operator""_ns(unsigned long long l) {
  return Duration::nanos(static_cast<uint32_t>(l));
}
inline constexpr Duration operator""_us(unsigned long long l) {
  return Duration::micros(static_cast<uint32_t>(l));
}
inline constexpr Duration operator""_ms(unsigned long long l) {
  return Duration::millis(static_cast<uint32_t>(l));
}
inline constexpr Duration operator""_s(unsigned long long l) {
  return Duration::seconds(static_cast<uint32_t>(l));
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
    return std::fabsf(_value - b._value) < 0.001;
  }
  constexpr bool operator!=(const Hertz &b) const {
    return std::fabsf(_value - b._value) > 0.001;
  }
  constexpr bool operator<=(const Hertz &b) const { return _value <= b._value; }
  constexpr bool operator>=(const Hertz &b) const { return _value >= b._value; }
  constexpr bool is_zero() const { return _value == 0; }
  constexpr Duration period() const { return Duration::nanos(1e9 / _value); }

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
  return Hertz(static_cast<uint32_t>(n));
}
inline constexpr Hertz operator""_khz(unsigned long long n) {
  return Hertz::kilohertz(static_cast<uint32_t>(n));
}
inline constexpr Hertz operator""_mhz(unsigned long long n) {
  return Hertz::megahertz(static_cast<uint32_t>(n));
}
