#include "envelope.hpp"
#include "lfo.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <synth.hpp>
#include <unity.h>

Note note;
// Assume that base note is 100Hz to simplify calculations
constexpr Config config{.a440 = 100_hz};
constexpr MidiNote mnote1{69, 127}, mnote2{69 + 12, 127},
    mnote3{69 + 2 * 12, 127};
const Envelope envelope(EnvelopeLevel(1));

void setUp(void) { note.start(mnote1, 100_us, envelope, config); }

void tearDown(void) {}

void test_empty(void) {
  Note note;
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_FALSE(note.is_released());
  TEST_ASSERT_TRUE(note.frequency().is_zero());

  TEST_ASSERT_TRUE(note.current().start.is_zero());
  TEST_ASSERT_TRUE(note.current().duty.is_zero());
  TEST_ASSERT_TRUE(note.current().period.is_zero());

  TEST_ASSERT_FALSE(note.next());

  TEST_ASSERT_TRUE(note.current().start.is_zero());
  TEST_ASSERT_TRUE(note.current().duty.is_zero());
  TEST_ASSERT_TRUE(note.current().period.is_zero());
}

void test_midi_note_frequency(void) {
  assert_hertz_equal(mnote1.frequency(config), 100_hz);
  assert_hertz_equal(mnote2.frequency(config), 200_hz);
  assert_hertz_equal(mnote3.frequency(config), 400_hz);

  assert_hertz_equal(mnote1.frequency({}), 440_hz);
  assert_hertz_equal(mnote2.frequency({}), 880_hz);
  assert_hertz_equal(mnote3.frequency({}), 1760_hz);
}

void test_started_note_initial_state(void) {
  TEST_ASSERT_TRUE(note.is_active());
  TEST_ASSERT_FALSE(note.is_released());
  assert_duration_equal(note.current().start, 100_us);
  assert_duration_equal(note.current().duty, 100_us);
  assert_duration_equal(note.current().period, 10000_us);
}

void test_started_note_initial_time(void) {
  note.start(mnote1, 100_s, envelope, config);
  assert_duration_equal(note.now(), 100_s + 10_ms);
}

void test_note_next(void) {
  note.next();
  assert_duration_equal(note.current().start, 10100_us);
  assert_duration_equal(note.current().duty, 100_us);
  assert_duration_equal(note.current().period, 10_ms);
}

void test_note_release(void) {
  note.release(30000_us);

  TEST_ASSERT_TRUE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());

  TEST_ASSERT_TRUE(note.next());
  TEST_ASSERT_TRUE(note.next());
  TEST_ASSERT_FALSE(note.next());

  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());
  assert_duration_equal(note.now(), 30100_us);
  assert_duration_equal(note.current().start, 20100_us);
  assert_duration_equal(note.current().duty, 100_us);
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_FALSE(note.next());
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());
  assert_duration_equal(note.now(), 30100_us);
  assert_duration_equal(note.current().start, 20100_us);
  assert_duration_equal(note.current().duty, 100_us);
  assert_duration_equal(note.current().period, 10_ms);
}

void test_note_release_with_zero_velocity(void) {
  note.start({mnote1.number, 0}, 30000_us, envelope, config);

  TEST_ASSERT_TRUE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());

  TEST_ASSERT_TRUE(note.next());
  TEST_ASSERT_TRUE(note.next());
  TEST_ASSERT_FALSE(note.next());

  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());
  assert_duration_equal(note.now(), 30100_us);
  assert_duration_equal(note.current().start, 20100_us);
  assert_duration_equal(note.current().duty, 100_us);
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_FALSE(note.next());
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());
  assert_duration_equal(note.now(), 30100_us);
  assert_duration_equal(note.current().start, 20100_us);
  assert_duration_equal(note.current().duty, 100_us);
  assert_duration_equal(note.current().period, 10_ms);
}

void test_note_second_start(void) {
  note.next();
  assert_duration_equal(note.current().start, 10100_us);
  assert_duration_equal(note.current().duty, 100_us);
  assert_duration_equal(note.current().period, 10_ms);

  note.start({69 + 12, 127}, 100_ms, envelope, config);
  assert_duration_equal(note.current().start, 100_ms);
  assert_duration_equal(note.current().duty, 100_us);
  assert_duration_equal(note.current().period, 5_ms);
}

void test_note_start_after_release(void) {
  note.release(30000_us);
  test_note_second_start();
}

void test_note_envelope(void) {
  Envelope envelope(
      ADSR{200_ms, 200_ms, EnvelopeLevel(0.5), 20_ms, CurveType::Lin});
  note.start(mnote1, 0_us, envelope, config);
  assert_duration_equal(note.current().start, 0_ms);
  assert_duration_equal(note.current().duty, 0_us);
  assert_duration_equal(note.current().period, 10_ms);

  note.next();
  assert_duration_equal(note.current().start, 10_ms);
  assert_duration_equal(note.current().duty, 5_us);
  assert_duration_equal(note.current().period, 10_ms);

  note.release(500_ms);
  // Let's fast forward to 400ms (sustain)
  for (int i = 0; i < 38; i++)
    TEST_ASSERT_TRUE(note.next());

  // Now we're at 400ms and on sustain
  assert_duration_equal(note.now(), 400_ms);
  for (int i = 0; i < 10; i++) {
    note.next();
    auto start = 400_ms + 10_ms * i;
    assert_duration_equal(note.current().start, start);
    assert_duration_equal(note.current().duty, 50_us);
    assert_duration_equal(note.current().period, 10_ms);
  }

  // Now the release cycle has begun
  assert_duration_equal(note.now(), 500_ms);
  TEST_ASSERT_TRUE(note.next());
  assert_duration_equal(note.current().start, 500_ms);
  assert_duration_equal(note.current().duty, 50_us);
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_TRUE(note.next());
  assert_duration_equal(note.current().start, 510_ms);
  assert_duration_equal(note.current().duty, 25_us);
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_FALSE(note.next());
}

void test_note_vibrato(void) {
  Vibrato vib{1_hz, 50_hz};
  note.start(mnote1, 0_us, Envelope(EnvelopeLevel(1)), vib, config);
  assert_duration_equal(note.now(), 10_ms);
  for (int i = 0; i < 5000; i++) {
    auto start = note.current().start;
    auto freq = 100_hz + vib.offset(start);
    assert_duration_equal(note.current().duty, 100_us);
    assert_duration_equal(note.current().period, freq.period());

    note.next();
  }
}

void test_off(void) {
  note.off();
  TEST_ASSERT_FALSE(note.is_active());
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_midi_note_frequency);
  RUN_TEST(test_started_note_initial_state);
  RUN_TEST(test_started_note_initial_time);
  RUN_TEST(test_note_next);
  RUN_TEST(test_note_release);
  RUN_TEST(test_note_release_with_zero_velocity);
  RUN_TEST(test_note_second_start);
  RUN_TEST(test_note_start_after_release);
  RUN_TEST(test_note_envelope);
  RUN_TEST(test_note_vibrato);
  RUN_TEST(test_off);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
