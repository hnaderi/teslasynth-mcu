#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "midi_synth.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstdint>
#include <unity.h>
#include <vector>

constexpr Config config{
    .min_deadtime = 100_us,
    .a440 = 100_hz,
};
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
  Sequencer<> seq(config, notes);
  assert_duration_equal(seq.clock(), Duration::zero());
}

void test_should_sequence_empty(void) {
  Notes notes(config);
  Sequencer<> seq(config, notes);
  assert_duration_equal(seq.clock(), 0_ms);
  Duration time = Duration::zero();
  for (auto i = 0; i < 100; i++) {
    Duration step = Duration::millis(i);
    auto pulse = seq.sample(step);
    assert_duration_equal(seq.clock(), time + step);
    assert_duration_equal(pulse.start, time);
    assert_duration_equal(pulse.duty, 0_ms);
    assert_duration_equal(pulse.period, step);
    time += step;
  }
}

void test_should_sequence_single(void) {
  Notes notes(config);
  Sequencer<> seq(config, notes);
  assert_duration_equal(seq.clock(), 0_ms);
  notes.start(mnotef(0), 10_ms, instrument, config);

  NotePulse pulse = seq.sample(20_ms);
  assert_duration_equal(seq.clock(), 10_ms);
  assert_duration_equal(pulse.start, 0_ms);
  assert_duration_equal(pulse.duty, 0_ms);
  assert_duration_equal(pulse.period, 10_ms);

  notes.release(mnotef(0), 1_s);

  for (auto i = 1; seq.clock() < 1_s; i++) {
    NotePulse pulse2 = seq.sample(10_ms);
    Duration time = 10_ms * i;
    assert_duration_equal(pulse2.start, time);
    assert_duration_equal(pulse2.duty, 100_us);
    assert_duration_equal(pulse2.period, 100_us + config.min_deadtime);
    assert_duration_equal(seq.clock(), time + pulse2.period);

    NotePulse pulse3 = seq.sample(10_ms);
    assert_duration_equal(pulse3.start, time + 100_us + config.min_deadtime);
    assert_duration_equal(pulse3.duty, 0_us);
    assert_duration_equal(pulse3.period, seq.clock() < 1_s ? 9800_us : 10_ms);
    assert_duration_equal(seq.clock(),
                          time + 10_ms + (seq.clock() < 1_s ? 0_us : 200_us));
  }
  assert_duration_equal(seq.clock(), 1_s + 200_us);
  TEST_ASSERT_EQUAL(0, notes.active());
}

void test_should_sequence_polyphonic(void) {
  Notes notes(config);
  Sequencer<> seq(config, notes);
  assert_duration_equal(seq.clock(), 0_ms);
  notes.start(mnotef(0), 10_ms, instrument, config);
  notes.start(mnotef(12), 10_ms, instrument, config);

  NotePulse pulse = seq.sample(20_ms);
  assert_duration_equal(seq.clock(), 10_ms);
  assert_duration_equal(pulse.start, 0_ms);
  assert_duration_equal(pulse.duty, 0_ms);
  assert_duration_equal(pulse.period, 10_ms);

  NotePulse pulse2 = seq.sample(10_ms);
  assert_duration_equal(pulse2.start, 10_ms);
  assert_duration_equal(pulse2.duty, 100_us);
  assert_duration_equal(pulse2.period, 100_us + config.min_deadtime);
  assert_duration_equal(seq.clock(), 10_ms + pulse2.period);

  NotePulse pulse3 = seq.sample(10_ms);
  assert_duration_equal(pulse3.start, 10_ms + pulse2.period);
  assert_duration_equal(pulse3.duty, 0_us);
  assert_duration_equal(pulse3.period, *(5_ms - pulse2.period));
  assert_duration_equal(seq.clock(), 15_ms);

  NotePulse pulse4 = seq.sample(10_ms);
  assert_duration_equal(pulse4.start, 15_ms);
  assert_duration_equal(pulse4.duty, 100_us);
  assert_duration_equal(pulse4.period, 100_us + config.min_deadtime);
  assert_duration_equal(seq.clock(), 15_ms + pulse4.period);
}

void test_should_sequence_polyphonic_out_of_phase(void) {
  Notes notes(config);
  Sequencer<> seq(config, notes);
  assert_duration_equal(seq.clock(), 0_ms);
  notes.start(mnotef(0), 10_ms, instrument, config);
  notes.start(mnotef(12), 11_ms, instrument, config);

  NotePulse pulse = seq.sample(20_ms);
  assert_duration_equal(seq.clock(), 10_ms);
  assert_duration_equal(pulse.start, 0_ms);
  assert_duration_equal(pulse.duty, 0_ms);
  assert_duration_equal(pulse.period, 10_ms);

  NotePulse pulse2 = seq.sample(10_ms);
  assert_duration_equal(pulse2.start, 10_ms);
  assert_duration_equal(pulse2.duty, 100_us);
  assert_duration_equal(pulse2.period, 100_us + config.min_deadtime);
  assert_duration_equal(seq.clock(), 10_ms + pulse2.period);

  NotePulse pulse3 = seq.sample(10_ms);
  assert_duration_equal(pulse3.start, 10_ms + pulse2.period);
  assert_duration_equal(pulse3.duty, 0_us);
  assert_duration_equal(pulse3.period, *(1_ms - pulse2.period));
  assert_duration_equal(seq.clock(), 11_ms);

  NotePulse pulse4 = seq.sample(10_ms);
  assert_duration_equal(pulse4.start, 11_ms);
  assert_duration_equal(pulse4.duty, 100_us);
  assert_duration_equal(pulse4.period, 100_us + config.min_deadtime);
  assert_duration_equal(seq.clock(), 11_ms + pulse4.period);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_empty);
  RUN_TEST(test_should_sequence_empty);
  RUN_TEST(test_should_sequence_single);
  RUN_TEST(test_should_sequence_polyphonic);
  RUN_TEST(test_should_sequence_polyphonic_out_of_phase);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
