#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "midi_core.hpp"
#include "midi_synth.hpp"
#include "notes.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include "unity_internals.h"
#include <cstdint>
#include <unity.h>
#include <vector>

using namespace teslasynth::midisynth;

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
    Hertz tuning;
  };

  struct Released {
    uint8_t mnote;
    Duration time;
  };

  struct Off {};

private:
  std::vector<Started> started_;
  std::vector<Released> released_;
  std::vector<Off> offs_;
  std::vector<uint8_t> adjusts_;

public:
  Note &start(const MidiNote &mnote, Duration time,
              const Instrument &instrument, Hertz tuning) {
    started_.push_back({mnote, time, instrument, tuning});
    return note;
  }
  void release(uint8_t number, Duration time) {
    released_.push_back({number, time});
  }
  void off() { offs_.push_back({}); }

  void adjust_size(uint8_t size) { adjusts_.push_back(size); }

  const std::vector<Started> started() const { return started_; }
  const std::vector<Released> released() const { return released_; }
  const std::vector<Off> turned_off() const { return offs_; }
  const std::vector<uint8_t> adjusted() const { return adjusts_; }
};

void test_note_pulse_empty(void) {
  Teslasynth<> tsynth;
  TEST_ASSERT_EQUAL(0, tsynth.instrument_number(0));
  TEST_ASSERT_FALSE(tsynth.track().is_playing());
}

void test_boundaries(void) {
  Teslasynth<> tsynth;
  tsynth.note_on(10, 70, 127, Duration::zero());
  tsynth.note_off(10, 70, Duration::zero());
  tsynth.change_instrument(10, 100);
}

void test_should_handle_note_on(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &track = tsynth.track();
  auto &notes = tsynth.voice();
  for (auto i = 0; i < 10; i++) {
    const Duration now = 10_ms * i;
    tsynth.handle(MidiChannelMessage::note_on(0, 69 + i, 10 * i), now);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, notes.started().size());
    assert_duration_equal(notes.started().back().time, now);
    TEST_ASSERT_EQUAL(69 + i, notes.started().back().mnote.number);
    TEST_ASSERT_EQUAL(10 * i, notes.started().back().mnote.velocity);
    TEST_ASSERT_TRUE(notes.started().back().instrument == default_instrument());
  }
}

void test_should_handle_note_off(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();
  for (auto i = 0; i < 10; i++) {
    const auto now = 10_ms * i;
    tsynth.handle(MidiChannelMessage::note_on(0, 69 + i, 10 * i), now);
    tsynth.handle(MidiChannelMessage::note_off(0, 69 + i, 10 * i), now);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, voice.released().size());
    assert_duration_equal(voice.released().back().time, now);
    TEST_ASSERT_EQUAL(69 + i, voice.released().back().mnote);
  }
}

void test_should_ignore_note_off_when_not_playing(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();
  for (auto i = 0; i < 10; i++) {
    const auto now = 10_ms * i;
    tsynth.handle(MidiChannelMessage::note_off(0, 69 + i, 10 * i), now);

    TEST_ASSERT_FALSE(track.is_playing());
    TEST_ASSERT_EQUAL(0, voice.released().size());
  }
}

void test_should_handle_instrument_change(void) {
  constexpr auto N = 10;
  std::array<Instrument, N> instruments;
  for (auto i = 0; i < N; i++) {
    instruments[i] = instrument(i);
  }
  Teslasynth<1, FakeNotes> tsynth;
  tsynth.use_instruments(instruments);
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();

  tsynth.handle(MidiChannelMessage::program_change(0, 0), 10_ms);
  TEST_ASSERT_EQUAL(0, tsynth.instrument_number(0));
  TEST_ASSERT_FALSE(track.is_playing());

  for (auto i = 0; i < N; i++) {
    tsynth.handle(MidiChannelMessage::program_change(0, i), 10_ms);
    TEST_ASSERT_EQUAL(i, tsynth.instrument_number(0));
    tsynth.handle(MidiChannelMessage::note_on(0, 69 + i, 10 * i), 0_ms);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, voice.started().size());
    assert_instrument_equal(voice.started().back().instrument, instrument(i));
  }
}

void test_should_ignore_instrument_change_when_config_has_instrument(void) {
  constexpr auto N = 10;
  std::array<Instrument, N> instruments;
  for (auto i = 0; i < N; i++) {
    instruments[i] = instrument(i);
  }

  Configuration<> config(SynthConfig{.instrument = 2});
  Teslasynth<1, FakeNotes> tsynth(config);
  tsynth.use_instruments(instruments);
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();

  for (auto i = 0; i < N; i++) {
    tsynth.handle(MidiChannelMessage::program_change(0, i), 10_ms);
    TEST_ASSERT_EQUAL(2, tsynth.instrument_number(0));
    tsynth.handle(MidiChannelMessage::note_on(0, 69 + i, 10 * i), 0_ms);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, voice.started().size());
    assert_instrument_equal(voice.started().back().instrument, instrument(2));
  }
}

void test_config_instrument_overrides_runtime_instrument(void) {
  Teslasynth<1, FakeNotes> tsynth;
  TEST_ASSERT_EQUAL(0, tsynth.instrument_number(0));

  tsynth.configuration().synth().instrument = 2;
  TEST_ASSERT_EQUAL(2, tsynth.instrument_number(0));

  tsynth.configuration().channel(0).instrument = 3;
  TEST_ASSERT_EQUAL(3, tsynth.instrument_number(0));

  tsynth.configuration().synth().instrument = 4;
  TEST_ASSERT_EQUAL(3, tsynth.instrument_number(0));
}

void test_non_existing_instrument_number_falls_back_to_default(void) {
  Teslasynth<1, FakeNotes> tsynth;
  TEST_ASSERT_EQUAL(0, tsynth.instrument_number(0));
  tsynth.configuration().synth().instrument = 200;
  assert_instrument_equal(tsynth.instrument(0), default_instrument());
}

void test_should_turnoff_when_needed(void) {
  const std::vector<ControlChange> cc_event_types{
      ControlChange::ALL_SOUND_OFF,
      ControlChange::ALL_NOTES_OFF,
      ControlChange::RESET_ALL_CONTROLLERS,
  };
  for (auto cc : cc_event_types) {
    for (auto ch = 0; ch < 16; ch++) {
      Teslasynth<1, FakeNotes> tsynth;
      auto &track = tsynth.track();
      auto &voice = tsynth.voice();
      TEST_ASSERT_EQUAL(0, voice.turned_off().size());
      tsynth.handle(MidiChannelMessage::control_change(ch, cc, 0), 10_ms);
      TEST_ASSERT_EQUAL(1, voice.turned_off().size());
      TEST_ASSERT_FALSE(track.is_playing());
    }
  }
}

void test_should_start_playing_the_first_note_on_message(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();
  TEST_ASSERT_FALSE(track.is_playing());
  tsynth.handle(MidiChannelMessage::note_on(0, 69, 127), 100_s);

  TEST_ASSERT_TRUE(track.is_playing());
  TEST_ASSERT_EQUAL(1, voice.started().size());
  assert_duration_equal(voice.started().back().time, Duration::zero());
  TEST_ASSERT_EQUAL(69, voice.started().back().mnote.number);
  TEST_ASSERT_EQUAL(127, voice.started().back().mnote.velocity);
  TEST_ASSERT_TRUE(voice.started().back().instrument == default_instrument());
}

void test_should_ignore_off_messages_when_not_playing(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &track = tsynth.track();
  auto &voice = tsynth.voice();
  TEST_ASSERT_FALSE(track.is_playing());
  tsynth.handle(MidiChannelMessage::note_off(0, 69, 127), 100_s);
  TEST_ASSERT_FALSE(track.is_playing());
  TEST_ASSERT_EQUAL(0, voice.started().size());
}

void test_should_adjust_note_sizes(void) {
  std::array<Config, 1> configs = {Config{.notes = 2}};
  Configuration<> conf({}, configs);
  Teslasynth<1, FakeNotes> tsynth(conf);
  auto &voice = tsynth.voice();

  TEST_ASSERT_EQUAL(1, voice.adjusted().size());
  TEST_ASSERT_EQUAL(2, voice.adjusted().back());
}

void test_reload_config_should_adjust_note_sizes(void) {
  Teslasynth<1, FakeNotes> tsynth;
  auto &voice = tsynth.voice();
  tsynth.configuration().channel(0).notes = 2;
  tsynth.reload_config();

  TEST_ASSERT_EQUAL(2, voice.adjusted().size());
  TEST_ASSERT_EQUAL(2, voice.adjusted().back());
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_note_pulse_empty);
  RUN_TEST(test_boundaries);
  RUN_TEST(test_should_handle_note_on);
  RUN_TEST(test_should_handle_note_off);
  RUN_TEST(test_should_ignore_note_off_when_not_playing);
  RUN_TEST(test_should_handle_instrument_change);
  RUN_TEST(test_should_ignore_instrument_change_when_config_has_instrument);
  RUN_TEST(test_config_instrument_overrides_runtime_instrument);
  RUN_TEST(test_non_existing_instrument_number_falls_back_to_default);
  RUN_TEST(test_should_turnoff_when_needed);
  RUN_TEST(test_should_start_playing_the_first_note_on_message);
  RUN_TEST(test_should_ignore_off_messages_when_not_playing);
  RUN_TEST(test_should_adjust_note_sizes);
  RUN_TEST(test_reload_config_should_adjust_note_sizes);

  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
