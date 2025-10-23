#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "midi_core.hpp"
#include "midi_synth.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstdint>
#include "notes.hpp"
#include <unity.h>
#include <vector>

constexpr Config config;
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
  FakeNotes notes;
  SynthChannel channel(config, notes);
  TEST_ASSERT_EQUAL(0, channel.instrument_number());
}

void test_should_handle_note_on(void) {
  FakeNotes notes;
  SynthChannel channel(config, notes);
  for (auto i = 0; i < 10; i++) {
    const Duration now = 10_ms * i;
    channel.handle(MidiChannelMessage::note_on(0, 69 + i, 10 * i), now);

    TEST_ASSERT_EQUAL(i + 1, notes.started().size());
    assert_duration_equal(notes.started().back().time, now);
    TEST_ASSERT_EQUAL(69 + i, notes.started().back().mnote.number);
    TEST_ASSERT_EQUAL(10 * i, notes.started().back().mnote.velocity);
    TEST_ASSERT_TRUE(notes.started().back().instrument == default_instrument());
  }
}

void test_should_handle_note_off(void) {
  FakeNotes notes;
  SynthChannel channel(config, notes);
  for (auto i = 0; i < 10; i++) {
    const auto now = 10_ms * i;
    channel.handle(MidiChannelMessage::note_off(0, 69 + i, 10 * i), now);

    TEST_ASSERT_EQUAL(i + 1, notes.released().size());
    assert_duration_equal(notes.released().back().time, now);
    TEST_ASSERT_EQUAL(69 + i, notes.released().back().mnote);
  }
}

void test_should_handle_instrument_change(void) {
  constexpr auto N = 10;
  FakeNotes notes;
  Instrument instruments[N];
  SynthChannel channel(config, notes, instruments, N);
  for (auto i = 0; i < N; i++) {
    instruments[i] = instrument(i);

    channel.handle(MidiChannelMessage::program_change(0, i), 10_ms);
    TEST_ASSERT_EQUAL(i, channel.instrument_number());

    channel.handle(MidiChannelMessage::note_on(0, 69 + i, 10 * i), 0_ms);

    TEST_ASSERT_EQUAL(i + 1, notes.started().size());
    assert_instrument_equal(notes.started().back().instrument, instrument(i));
  }
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_should_handle_note_on);
  RUN_TEST(test_should_handle_note_off);
  RUN_TEST(test_should_handle_instrument_change);

  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
