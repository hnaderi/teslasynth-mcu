#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <synth.hpp>
#include <unity.h>
#include <vector>

constexpr Config config(size_t notes) {
  return {.a440 = 100_hz, .notes = std::max<size_t>(1, notes)};
}
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

void test_should_handle_note_on(void) {
  FakeNotes notes;
  SynthChannel channel(config(1), notes);
  for (auto i = 0; i < 10; i++) {
    channel.on_note_on(mnotef(i), 100_ms * i);

    TEST_ASSERT_EQUAL(i + 1, notes.started().size());
    assert_duration_equal(notes.started().back().time, 100_ms * i);
    TEST_ASSERT_TRUE(notes.started().back().mnote == mnotef(i));
    TEST_ASSERT_TRUE(notes.started().back().instrument == default_instrument());
  }
}

void test_should_handle_note_off(void) {
  FakeNotes notes;
  SynthChannel channel(config(1), notes);
  for (auto i = 0; i < 10; i++) {
    channel.on_note_off(mnotef(i), 100_ms * i);

    TEST_ASSERT_EQUAL(i + 1, notes.released().size());
    assert_duration_equal(notes.released().back().time, 100_ms * i);
    TEST_ASSERT_TRUE(notes.released().back().mnote == mnotef(i).number);
  }
}

void test_should_handle_note_on_with_velocity_zero(void) {
  FakeNotes notes;
  SynthChannel channel(config(1), notes);
  for (auto i = 0; i < 10; i++) {
    channel.on_note_on({static_cast<uint8_t>(i + 60), 0}, 100_ms * i);

    TEST_ASSERT_EQUAL(0, notes.started().size());
    TEST_ASSERT_EQUAL(i + 1, notes.released().size());
    assert_duration_equal(notes.released().back().time, 100_ms * i);
    TEST_ASSERT_EQUAL(notes.released().back().mnote, i + 60);
  }
}

void test_should_handle_instrument_change(void) {
  constexpr auto N = 10;
  FakeNotes notes;
  Instrument instruments[N];
  SynthChannel channel(config(1), notes, instruments, N);
  for (auto i = 0; i < N; i++) {
    instruments[i] = instrument(i);

    channel.on_program_change(i);
    channel.on_note_on(mnotef(i), 0_ms);

    TEST_ASSERT_EQUAL(i + 1, notes.started().size());
    assert_instrument_equal(notes.started().back().instrument, instrument(i));
  }
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_should_handle_note_on);
  RUN_TEST(test_should_handle_note_off);
  RUN_TEST(test_should_handle_note_on_with_velocity_zero);

  RUN_TEST(test_should_handle_instrument_change);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
