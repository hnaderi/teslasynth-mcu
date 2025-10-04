#include <core.hpp>
#include <unity.h>

void test_microseconds(void) {
  TEST_ASSERT_FALSE(0_us > 1_us);
  TEST_ASSERT_TRUE(0_us < 1_us);
  TEST_ASSERT_FALSE(10_us < 10_us);
  TEST_ASSERT_FALSE(10_us > 10_us);

  TEST_ASSERT_TRUE(10_us == 10_us);
  TEST_ASSERT_TRUE(10_us != 9_us);

  TEST_ASSERT_TRUE(10_us >= 10_us);
  TEST_ASSERT_TRUE(11_us >= 10_us);
  TEST_ASSERT_TRUE(10_us <= 11_us);

  TEST_ASSERT_TRUE(10_us + 11_us == 21_us);
  TEST_ASSERT_TRUE(10_us * 100 == 1_ms);
}

void test_nanoseconds(void) {
  // Resolution is 100ns
  TEST_ASSERT_TRUE(1_ns == 0_ns);
  TEST_ASSERT_TRUE(101_ns == 100_ns);
  TEST_ASSERT_TRUE(1001_ns == 1_us);
  TEST_ASSERT_TRUE(1099_ns == 1_us);
}

void test_seconds(void) {
  TEST_ASSERT_TRUE(1_s == 1000_ms);
  TEST_ASSERT_TRUE(1_s == 1000000_us);
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
  TEST_ASSERT_TRUE((2_mhz).period() == 500_ns);
  TEST_ASSERT_TRUE((100_khz).period() == 10_us);
  TEST_ASSERT_TRUE((100_hz).period() == 10_ms);

  TEST_ASSERT_TRUE(Hertz::megahertz(2) == Hertz::kilohertz(2000));
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_seconds);
  RUN_TEST(test_microseconds);
  RUN_TEST(test_nanoseconds);
  RUN_TEST(test_duration_minus);
  RUN_TEST(test_hertz);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
