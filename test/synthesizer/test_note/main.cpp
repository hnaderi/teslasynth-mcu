#include <synth.hpp>
#include <unity.h>

Note note;
// Assume to base note is 100Hz to simplify calculations
Config config{.a440 = 100};
NotePulse pulse;

void setUp(void) { note = Note(69, 127, 0, 100); }

void tearDown(void) {}

void test_empty(void) {
  Note note;
  TEST_ASSERT_FALSE(note.is_active());
}

void test_started_note_initial_state(void) {
  TEST_ASSERT_TRUE(note.is_active());
  TEST_ASSERT_EQUAL(note.number(), 69);
  TEST_ASSERT_EQUAL(note.time(), 100);
}

void test_note_tick_on(void) {
  TEST_ASSERT_TRUE(note.tick(config, pulse));
  TEST_ASSERT_EQUAL(100, pulse.start);
  TEST_ASSERT_EQUAL(300, pulse.off);
  TEST_ASSERT_EQUAL(20100, pulse.end);
  TEST_ASSERT_EQUAL(20100, note.time());
}

void test_note_tick_off(void) {
  note.release(30000);
  TEST_ASSERT_TRUE(note.tick(config, pulse));
  TEST_ASSERT_EQUAL(100, pulse.start);
  TEST_ASSERT_EQUAL(300, pulse.off);
  TEST_ASSERT_EQUAL(20100, pulse.end);
  TEST_ASSERT_EQUAL(20100, note.time());
}

void test_note_release(void) {
  TEST_ASSERT_FALSE(note.is_released());
  note.release(1000);
  TEST_ASSERT_TRUE(note.is_released());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_started_note_initial_state);
  RUN_TEST(test_note_tick_on);
  RUN_TEST(test_note_tick_off);
  RUN_TEST(test_note_release);
  UNITY_END();
}
