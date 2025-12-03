#include "synthesizer/helpers/assertions.hpp"
#include <cmath>
#include <core.hpp>
#include <cstdint>
#include <unity.h>

using namespace teslasynth::core;

void test_microseconds(void) {
  TEST_ASSERT_FALSE(0_us > 1_us);
  TEST_ASSERT_TRUE(0_us < 1_us);
  TEST_ASSERT_FALSE(10_us < 10_us);
  TEST_ASSERT_FALSE(10_us > 10_us);

  assert_duration_equal(10_us, 10_us);
  assert_duration_not_equal(10_us, 9_us);

  TEST_ASSERT_TRUE(10_us >= 10_us);
  TEST_ASSERT_TRUE(11_us >= 10_us);
  TEST_ASSERT_TRUE(10_us <= 11_us);

  assert_duration_equal(10_us + 11_us, 21_us);
  assert_duration_equal(10_us * 100, 1_ms);
}

void test_seconds(void) {
  assert_duration_equal(1_s, 1000_ms);
  assert_duration_equal(1_s, 1000000_us);
  assert_duration_equal(1_s, Duration::seconds(1));
  assert_duration_equal(429_s, 429000000_us);
}

void test_duration_literals() {
  assert_duration_equal(1_s, Duration::seconds(1));
  assert_duration_equal(100_s, Duration::seconds(100));
  assert_duration_equal(1000_s, Duration::seconds(1000));
  assert_duration_equal(10000_us, Duration::micros(10000));
  assert_duration_equal(100000_ms, Duration::millis(100000));
  assert_duration_equal(1000000000_s, Duration::seconds(1000000000));
}

void test_duration_constants() {
  constexpr Duration zero = Duration::zero();
  constexpr Duration max = Duration::max();

  assert_duration_equal(zero, Duration());
  assert_duration_equal(zero, 0_us);

  // Max time is more than 50 years...
  TEST_ASSERT_TRUE(max > 3600_s * 24 * 365 * 50);
  TEST_ASSERT_EQUAL(sizeof(uint64_t), sizeof(Duration));
}

void test_duration32_constants() {
  constexpr Duration32 zero = Duration32::zero();
  constexpr Duration32 max = Duration32::max();
  Duration max64 = max;

  assert_duration_equal(zero, Duration32());
  assert_duration_equal(zero, Duration32::zero());

  assert_duration_equal(max, max64);

  TEST_ASSERT_TRUE_MESSAGE(max > Duration32::seconds(3600),
                           "Must be more than one hour");
  TEST_ASSERT_TRUE_MESSAGE(max > Duration32::seconds(71 * 60) &&
                               Duration32::seconds(71 * 60) >
                                   Duration32::seconds(72 * 60),
                           "Must be more than 71 minutes");
  TEST_ASSERT_EQUAL(sizeof(uint32_t), sizeof(Duration32));

  TEST_ASSERT_TRUE_MESSAGE(Duration::max() > Duration32::max(),
                           "Must be less than 64 bit duration max");
}

void test_duration16_constants() {
  constexpr Duration16 zero = Duration16::zero();
  constexpr Duration16 max = Duration16::max();

  assert_duration_equal(zero, Duration16());
  assert_duration_equal(zero, Duration16::zero());

  TEST_ASSERT_TRUE_MESSAGE(66_ms > max, "Must be less than 66ms");
  TEST_ASSERT_TRUE_MESSAGE(max > 65_ms, "Must be more than 65ms");
  TEST_ASSERT_EQUAL(sizeof(uint16_t), sizeof(Duration16));
}

void test_duration_arithmetics() {
  constexpr Duration zero = Duration::zero();
  constexpr Duration max = Duration::max();

  assert_duration_equal(zero * 1, zero);
  assert_duration_equal(zero + zero, zero);
  assert_duration_equal(max * 0, zero);

  assert_duration_equal(1_us + 100_s, 100_s + 1_us);
}

void test_duration_minus(void) {
  auto a = 1_s - 900_ms;
  TEST_ASSERT_TRUE(a);
  TEST_ASSERT_TRUE(*a == 100_ms);

  a = 1_s - 2_s;
  TEST_ASSERT_FALSE(a);
  TEST_ASSERT_TRUE(100_s - 1_us);
}

void test_hertz(void) {
  TEST_ASSERT_TRUE(0.5_hz < 1_hz);
  TEST_ASSERT_TRUE(2_mhz > 100_khz);
  TEST_ASSERT_TRUE(20_khz < 100_khz);
  assert_duration_equal((1_mhz).period(), 1_us);
  assert_duration_equal((100_khz).period(), 10_us);
  assert_duration_equal((100_hz).period(), 10_ms);
  TEST_ASSERT_TRUE(Hertz::megahertz(2) == Hertz::kilohertz(2000));
  TEST_ASSERT_TRUE(1.5_hz * 2 == 3_hz);

  assert_hertz_not_equal(2_hz, -2_hz);
  assert_hertz_not_equal(-1_hz, -2_hz);
  assert_hertz_equal(-1_hz, -1_hz);
  assert_hertz_not_equal(1_hz, 2_hz);
}

void test_hertz_arithmetic(void) {
  assert_hertz_equal(100_hz + 0_hz, 100_hz);
  assert_hertz_equal(100_hz - 0_hz, 100_hz);
  assert_hertz_equal(100_hz - 2_hz, 98_hz);

  assert_hertz_equal(100_hz + 900_hz, 1_khz);
  assert_hertz_equal(100_mhz + 900_mhz, 1000_mhz);

  assert_hertz_equal(900_mhz - 100_mhz, 800_mhz);
  assert_hertz_equal(2_khz * 4, 8_khz);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_seconds);
  RUN_TEST(test_microseconds);
  RUN_TEST(test_duration_literals);
  RUN_TEST(test_duration_constants);
  RUN_TEST(test_duration16_constants);
  RUN_TEST(test_duration32_constants);
  RUN_TEST(test_duration_arithmetics);
  RUN_TEST(test_duration_minus);
  RUN_TEST(test_hertz);
  RUN_TEST(test_hertz_arithmetic);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
