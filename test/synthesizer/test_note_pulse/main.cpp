#include "core.hpp"
#include "envelope.hpp"
#include "notes.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <unity.h>

void test_empty(void) {
  NotePulse pulse;
  TEST_ASSERT_TRUE(pulse.is_zero());
  assert_duration_equal(pulse.start, Duration::zero());
  assert_level_equal(pulse.volume, EnvelopeLevel());
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
