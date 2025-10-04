#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdint.h>
#include <string>

struct Pulse {
  uint16_t on, off;
};

class Duration {
  uint32_t _value;

  explicit Duration(uint32_t v) : _value(v) {}
  explicit Duration(int v) = delete;
  static constexpr uint32_t _coef_micro = 10;
  static constexpr uint32_t _coef_milli = 1000 * _coef_micro;
  static constexpr uint32_t _coef_sec = 1000 * _coef_milli;

public:
  Duration() : _value(0) {}
  static Duration nanos(uint32_t v) { return Duration(v / 100); }
  static Duration micros(uint32_t v) { return Duration(v * _coef_micro); }
  static Duration millis(uint32_t v) { return Duration(v * _coef_milli); }
  static Duration seconds(uint32_t v) { return Duration(v * _coef_sec); }
  constexpr uint32_t value() const { return _value; }

  Duration operator+(const Duration &b) const {
    return Duration(_value + b._value);
  }
  std::optional<Duration> operator-(const Duration &b) const {
    if (_value >= b._value)
      return Duration(_value - b._value);
    else
      return {};
  }
  Duration operator*(const int b) const { return Duration(_value * b); }
  Duration operator*(const float b) const {
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

  inline std::string print() const {
    if (_value > _coef_sec) {
      return std::to_string(_value / _coef_sec) + "S";
    } else if (_value > _coef_milli) {
      return std::to_string(_value / _coef_milli) + "ms";
    } else if (_value > _coef_micro) {
      return std::to_string(_value / _coef_micro) + "us";
    } else {
      return std::to_string(_value) + "00ns";
    }
  }
};

inline Duration operator""_ns(unsigned long long l) {
  return Duration::nanos(static_cast<uint32_t>(l));
}
inline Duration operator""_us(unsigned long long l) {
  return Duration::micros(static_cast<uint32_t>(l));
}
inline Duration operator""_ms(unsigned long long l) {
  return Duration::millis(static_cast<uint32_t>(l));
}
inline Duration operator""_s(unsigned long long l) {
  return Duration::seconds(static_cast<uint32_t>(l));
}

class Hertz {
  uint32_t value;

public:
  explicit Hertz(int v) = delete;
  explicit Hertz(uint32_t v) : value(std::max<uint32_t>(1, v)) {}
  static Hertz kilohertz(uint32_t v) { return Hertz(v * 1000); }
  static Hertz megahertz(uint32_t v) { return Hertz(v * 1'000'000); }

  Hertz operator*(const int b) const { return Hertz(value * b); }
  Hertz operator*(const float b) const {
    return Hertz(static_cast<uint32_t>(value * b));
  }

  constexpr bool operator<(const Hertz &b) const { return value < b.value; }
  constexpr bool operator>(const Hertz &b) const { return value > b.value; }
  constexpr bool operator==(const Hertz &b) const { return value == b.value; }
  constexpr bool operator!=(const Hertz &b) const { return value != b.value; }
  constexpr bool operator<=(const Hertz &b) const { return value <= b.value; }
  constexpr bool operator>=(const Hertz &b) const { return value >= b.value; }
  Duration period() const {
    return Duration::nanos(1'000'000 / (value / 1000.f));
  }
};

inline Hertz operator""_hz(unsigned long long n) {
  return Hertz(static_cast<uint32_t>(n));
}
inline Hertz operator""_khz(unsigned long long n) {
  return Hertz::kilohertz(static_cast<uint32_t>(n));
}
inline Hertz operator""_mhz(unsigned long long n) {
  return Hertz::megahertz(static_cast<uint32_t>(n));
}
