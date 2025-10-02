#include <synth.hpp>
#include <unity.h>

Config config{};

#define TEST_ASSERT_EQUAL_DURATION(expected, actual)                           \
  TEST_ASSERT_TRUE((expected) == (actual))

void test_empty(void) {
  Notes notes;
  TEST_ASSERT_EQUAL(0, notes.active());

  NotePulse pulse;
  Note &note = notes.next();
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_FALSE(note.tick(config, pulse));
  TEST_ASSERT_EQUAL(0, note.number());
}

void test_start(void) {
  Notes notes;
  for (auto i = 0; i < config.notes; i++) {
    Note &note = notes.start(70 + i, 100, 0, 1000_us);
    TEST_ASSERT_EQUAL(i + 1, notes.active());

    TEST_ASSERT_TRUE(note.is_active());
    TEST_ASSERT_EQUAL(70 + i, note.number());
    TEST_ASSERT_EQUAL_DURATION(1000_us, note.time());
  }
}

void test_should_limit_concurrent_notes(void) {
  Notes notes(2);
  notes.start(70, 100, 0, 1000_us);
  notes.start(71, 100, 0, 1000_us);
  notes.start(72, 100, 0, 1000_us);
  TEST_ASSERT_EQUAL(2, notes.active());
}

void test_should_restart_the_same_note(void) {
  Notes notes(2);
  notes.start(70, 100, 0, 1000_us);
  notes.start(70, 100, 0, 2000_us);
  TEST_ASSERT_EQUAL(1, notes.active());
  auto note = notes.next();
  TEST_ASSERT_EQUAL(70, note.number());
  TEST_ASSERT_EQUAL_DURATION(2000_us, note.time());
}

void test_should_return_the_note_with_least_time(void) {
  Notes notes;
  notes.start(70, 100, 0, 2000_us);
  notes.start(71, 100, 0, 1000_us);
  notes.start(72, 100, 0, 1500_us);
  Note &note = notes.next();
  TEST_ASSERT_EQUAL(71, note.number());
  TEST_ASSERT_EQUAL_DURATION(1000_us, note.time());
}

void test_should_return_the_note_with_least_time_after_tick(void) {
  Notes notes;
  notes.start(70, 100, 0, 2000_us);
  notes.start(71, 100, 0, 1000_us);
  Note &note = notes.next();
  NotePulse pulse;
  note.tick(config, pulse);

  note = notes.next();
  TEST_ASSERT_EQUAL(70, note.number());
  TEST_ASSERT_EQUAL_DURATION(2000_us, note.time());
}

void test_should_release_note(void) {
  Notes notes;
  notes.start(70, 100, 0, 2000_us);
  Note &note = notes.next();
  TEST_ASSERT_FALSE(note.is_released());
  notes.release(70, 3000_us);
  note = notes.next();
  TEST_ASSERT_TRUE(note.is_released());
}

void test_should_not_release_other_notes(void) {
  Notes notes;
  notes.start(70, 100, 0, 2000_us);
  notes.start(71, 100, 0, 2000_us);
  Note &note = notes.next();
  TEST_ASSERT_FALSE(note.is_released());
  notes.release(70, 3000_us);
  note = notes.next();
  TEST_ASSERT_TRUE(note.is_released());
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_start);
  RUN_TEST(test_should_limit_concurrent_notes);
  RUN_TEST(test_should_restart_the_same_note);
  RUN_TEST(test_should_return_the_note_with_least_time);
  RUN_TEST(test_should_return_the_note_with_least_time_after_tick);
  RUN_TEST(test_should_release_note);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
