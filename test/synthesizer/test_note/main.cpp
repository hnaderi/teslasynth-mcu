#include <string>
#include <synth.hpp>
#include <unity.h>

Note note;
// Assume to base note is 100Hz to simplify calculations
Config config{.a440 = 100_hz};
NotePulse pulse;

#define TEST_ASSERT_EQUAL_DURATION(expected, actual)                           \
  {                                                                            \
    std::string msg = (std::string("Expected: ") + expected.print() +          \
                       " Actual: " + actual.print());                          \
    TEST_ASSERT_TRUE_MESSAGE((expected) == (actual), msg.c_str());             \
  };

void setUp(void) { note = Note(69, 127, 0, 100_us); }

void tearDown(void) {}

void test_empty(void) {
  Note note;
  TEST_ASSERT_FALSE(note.is_active());
}

void test_started_note_initial_state(void) {
  TEST_ASSERT_TRUE(note.is_active());
  TEST_ASSERT_EQUAL(note.number(), 69);
  TEST_ASSERT_EQUAL_DURATION(note.time(), 100_us);
}

void test_note_tick_on(void) {
  TEST_ASSERT_TRUE(note.tick(config, pulse));
  TEST_ASSERT_EQUAL_DURATION(100_us, pulse.start);
  TEST_ASSERT_EQUAL_DURATION(200_us, pulse.off);
  TEST_ASSERT_EQUAL_DURATION(10100_us, pulse.end);
  TEST_ASSERT_EQUAL_DURATION(10100_us, note.time());
}

void test_note_tick_off(void) {
  note.release(30000_us);
  TEST_ASSERT_TRUE(note.tick(config, pulse));
  TEST_ASSERT_EQUAL_DURATION(100_us, pulse.start);
  TEST_ASSERT_EQUAL_DURATION(200_us, pulse.off);
  TEST_ASSERT_EQUAL_DURATION(10100_us, pulse.end);
  TEST_ASSERT_EQUAL_DURATION(10100_us, note.time());
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
