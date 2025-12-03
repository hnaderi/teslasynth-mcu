#include "core.hpp"
#include "envelope.hpp"
#include "lfo.hpp"
#include "notes.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstdint>
#include <unity.h>

using namespace teslasynth::synth;

Note note;
// Assume that base note is 100Hz to simplify calculations
constexpr Hertz tuning = 100_hz;
const Config config;
constexpr MidiNote mnote(uint8_t i, uint8_t velocity = 127) {
  return {static_cast<uint8_t>(69 + i), velocity};
}

constexpr MidiNote mnote1 = mnote(0), mnote2 = mnote(12),
                   mnote3 = mnote(2 * 12);
const Envelope envelope(EnvelopeLevel(1));

void setUp(void) { note.start(mnote1, 100_us, envelope, tuning); }

void tearDown(void) {}

void test_empty(void) {
  Note note;
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_FALSE(note.is_released());
  TEST_ASSERT_TRUE(note.frequency().is_zero());

  TEST_ASSERT_TRUE(note.current().start.is_zero());
  TEST_ASSERT_TRUE(note.current().volume.is_zero());
  TEST_ASSERT_TRUE(note.current().period.is_zero());

  TEST_ASSERT_FALSE(note.next());

  TEST_ASSERT_TRUE(note.current().start.is_zero());
  TEST_ASSERT_TRUE(note.current().volume.is_zero());
  TEST_ASSERT_TRUE(note.current().period.is_zero());
}

void test_midi_note_frequency(void) {
  assert_hertz_equal(mnote1.frequency(tuning), 100_hz);
  assert_hertz_equal(mnote2.frequency(tuning), 200_hz);
  assert_hertz_equal(mnote3.frequency(tuning), 400_hz);

  assert_hertz_equal(mnote1.frequency(), 440_hz);
  assert_hertz_equal(mnote2.frequency(), 880_hz);
  assert_hertz_equal(mnote3.frequency(), 1760_hz);
}

void test_started_note_initial_state(void) {
  TEST_ASSERT_TRUE(note.is_active());
  TEST_ASSERT_FALSE(note.is_released());
  assert_duration_equal(note.current().start, 100_us);
  assert_level_equal(note.current().volume, EnvelopeLevel::max());
  assert_duration_equal(note.current().period, 10000_us);
}

void test_started_note_initial_time(void) {
  note.start(mnote1, 100_s, envelope, tuning);
  assert_duration_equal(note.now(), 100_s + 10_ms);
}

void test_note_next(void) {
  note.next();
  assert_duration_equal(note.current().start, 10100_us);
  assert_level_equal(note.current().volume, EnvelopeLevel::max());
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
  assert_level_equal(note.current().volume, EnvelopeLevel::max());
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_FALSE(note.next());
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());
  assert_duration_equal(note.now(), 30100_us);
  assert_duration_equal(note.current().start, 20100_us);
  assert_level_equal(note.current().volume, EnvelopeLevel::max());
  assert_duration_equal(note.current().period, 10_ms);
}

void test_note_release_with_zero_velocity(void) {
  note.start({mnote1.number, 0}, 30000_us, envelope, tuning);

  TEST_ASSERT_TRUE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());

  TEST_ASSERT_TRUE(note.next());
  TEST_ASSERT_TRUE(note.next());
  TEST_ASSERT_FALSE(note.next());

  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());
  assert_duration_equal(note.now(), 30100_us);
  assert_duration_equal(note.current().start, 20100_us);
  assert_level_equal(note.current().volume, EnvelopeLevel::max());
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_FALSE(note.next());
  TEST_ASSERT_FALSE(note.is_active());
  TEST_ASSERT_TRUE(note.is_released());
  assert_duration_equal(note.now(), 30100_us);
  assert_duration_equal(note.current().start, 20100_us);
  assert_level_equal(note.current().volume, EnvelopeLevel::max());
  assert_duration_equal(note.current().period, 10_ms);
}

void test_note_second_start(void) {
  note.next();
  assert_duration_equal(note.current().start, 10100_us);
  assert_level_equal(note.current().volume, EnvelopeLevel::max());
  assert_duration_equal(note.current().period, 10_ms);

  note.start({69 + 12, 127}, 100_ms, envelope, tuning);
  assert_duration_equal(note.current().start, 100_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel::max());
  assert_duration_equal(note.current().period, 5_ms);

  TEST_ASSERT_FALSE(note.is_released());
  TEST_ASSERT_TRUE(note.is_active());
}

void test_note_start_after_release(void) {
  note.release(30000_us);
  test_note_second_start();
}

void test_note_envelope(void) {
  Envelope envelope(
      ADSR{200_ms, 200_ms, EnvelopeLevel(0.5), 20_ms, CurveType::Lin});
  note.start(mnote1, 0_us, envelope, tuning);
  assert_duration_equal(note.current().start, 0_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel::zero());
  assert_duration_equal(note.current().period, 10_ms);

  note.next();
  assert_duration_equal(note.current().start, 10_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel(0.05f));
  assert_duration_equal(note.current().period, 10_ms);

  note.release(500_ms);
  // Let's fast forward to 400ms (sustain)
  for (int i = 0; i < 38; i++)
    TEST_ASSERT_TRUE(note.next());

  // Now we're at 400ms and on sustain
  assert_duration_equal(note.now(), 400_ms);
  Duration32 start = 400_ms;
  for (int i = 0; i < 10; i++) {
    note.next();
    assert_duration_equal(note.current().start, start);
    assert_level_equal(note.current().volume, EnvelopeLevel(0.5f));
    assert_duration_equal(note.current().period, 10_ms);
    start += 10_ms;
  }

  // Now the release cycle has begun
  assert_duration_equal(note.now(), 500_ms);
  TEST_ASSERT_TRUE(note.next());
  assert_duration_equal(note.current().start, 500_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel(0.5f));
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_TRUE(note.next());
  assert_duration_equal(note.current().start, 510_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel(0.25f));
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_FALSE(note.next());
}

void test_note_envelope2(void) {
  Envelope envelope(
      ADSR{200_ms, 200_ms, EnvelopeLevel(0.5), 20_ms, CurveType::Lin});
  note.start(mnote1, 0_us, envelope, tuning);
  assert_duration_equal(note.current().start, 0_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel::zero());
  assert_duration_equal(note.current().period, 10_ms);

  note.next();
  assert_duration_equal(note.current().start, 10_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel(0.05f));
  assert_duration_equal(note.current().period, 10_ms);

  note.release(496_ms);
  // Let's fast forward to 400ms (sustain)
  for (int i = 0; i < 38; i++)
    TEST_ASSERT_TRUE(note.next());

  // Now we're at 400ms and on sustain
  assert_duration_equal(note.now(), 400_ms);
  Duration32 start = 400_ms;
  for (int i = 0; i < 10; i++) {
    note.next();
    assert_duration_equal(note.current().start, start);
    assert_level_equal(note.current().volume, EnvelopeLevel(0.5f));
    assert_duration_equal(note.current().period, 10_ms);
    start += 10_ms;
  }

  // Now the release cycle has begun
  assert_duration_equal(note.now(), 500_ms);
  TEST_ASSERT_TRUE(note.next());
  assert_duration_equal(note.current().start, 500_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel(0.4f));
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_TRUE(note.next());
  assert_duration_equal(note.current().start, 510_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel(0.15f));
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_FALSE(note.next());
}

void test_note_volume(void) {
  Envelope envelope(
      ADSR{200_ms, 200_ms, EnvelopeLevel(0.5), 20_ms, CurveType::Lin});

  auto volume = EnvelopeLevel(7.f / 8);
  note.start(mnote(0, 63), 0_us, envelope, tuning);
  assert_level_equal(note.max_volume(), volume);

  assert_duration_equal(note.current().start, 0_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel::zero());
  assert_duration_equal(note.current().period, 10_ms);

  note.next();
  assert_duration_equal(note.current().start, 10_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel(0.05f) * volume);
  assert_duration_equal(note.current().period, 10_ms);

  note.release(500_ms);
  // Let's fast forward to 400ms (sustain)
  for (int i = 0; i < 38; i++)
    TEST_ASSERT_TRUE(note.next());

  // Now we're at 400ms and on sustain
  assert_duration_equal(note.now(), 400_ms);
  Duration32 start = 400_ms;
  for (int i = 0; i < 10; i++) {
    note.next();
    assert_duration_equal(note.current().start, start);
    assert_level_equal(note.current().volume, EnvelopeLevel(0.5f) * volume);
    assert_duration_equal(note.current().period, 10_ms);

    start += 10_ms;
  }

  // Now the release cycle has begun
  assert_duration_equal(note.now(), 500_ms);
  TEST_ASSERT_TRUE(note.next());
  assert_duration_equal(note.current().start, 500_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel(0.5f) * volume);
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_TRUE(note.next());
  assert_duration_equal(note.current().start, 510_ms);
  assert_level_equal(note.current().volume, EnvelopeLevel(0.25f) * volume);
  assert_duration_equal(note.current().period, 10_ms);

  TEST_ASSERT_FALSE(note.next());
}

void test_note_envelope_constant(void) {
  for (auto n = 1; n <= 10; n++) {
    EnvelopeLevel volume(0.1 * n);
    Envelope envelope(volume);
    note.start(mnote1, 100_ms, envelope, tuning);
    note.release(490_ms + Duration::millis(n));

    Duration32 time = 100_ms;
    for (int i = 0; note.now() < 500_ms; i++) {
      assert_duration_equal(note.current().start, time);
      assert_level_equal(note.current().volume, volume);
      assert_duration_equal(note.current().period, 10_ms);
      TEST_ASSERT_TRUE(note.next());
      time += 10_ms;
    }

    assert_duration_equal(note.now(), 500_ms);

    for (int i = 0; i < 10; i++) {
      TEST_ASSERT_FALSE(note.next());
      assert_duration_equal(note.current().start, 490_ms);
      assert_level_equal(note.current().volume, volume);
      assert_duration_equal(note.current().period, 10_ms);
    }
  }
}

void test_note_vibrato(void) {
  Vibrato vib{1_hz, 50_hz};
  note.start(mnote1, 0_us, Envelope(EnvelopeLevel(1)), vib, tuning);
  assert_duration_equal(note.now(), 10_ms);
  for (int i = 0; i < 5000; i++) {
    auto start = note.current().start;
    auto freq = 100_hz + vib.offset(start);
    assert_level_equal(note.current().volume, EnvelopeLevel::max());
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
  RUN_TEST(test_note_volume);
  RUN_TEST(test_note_envelope);
  RUN_TEST(test_note_envelope2);
  RUN_TEST(test_note_envelope_constant);
  RUN_TEST(test_note_vibrato);
  RUN_TEST(test_off);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
