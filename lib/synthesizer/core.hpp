#pragma once

#include <algorithm>
#include <cstdint>
#include <stdint.h>
#include <string>

struct Pulse {
  uint16_t on, off;
};

class Duration {
  uint32_t value;

  explicit Duration(uint32_t v) : value(v) {}
  explicit Duration(int v) = delete;

public:
  Duration() : value(0) {}
  static Duration nanos(uint32_t v) { return Duration(v); }
  static Duration micros(uint32_t v) { return Duration(v * 1000); }
  static Duration millis(uint32_t v) { return Duration(v * 1'000'000); }

  Duration operator+(const Duration &b) const {
    return Duration(value + b.value);
  }
  Duration operator*(const int b) const { return Duration(value * b); }
  Duration operator*(const float b) const {
    return Duration(static_cast<uint32_t>(value * b));
  }
  Duration &operator+=(const Duration &b) {
    value += b.value;
    return *this;
  }
  Duration &operator*=(const Duration &b) {
    value *= b.value;
    return *this;
  }

  constexpr bool operator<(const Duration &b) const { return value < b.value; }
  constexpr bool operator>(const Duration &b) const { return value > b.value; }
  constexpr bool operator==(const Duration &b) const {
    return value == b.value;
  }
  constexpr bool operator!=(const Duration &b) const {
    return value != b.value;
  }
  constexpr bool operator<=(const Duration &b) const {
    return value <= b.value;
  }
  constexpr bool operator>=(const Duration &b) const {
    return value >= b.value;
  }
  inline std::string print() const {
    if (value > 1'000'000'000) {
      return std::to_string(value / 1e9) + "S";
    } else if (value > 1'000'000) {
      return std::to_string(value / 1e6) + "ms";
    } else if (value > 1'000) {
      return std::to_string(value / 1e3) + "us";
    } else {
      return std::to_string(value) + "ns";
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
