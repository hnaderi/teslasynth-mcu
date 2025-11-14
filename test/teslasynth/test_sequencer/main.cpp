#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "midi_synth.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstdint>
#include <unity.h>
#include <vector>

constexpr Config config_(uint8_t notes) {
  return {
      .min_deadtime = 100_us,
      .a440 = 100_hz,
      .notes = notes,
  };
}
constexpr Config config = config_(4);
constexpr MidiNote mnotef(int i) { return {static_cast<uint8_t>(69 + i), 127}; }
constexpr Instrument instrument{.envelope = ADSR::constant(EnvelopeLevel(1)),
                                .vibrato = Vibrato::none()};

class FakeNotes {
  Note note;

public:
  struct Started {
    MidiNote mnote;
    Duration time;
    Instrument instrument;
    Config config;
  };

  struct Released {
    uint8_t mnote;
    Duration time;
  };

private:
  std::vector<Started> started_;
  std::vector<Released> released_;

public:
  Note &start(const MidiNote &mnote, Duration time,
              const Instrument &instrument, const Config &config) {
    started_.push_back({mnote, time, instrument, config});
    return note;
  }
  void release(uint8_t number, Duration time) {
    released_.push_back({number, time});
  }

  const std::vector<Started> started() const { return started_; }
  const std::vector<Released> released() const { return released_; }
};

void test_empty(void) {
  Notes notes;
  TrackState track;
  Sequencer<> seq(config, notes, track);
  assert_duration_equal(track.played_time(), Duration::zero());
}

void test_should_sequence_empty(void) {
  Notes notes(config);
  TrackState track;
  Sequencer<> seq(config, notes, track);
  assert_duration_equal(track.played_time(), 0_ms);
  Duration time = Duration::zero();
  for (auto i = 0; i < 100; i++) {
    Duration step = Duration::millis(i);
    auto pulse = seq.sample(step);
    TEST_ASSERT_TRUE(pulse.is_zero());
    assert_duration_equal(track.played_time(), Duration::zero());
    assert_duration_equal(pulse.on, 0_ms);
    assert_duration_equal(pulse.off, step);
    time += step;
  }
}

void test_should_sequence_empty_when_no_notes_are_playing(void) {
  Notes notes(config);
  TrackState track;
  track.on_receive(Duration::zero());
  Sequencer<> seq(config, notes, track);
  assert_duration_equal(track.played_time(), 0_ms);
  Duration time = Duration::zero();
  for (auto i = 0; i < 100; i++) {
    Duration step = Duration::millis(i);
    auto pulse = seq.sample(step);
    assert_duration_equal(track.played_time(), time + step);
    assert_duration_equal(pulse.on, 0_ms);
    assert_duration_equal(pulse.off, step);
    time += step;
  }
}

void test_should_sequence_single(void) {
  Notes notes(config);
  TrackState track;
  track.on_receive(Duration::zero());
  Sequencer<> seq(config, notes, track);
  assert_duration_equal(track.played_time(), 0_ms);
  notes.start(mnotef(0), 10_ms, instrument, config);

  Pulse pulse = seq.sample(20_ms);
  assert_duration_equal(track.played_time(), 10_ms);
  assert_duration_equal(pulse.on, 0_ms);
  assert_duration_equal(pulse.off, 10_ms);

  notes.release(mnotef(0), 1_s);

  for (auto i = 1; track.played_time() < 1_s; i++) {
    Pulse pulse2 = seq.sample(10_ms);
    Duration time = 10_ms * i;
    assert_duration_equal(pulse2.on, config.max_on_time);
    assert_duration_equal(pulse2.off, config.min_deadtime);
    assert_duration_equal(track.played_time(), time + pulse2.length());

    Pulse pulse3 = seq.sample(10_ms);
    assert_duration_equal(pulse3.on, 0_us);
    assert_duration_equal(pulse3.off,
                          track.played_time() < 1_s ? 9800_us : 10_ms);
    assert_duration_equal(track.played_time(),
                          time + 10_ms +
                              (track.played_time() < 1_s ? 0_us : 200_us));
  }
  assert_duration_equal(track.played_time(), 1_s + 200_us);
  TEST_ASSERT_EQUAL(0, notes.active());
}

void test_should_sequence_polyphonic(void) {
  Notes notes(config);
  TrackState track;
  track.on_receive(Duration::zero());
  Sequencer<> seq(config, notes, track);
  assert_duration_equal(track.played_time(), 0_ms);
  notes.start(mnotef(0), 10_ms, instrument, config);
  notes.start(mnotef(12), 10_ms, instrument, config);

  Pulse pulse = seq.sample(20_ms);
  assert_duration_equal(track.played_time(), 10_ms);
  assert_duration_equal(pulse.on, 0_ms);
  assert_duration_equal(pulse.off, 10_ms);

  Pulse pulse2 = seq.sample(10_ms);
  assert_duration_equal(pulse2.on, config.max_on_time);
  assert_duration_equal(pulse2.off, config.min_deadtime);
  assert_duration_equal(track.played_time(), 10_ms + pulse2.length());

  Pulse pulse3 = seq.sample(10_ms);
  assert_duration_equal(pulse3.on, 0_us);
  assert_duration_equal(pulse3.off, *(5_ms - pulse2.length()));
  assert_duration_equal(track.played_time(), 15_ms);

  Pulse pulse4 = seq.sample(10_ms);
  assert_duration_equal(pulse4.on, config.max_on_time);
  assert_duration_equal(pulse4.off, config.min_deadtime);
  assert_duration_equal(track.played_time(), 15_ms + pulse4.length());
}

void test_should_sequence_polyphonic_out_of_phase(void) {
  Notes notes(config);
  TrackState track;
  track.on_receive(Duration::zero());
  Sequencer<> seq(config, notes, track);
  assert_duration_equal(track.played_time(), 0_ms);
  notes.start(mnotef(0), 10_ms, instrument, config);
  notes.start(mnotef(12), 11_ms, instrument, config);

  Pulse pulse = seq.sample(20_ms);
  assert_duration_equal(track.played_time(), 10_ms);
  assert_duration_equal(pulse.on, 0_ms);
  assert_duration_equal(pulse.off, 10_ms);

  Pulse pulse2 = seq.sample(10_ms);
  assert_duration_equal(pulse2.on, config.max_on_time);
  assert_duration_equal(pulse2.off, config.min_deadtime);
  assert_duration_equal(track.played_time(), 10_ms + pulse2.length());

  Pulse pulse3 = seq.sample(10_ms);
  assert_duration_equal(pulse3.on, 0_us);
  assert_duration_equal(pulse3.off, *(1_ms - pulse2.length()));
  assert_duration_equal(track.played_time(), 11_ms);

  Pulse pulse4 = seq.sample(10_ms);
  assert_duration_equal(pulse4.on, config.max_on_time);
  assert_duration_equal(pulse4.off, config.min_deadtime);
  assert_duration_equal(track.played_time(), 11_ms + pulse4.length());
}

void assert_plays(Sequencer<> &seq, TrackState &track, Duration duty,
                  Duration period, Duration start, Duration release) {
  while (track.played_time() < start) {
    seq.sample(1_ms);
  }
  assert_duration_equal(track.played_time(), start);

  for (auto i = 0; track.played_time() < release; i++) {
    Pulse pulse = seq.sample(10_ms);
    Duration time = period * i;
    assert_duration_equal(pulse.on, duty);
    assert_duration_equal(pulse.off, config.min_deadtime);
    assert_duration_equal(track.played_time(), start + time + pulse.length());

    while (track.played_time() < start + time + period) {
      Pulse pulse2 = seq.sample(10_ms);
      TEST_ASSERT_TRUE(pulse2.is_zero());
    }
    TEST_ASSERT_TRUE(track.played_time() >= start + time + period);
  }
  TEST_ASSERT_TRUE(track.played_time() >= release);
}

void test_should_sequence_monophonic_tracks(void) {
  Notes notes(config_(2));
  TrackState track;
  track.on_receive(Duration::zero());
  Sequencer<> seq(config, notes, track);
  assert_duration_equal(track.played_time(), 0_ms);

  Note *n1 = &notes.start(mnotef(0), 10_ms, instrument, config);
  notes.release(mnotef(0), 1_s);
  Note *n2 = &notes.start(mnotef(12), 1_s, instrument, config);
  notes.release(mnotef(12), 2_s);

  TEST_ASSERT_EQUAL(2, notes.active());
  assert_plays(seq, track, 100_us, 10_ms, 10_ms, 1_s);
  TEST_ASSERT_EQUAL(1, notes.active());
  assert_plays(seq, track, 100_us, 5_ms, 1_s, 2_s);
  TEST_ASSERT_EQUAL(0, notes.active());

  Note *n3 = &notes.start(mnotef(-12), 2250_ms, instrument, config);
  assert_hertz_equal(n3->frequency(), mnotef(-12).frequency(100_hz));
  notes.release(mnotef(-12), 3_s);
  TEST_ASSERT_EQUAL(1, notes.active());
  TEST_ASSERT_TRUE(n3->is_active());
  TEST_ASSERT_TRUE(n3 == n1);
  assert_plays(seq, track, 100_us, 20_ms, 2250_ms, 3_s);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_empty);
  RUN_TEST(test_should_sequence_empty);
  RUN_TEST(test_should_sequence_empty_when_no_notes_are_playing);
  RUN_TEST(test_should_sequence_single);
  RUN_TEST(test_should_sequence_polyphonic);
  RUN_TEST(test_should_sequence_polyphonic_out_of_phase);
  RUN_TEST(test_should_sequence_monophonic_tracks);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
