#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "midi_core.hpp"
#include "midi_synth.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstdint>
#include <synth.hpp>
#include <unity.h>
#include <vector>

constexpr Config config{.a440 = 100_hz};
constexpr MidiNote mnotef(int i) { return {static_cast<uint8_t>(69 + i), 127}; }
constexpr Instrument instrument(int i) {
  return {.envelope = ADSR::constant(EnvelopeLevel(i * 0.1)),
          .vibrato = Vibrato::none()};
}

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
    assert_duration_equal(pulse.start, 0_ms);
    assert_duration_equal(pulse.duty, 0_ms);
    assert_duration_equal(pulse.period, step);
    time += step;
  }
}

void test_should_sequence_single(void) {
  Notes notes(config);
  Sequencer<> seq(config, notes);
  assert_duration_equal(seq.clock(), 0_ms);
  notes.start(mnotef(0), 10_ms, instrument(0), config);
  // notes.release(mnotef(0), 5_s);

  NotePulse pulse = seq.sample(20_ms);
  assert_duration_equal(seq.clock(), 10_ms);
  assert_duration_equal(pulse.start, 0_ms);
  assert_duration_equal(pulse.duty, 0_ms);
  assert_duration_equal(pulse.period, 10_ms);

  NotePulse pulse2 = seq.sample(10_ms);
  assert_duration_equal(seq.clock(), 20_ms);
  assert_duration_equal(pulse2.start, 10_ms);
  assert_duration_equal(pulse2.duty, 10_ms + 100_us);
  assert_duration_equal(pulse2.period, 10_ms + 100_us + config.min_deadtime);
}

// void test_should_sequence_single(void) {
//   SynthChannel channel(config(1));
//   channel.on_note_on(mnotef(0), 100_ms);
//   assert_duration_equal(channel.clock(), 0_ms);
//   for (uint8_t i = 0; i < 10; i++) {
//     auto pulse = channel.tick();
//     Duration start = 100_ms + (10_ms * i);
//     assert_duration_equal(pulse.start, start);
//     assert_duration_equal(pulse.off, start + 100_us);
//     assert_duration_equal(channel.clock(), start + 100_us);
//     assert_duration_equal(pulse.end, start + 10_ms);
//   }
// }

// void test_should_sequence_polyphonic(void) {
//   SynthChannel channel(config(2));
//   channel.on_note_on(mnotef(0), 100_ms);
//   channel.on_note_on(mnotef(12), 100_ms);

//   assert_duration_equal(channel.clock(), 0_ms);
//   for (uint8_t i = 0; i < 10; i++) {
//     auto pulse = channel.tick();
//     Duration start = 100_ms + (5_ms * i);
//     assert_duration_equal(pulse.start, start);
//     assert_duration_equal(pulse.off, start + 100_us);
//     assert_duration_equal(channel.clock(), start + 100_us);
//     assert_duration_equal(pulse.end, start + 5_ms);
//   }
// }

extern "C" void app_main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_empty);
  // RUN_TEST(test_should_sequence_empty);
  // RUN_TEST(test_should_sequence_single);
  // RUN_TEST(test_should_sequence_polyphonic);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
