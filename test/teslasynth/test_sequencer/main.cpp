#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "midi_synth.hpp"
#include "notes.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include "unity_internals.h"
#include <cstdint>
#include <unity.h>

using namespace teslasynth::midisynth;

constexpr Config config_(uint8_t notes) {
  return {
      .min_deadtime = 100_us,
      .notes = notes,
  };
}
constexpr Config config = config_(4);
constexpr SynthConfig sconf = {.a440 = 100_hz};
constexpr MidiNote mnotef(int i) { return {static_cast<uint8_t>(69 + i), 127}; }
constexpr Instrument instrument{.envelope = ADSR::constant(EnvelopeLevel(1)),
                                .vibrato = Vibrato::none()};

void test_empty(void) {
  Teslasynth<> tsynth;
  assert_duration_equal(tsynth.track().played_time(0), Duration::zero());
}

void test_should_sequence_empty(void) {
  Teslasynth<> tsynth;
  auto &track = tsynth.track();
  assert_duration_equal(track.played_time(0), 0_ms);
  auto time = Duration::zero();
  for (auto i = 0; i < 100; i++) {
    auto step = Duration32::millis(i);
    auto pulse = tsynth.sample(0, step);
    TEST_ASSERT_TRUE(pulse.is_zero());
    assert_duration_equal(track.played_time(0), Duration::zero());
    assert_duration_equal(pulse.on, 0_ms);
    assert_duration_equal(pulse.off, step);
    time += step;
  }
}

void test_should_sequence_empty_when_no_notes_are_playing(void) {
  Teslasynth<> tsynth;
  auto &track = tsynth.track();

  tsynth.note_on(0, 69, 0, Duration::zero());
  tsynth.note_off(0, 69, Duration::zero());
  while (track.played_time(0) < 1_s) {
    auto pulse = tsynth.sample(0, 100_ms);
  }

  // Now there is no note playing
  Duration time = track.played_time(0);
  for (auto i = 0; i < 64; i++) {
    auto step = Duration32::millis(i);
    auto pulse = tsynth.sample(0, step);
    assert_duration_equal(track.played_time(0), time + step);
    assert_duration_equal(pulse.on, 0_ms);
    assert_duration_equal(pulse.off, step);
    time += step;
  }
}

void test_should_sequence_single(void) {
  Teslasynth<> tsynth(sconf);
  auto &track = tsynth.track();
  auto &voice = tsynth.voice(0);
  assert_duration_equal(track.played_time(0), 0_ms);
  tsynth.note_on(0, 69, 127, 10_ms);
  tsynth.note_off(0, 69, 1_s + 10_ms);

  for (auto i = 0; track.played_time(0) < 1_s; i++) {
    Pulse pulse2 = tsynth.sample(0, 10_ms);
    auto time = Duration32::millis(10) * i;
    assert_duration_equal(pulse2.on, config.max_on_time);
    assert_duration_equal(pulse2.off, config.min_deadtime);
    assert_duration_equal(track.played_time(0), time + pulse2.length());

    Pulse pulse3 = tsynth.sample(0, 10_ms);
    assert_duration_equal(pulse3.on, 0_us);
    assert_duration_equal(pulse3.off,
                          track.played_time(0) < 1_s ? 9800_us : 10_ms);
    assert_duration_equal(track.played_time(0),
                          time + 10_ms +
                              (track.played_time(0) < 1_s ? 0_us : 200_us));
  }
  assert_duration_equal(track.played_time(0), 1_s + 200_us);
  TEST_ASSERT_EQUAL(0, voice.active());
}

void test_should_sequence_polyphonic(void) {
  Teslasynth<> tsynth(sconf);
  auto &track = tsynth.track();

  assert_duration_equal(track.played_time(0), 0_ms);
  tsynth.note_on(0, mnotef(0), 10_ms);
  tsynth.note_on(0, mnotef(12), 10_ms);

  Pulse pulse1 = tsynth.sample(0, 10_ms);
  assert_duration_equal(pulse1.on, config.max_on_time);
  assert_duration_equal(pulse1.off, config.min_deadtime);
  assert_duration_equal(track.played_time(0), pulse1.length());

  Pulse pulse2 = tsynth.sample(0, 10_ms);
  assert_duration_equal(pulse2.on, 0_us);
  assert_duration_equal(pulse2.off, *(5_ms - pulse1.length()));
  assert_duration_equal(track.played_time(0), 5_ms);

  Pulse pulse3 = tsynth.sample(0, 10_ms);
  assert_duration_equal(pulse3.on, config.max_on_time);
  assert_duration_equal(pulse3.off, config.min_deadtime);
  assert_duration_equal(track.played_time(0), 5_ms + pulse3.length());
}

void test_should_sequence_polyphonic_out_of_phase(void) {
  Teslasynth<> tsynth(sconf);
  auto &track = tsynth.track();

  tsynth.note_on(0, mnotef(0), 10_ms);
  tsynth.note_on(0, mnotef(12), 11_ms);

  Pulse pulse2 = tsynth.sample(0, 10_ms);
  assert_duration_equal(pulse2.on, config.max_on_time);
  assert_duration_equal(pulse2.off, config.min_deadtime);
  assert_duration_equal(track.played_time(0), pulse2.length());

  Pulse pulse3 = tsynth.sample(0, 10_ms);
  assert_duration_equal(pulse3.on, 0_us);
  assert_duration_equal(pulse3.off, *(1_ms - pulse2.length()));
  assert_duration_equal(track.played_time(0), 1_ms);

  Pulse pulse4 = tsynth.sample(0, 10_ms);
  assert_duration_equal(pulse4.on, config.max_on_time);
  assert_duration_equal(pulse4.off, config.min_deadtime);
  assert_duration_equal(track.played_time(0), 1_ms + pulse4.length());
}

void test_should_sequence_polyphonic_out_of_phase_multichannel(void) {
  Teslasynth<2> tsynth(sconf);
  auto &track = tsynth.track();

  tsynth.note_on(0, mnotef(0), 10_ms);
  tsynth.note_on(0, mnotef(12), 11_ms);
  tsynth.note_on(1, mnotef(12), 12_ms);

  PulseBuffer<2> buffer;

  tsynth.sample_all(10_ms, buffer);

  TEST_ASSERT_EQUAL(6, buffer.written[0]);
  TEST_ASSERT_EQUAL(5, buffer.written[1]);

  std::vector<Pulse> ch0 = {
      {100_us, 100_us}, {0_s, 800_us},    {100_us, 100_us},
      {0_s, 4800_us},   {100_us, 100_us}, {0_s, 3800_us},
  };
  std::vector<Pulse> ch1 = {
      {0_s, 2_ms},      {100_us, 100_us}, {0_s, 4800_us},
      {100_us, 100_us}, {0_s, 2800_us},
  };

  for (auto i = 0; i < 6; i++) {
    assert_duration_equal(ch0[i].on, buffer.at(0, i).on);
    assert_duration_equal(ch0[i].off, buffer.at(0, i).off);
  }

  for (auto i = 0; i < 5; i++) {
    assert_duration_equal(ch1[i].on, buffer.at(1, i).on);
    assert_duration_equal(ch1[i].off, buffer.at(1, i).off);
  }
}

void test_should_sequence_polyphonic_out_of_phase_multichannel_note_off(void) {
  Teslasynth<2> tsynth(sconf);
  auto &track = tsynth.track();

  tsynth.note_on(0, mnotef(0), 10_ms);
  tsynth.note_on(0, mnotef(12), 11_ms);
  tsynth.note_on(1, mnotef(12), 12_ms);
  tsynth.note_off(1, mnotef(12), 22_ms);

  PulseBuffer<2> buffer;

  tsynth.sample_all(10_ms, buffer);
  tsynth.sample_all(10_ms, buffer);

  TEST_ASSERT_EQUAL(6, buffer.written[0]);
  TEST_ASSERT_EQUAL(1, buffer.written[1]);

  std::vector<Pulse> ch0 = {
      {100_us, 100_us}, {0_s, 800_us},    {100_us, 100_us},
      {0_s, 4800_us},   {100_us, 100_us}, {0_s, 3800_us},
  };
  std::vector<Pulse> ch1 = {{0_s, 10_ms}};

  for (auto i = 0; i < 6; i++) {
    assert_duration_equal(ch0[i].on, buffer.at(0, i).on);
    assert_duration_equal(ch0[i].off, buffer.at(0, i).off);
  }

  assert_duration_equal(ch1[0].on, buffer.at(1, 0).on);
  assert_duration_equal(ch1[0].off, buffer.at(1, 0).off);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_should_sequence_empty);
  RUN_TEST(test_should_sequence_empty_when_no_notes_are_playing);
  RUN_TEST(test_should_sequence_single);
  RUN_TEST(test_should_sequence_polyphonic);
  RUN_TEST(test_should_sequence_polyphonic_out_of_phase);
  RUN_TEST(test_should_sequence_polyphonic_out_of_phase_multichannel);
  RUN_TEST(test_should_sequence_polyphonic_out_of_phase_multichannel_note_off);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
