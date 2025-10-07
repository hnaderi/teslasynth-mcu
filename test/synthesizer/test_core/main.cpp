#include "synthesizer/helpers/assertions.hpp"
#include <cmath>
#include <core.hpp>
#include <unity.h>

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

void test_nanoseconds(void) {
  // Resolution is 100ns
  assert_duration_equal(1_ns, 0_ns);
  assert_duration_equal(101_ns, 100_ns);
  assert_duration_equal(1001_ns, 1_us);
  assert_duration_equal(1099_ns, 1_us);
}

void test_seconds(void) {
  assert_duration_equal(1_s, 1000_ms);
  assert_duration_equal(1_s, 1000000_us);
  assert_duration_equal(429_s, 429'000'000_us);
}

void test_duration_constants() {
  constexpr Duration zero = Duration::zero();
  constexpr Duration max = Duration::max();

  assert_duration_equal(zero, Duration());
  assert_duration_equal(zero, 0_ns);
  TEST_ASSERT_TRUE(max - 429_s < 1_s);
}

void test_duration_minus(void) {
  auto a = 1_s - 900_ms;
  TEST_ASSERT_TRUE(a);
  TEST_ASSERT_TRUE(*a == 100_ms);

  a = 1_s - 2_s;
  TEST_ASSERT_FALSE(a);
}

void test_hertz(void) {
  TEST_ASSERT_TRUE(2_mhz > 100_khz);
  TEST_ASSERT_TRUE(20_khz < 100_khz);
  assert_duration_equal((2_mhz).period(), 500_ns);
  assert_duration_equal((100_khz).period(), 10_us);
  assert_duration_equal((100_hz).period(), 10_ms);
  TEST_ASSERT_TRUE(Hertz::megahertz(2) == Hertz::kilohertz(2000));

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
  RUN_TEST(test_nanoseconds);
  RUN_TEST(test_duration_constants);
  RUN_TEST(test_duration_minus);
  RUN_TEST(test_hertz);
  RUN_TEST(test_hertz_arithmetic);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
