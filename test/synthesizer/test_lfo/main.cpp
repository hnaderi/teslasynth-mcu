#include "core.hpp"
#include "lfo.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <synth.hpp>
#include <unity.h>

void test_flat(void) {
  Vibrato lfo;

  assert_hertz_equal(lfo.offset(0_us), 0_hz);
  assert_hertz_equal(lfo.offset(1_ms), 0_hz);
  assert_hertz_equal(lfo.offset(3_ms), 0_hz);
  assert_hertz_equal(lfo.offset(1_s), 0_hz);
}

void test_oscillation1(void) {
  Vibrato lfo{1_hz, 2_hz};

  for (int i = 0; i < 100; i++) {
    auto start = Duration::seconds(i);
    assert_hertz_equal(lfo.offset(start + 0_us), 0_hz);
    assert_hertz_equal(lfo.offset(start + 250_ms), 2_hz);
    assert_hertz_equal(lfo.offset(start + 500_ms), 0_hz);
    assert_hertz_equal(lfo.offset(start + 750_ms), -2_hz);
    assert_hertz_equal(lfo.offset(start + 1_s), 0_hz);
  }
}

void test_oscillation2(void) {
  Vibrato lfo{2_hz, 10_hz};

  assert_hertz_equal(lfo.offset(0_us), 0_hz);
  assert_hertz_equal(lfo.offset(125_ms), 10_hz);
  assert_hertz_equal(lfo.offset(250_ms), 0_hz);
  assert_hertz_equal(lfo.offset(375_ms), -10_hz);
  assert_hertz_equal(lfo.offset(500_ms), 0_hz);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_flat);
  RUN_TEST(test_oscillation1);
  RUN_TEST(test_oscillation2);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
