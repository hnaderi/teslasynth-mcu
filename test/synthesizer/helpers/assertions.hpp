#pragma once

#include "core.hpp"
#include "envelope.hpp"
#include <optional>
#include <string>
#include <unity.h>

namespace synth {
namespace assertions {
template <typename T> inline std::string __msg_for(T a, T b) {
  return std::string("Obtained: " + std::string(a) +
                     " Expected: " + std::string(b));
}

inline void assert_level_equal(EnvelopeLevel a, EnvelopeLevel b, int line) {
  UNITY_TEST_ASSERT(a == b, line, __msg_for(a, b).c_str());
}
inline void assert_level_not_equal(EnvelopeLevel a, EnvelopeLevel b, int line) {
  UNITY_TEST_ASSERT(a != b, line, __msg_for(a, b).c_str());
}

inline void assert_duration_equal(Duration a, Duration b, int line) {
  UNITY_TEST_ASSERT(a == b, line, __msg_for(a, b).c_str());
}
inline void assert_duration_equal(std::optional<Duration> a, Duration b,
                                  int line) {
  UNITY_TEST_ASSERT(a, line, "No duration!");
  UNITY_TEST_ASSERT(a == b, line, __msg_for(*a, b).c_str());
}
inline void assert_duration_not_equal(Duration a, Duration b, int line) {
  UNITY_TEST_ASSERT(a != b, line, __msg_for(a, b).c_str());
}
inline void assert_hertz_equal(Hertz a, Hertz b, int line) {
  UNITY_TEST_ASSERT(a == b, line, __msg_for(a, b).c_str());
}
inline void assert_hertz_not_equal(Hertz a, Hertz b, int line) {
  UNITY_TEST_ASSERT(a != b, line, __msg_for(a, b).c_str());
}
}; // namespace assertions
}; // namespace synth

#define assert_level_equal(a, b)                                               \
  synth::assertions::assert_level_equal(a, b, __LINE__);

#define assert_level_not_equal(a, b)                                           \
  synth::assertions::assert_level_not_equal(a, b, __LINE__);

#define assert_duration_equal(a, b)                                            \
  synth::assertions::assert_duration_equal(a, b, __LINE__);

#define assert_duration_not_equal(a, b)                                        \
  synth::assertions::assert_duration_not_equal(a, b, __LINE__);

#define assert_hertz_equal(a, b)                                               \
  synth::assertions::assert_hertz_equal(a, b, __LINE__);

#define assert_hertz_not_equal(a, b)                                           \
  synth::assertions::assert_hertz_not_equal(a, b, __LINE__);
