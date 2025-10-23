#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstddef>
#include <cstdint>
#include "notes.hpp"
#include <unity.h>

Config config{.a440 = 100_hz};
Instrument instrument{.envelope = ADSR::constant(EnvelopeLevel(1)),
                      .vibrato = Vibrato::none()};
constexpr MidiNote mnotef(int i) { return {static_cast<uint8_t>(69 + i), 127}; }

void test_empty(void) {
  Notes notes;
  TEST_ASSERT_EQUAL(0, notes.active());

  Note &note = notes.next();
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_FALSE(note.next());
}

void assert_note(Notes &notes, const MidiNote &mnote, const Duration &time) {
  Note &note = notes.start(mnote, time, instrument, config);

  TEST_ASSERT_TRUE(note.is_active());
  assert_hertz_equal(note.frequency(), mnote.frequency(config));
  assert_duration_equal(time, note.current().start);
}

void test_start(void) {
  Notes notes;
  for (size_t i = 0; i < config.notes; i++) {
    auto mnote = mnotef(i);
    Duration time = 1000_us * static_cast<int>(i);
    assert_note(notes, mnote, time);
    TEST_ASSERT_EQUAL(i + 1, notes.active());
  }
}

void test_should_limit_concurrent_notes(void) {
  for (size_t max = 1; max < 5; max++) {
    Notes notes(max);
    for (uint8_t i = 0; i < max; i++) {
      assert_note(notes, mnotef(i), 100_us * i);
      TEST_ASSERT_EQUAL(i + 1, notes.active());
    }
    TEST_ASSERT_EQUAL(max, notes.active());
    for (uint8_t i = 5; i < max; i++) {
      assert_note(notes, mnotef(i), 100_us * i);
      TEST_ASSERT_EQUAL(max, notes.active());
    }
  }
}

void test_should_restart_the_same_note(void) {
  Notes notes(2);
  assert_note(notes, mnotef(0), 200_us);
  assert_note(notes, mnotef(0), 100_us);
  TEST_ASSERT_EQUAL(1, notes.active());
  auto note = notes.next();
  assert_duration_equal(note.current().start, 100_us);
}

void test_should_return_the_note_with_least_time(void) {
  Notes notes;
  assert_note(notes, mnotef(1), 200_us);
  assert_note(notes, mnotef(2), 50_us);
  assert_note(notes, mnotef(3), 100_us);
  Note &note = notes.next();
  assert_hertz_equal(note.frequency(), mnotef(2).frequency(config));
  assert_duration_equal(note.current().start, 50_us);
}

void test_should_return_the_note_with_least_time_after_tick(void) {
  Notes notes(4);
  assert_note(notes, mnotef(1), 200_us);
  assert_note(notes, mnotef(2), 50_us);
  assert_note(notes, mnotef(3), 100_us);
  Note &note1 = notes.next();
  assert_hertz_equal(note1.frequency(), mnotef(2).frequency(config));
  assert_duration_equal(note1.current().start, 50_us);
  TEST_ASSERT_TRUE(note1.now() > 200_us);

  TEST_ASSERT_TRUE(note1.next());
  TEST_ASSERT_TRUE(note1.current().start > 200_us);

  Note &note2 = notes.next();
  assert_hertz_equal(note2.frequency(), mnotef(3).frequency(config));
  assert_duration_equal(note2.current().start, 100_us);
  TEST_ASSERT_TRUE(note2.now() > 200_us);

  TEST_ASSERT_TRUE(note2.next());
  TEST_ASSERT_TRUE(note2.current().start > 200_us);

  Note &note3 = notes.next();
  assert_duration_equal(note3.current().start, 200_us);
  assert_hertz_equal(note3.frequency(), mnotef(1).frequency(config));
}

void test_should_release_note(void) {
  Notes notes;
  auto mnote = mnotef(1);
  assert_note(notes, mnote, 200_ms);
  Note &note = notes.next();
  TEST_ASSERT_FALSE(note.is_released());
  notes.release(mnote.number, 3000_ms);
  note = notes.next();
  TEST_ASSERT_TRUE(note.is_released());
}

void test_should_release_note_on_start_with_zero_velocity(void) {
  Notes notes;
  auto mnote = mnotef(1);
  assert_note(notes, mnote, 200_ms);
  Note &note = notes.next();
  TEST_ASSERT_FALSE(note.is_released());
  notes.start({mnote.number, 0}, 3000_ms, instrument, config);
  note = notes.next();
  TEST_ASSERT_TRUE(note.is_released());
}

void test_should_not_release_other_notes(void) {
  Notes notes;
  assert_note(notes, mnotef(0), 100_ms);
  assert_note(notes, mnotef(1), 200_ms);
  Note &note = notes.next();
  TEST_ASSERT_FALSE(note.is_released());
  notes.release(mnotef(1).number, 3000_us);
  TEST_ASSERT_FALSE(note.is_released());
}

void test_should_allow_the_minimum_size_of_one(void) {
  Notes notes(1);
  assert_note(notes, mnotef(0), 100_ms);
  assert_note(notes, mnotef(1), 200_ms);
  TEST_ASSERT_EQUAL(notes.active(), 1);
  TEST_ASSERT_EQUAL(notes.size(), 1);

  Note &note = notes.next();
  assert_hertz_equal(note.frequency(), mnotef(1).frequency(config));
}

void test_off(void) {
  Notes notes;
  assert_note(notes, mnotef(0), 200_ms);
  assert_note(notes, mnotef(1), 200_ms);
  TEST_ASSERT_EQUAL(2, notes.active());
  notes.off();
  TEST_ASSERT_EQUAL(0, notes.active());
  Note &note = notes.next();
  TEST_ASSERT_FALSE(note.is_active());
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
  RUN_TEST(test_should_release_note_on_start_with_zero_velocity);
  RUN_TEST(test_should_not_release_other_notes);
  RUN_TEST(test_should_allow_the_minimum_size_of_one);
  RUN_TEST(test_off);

  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
