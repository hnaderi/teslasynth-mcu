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
      : _value(static_cast<T>(other.micros())) {
    static_assert(sizeof(U) <= sizeof(T),
                  "Cannot convert SimpleDuration<U> to smaller "
                  "SimpleDuration<T>: potential overflow");
  }

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

  template <typename U>
  constexpr auto operator+(const SimpleDuration<U> &b) const {
    using R = std::common_type_t<T, U>;
    return SimpleDuration<R>::micros(_value + b.micros());
  }

  template <typename U>
  constexpr std::optional<SimpleDuration>
  operator-(const SimpleDuration<U> &b) const {
    if (_value >= b.micros())
      return SimpleDuration<T>(_value - b.micros());
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

  template <typename U>
  constexpr bool operator<(const SimpleDuration<U> &b) const {
    return _value < b.micros();
  }

  template <typename U>
  constexpr bool operator>(const SimpleDuration<U> &b) const {
    return _value > b.micros();
  }

  template <typename U>
  constexpr bool operator==(const SimpleDuration<U> &b) const {
    return _value == b.micros();
  }
  template <typename U>
  constexpr bool operator!=(const SimpleDuration<U> &b) const {
    return _value != b.micros();
  }

  template <typename U>
  constexpr bool operator<=(const SimpleDuration<U> &b) const {
    return _value <= b.micros();
  }
  template <typename U>
  constexpr bool operator>=(const SimpleDuration<U> &b) const {
    return _value >= b.micros();
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
typedef SimpleDuration<uint64_t> Duration64;
typedef SimpleDuration<uint32_t> Duration32;
typedef SimpleDuration<uint16_t> Duration16;

template <unsigned long long V> struct smallest_uint {
  using type = std::conditional_t<
      (V <= UINT16_MAX), uint16_t,
      std::conditional_t<(V <= UINT32_MAX), uint32_t, uint64_t>>;
};

template <unsigned long long V>
constexpr SimpleDuration<typename smallest_uint<V>::type> make_duration() {
  using T = typename smallest_uint<V>::type;
  return SimpleDuration<T>::micros(V);
}

constexpr unsigned long long pow10(unsigned n) {
  unsigned long long r = 1;
  for (unsigned i = 0; i < n; ++i)
    r *= 10;
  return r;
}

// Convert char digits to integer at compile time
template <char... Cs> struct digits_to_value;

template <> struct digits_to_value<> {
  static constexpr unsigned long long value = 0;
};

template <char C, char... Cs> struct digits_to_value<C, Cs...> {
  static_assert(C >= '0' && C <= '9', "Invalid digit");
  static constexpr unsigned long long value =
      (C - '0') * pow10(sizeof...(Cs)) + digits_to_value<Cs...>::value;
};

template <char... Cs> constexpr auto operator""_us() {
  constexpr unsigned long long v = digits_to_value<Cs...>::value;
  using T = typename smallest_uint<v>::type;
  return SimpleDuration<T>::micros(v);
}
template <char... Cs> constexpr auto operator""_ms() {
  constexpr unsigned long long v = digits_to_value<Cs...>::value * 1000;
  using T = typename smallest_uint<v>::type;
  return SimpleDuration<T>::micros(v);
}
template <char... Cs> constexpr auto operator""_s() {
  constexpr unsigned long long v = digits_to_value<Cs...>::value * 1000'000;
  using T = typename smallest_uint<v>::type;
  return SimpleDuration<T>::micros(v);
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
