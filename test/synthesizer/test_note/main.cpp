#include "envelope.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <synth.hpp>
#include <unity.h>

Note note;
// Assume that base note is 100Hz to simplify calculations
Config config{.a440 = 100_hz};
NotePulse pulse;

void setUp(void) { note = Note(69, 127, 100_us, Envelope(EnvelopeLevel(1))); }

void tearDown(void) {}

void test_empty(void) {
  Note note;
  TEST_ASSERT_FALSE(note.is_active());
}

void test_started_note_initial_state(void) {
  TEST_ASSERT_TRUE(note.is_active());
  TEST_ASSERT_EQUAL(note.number(), 69);
  assert_duration_equal(note.time(), 100_us);
}

void test_note_tick_on(void) {
  TEST_ASSERT_TRUE(note.tick(config, pulse));
  assert_duration_equal(100_us, pulse.start);
  assert_duration_equal(200_us, pulse.off);
  assert_duration_equal(10100_us, pulse.end);
  assert_duration_equal(10100_us, note.time());
}

void test_note_tick_off(void) {
  note.release(30000_us);
  TEST_ASSERT_TRUE(note.tick(config, pulse));
  assert_duration_equal(100_us, pulse.start);
  assert_duration_equal(200_us, pulse.off);
  assert_duration_equal(10100_us, pulse.end);
  assert_duration_equal(10100_us, note.time());
}

void test_note_release(void) {
  TEST_ASSERT_FALSE(note.is_released());
  note.release(1000_us);
  TEST_ASSERT_TRUE(note.is_released());
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_started_note_initial_state);
  RUN_TEST(test_note_tick_on);
  RUN_TEST(test_note_tick_off);
  RUN_TEST(test_note_release);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
