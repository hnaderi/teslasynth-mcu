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
  RUN_TEST(test_microseconds);
  RUN_TEST(test_hertz);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
