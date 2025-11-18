#include "core.hpp"
#include "envelope.hpp"
#include "instruments.hpp"
#include "lfo.hpp"
#include "midi_core.hpp"
#include "midi_synth.hpp"
#include "notes.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <cstdint>
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

  struct Off {};

private:
  std::vector<Started> started_;
  std::vector<Released> released_;
  std::vector<Off> offs_;

public:
  Note &start(const MidiNote &mnote, Duration time,
              const Instrument &instrument, const Config &config) {
    started_.push_back({mnote, time, instrument, config});
    return note;
  }
  void release(uint8_t number, Duration time) {
    released_.push_back({number, time});
  }
  void off() { offs_.push_back({}); }

  const std::vector<Started> started() const { return started_; }
  const std::vector<Released> released() const { return released_; }
  const std::vector<Off> turned_off() const { return offs_; }
};

void test_empty(void) {
  FakeNotes notes;
  TrackState track;
  SynthChannel channel(config, notes, track);
  TEST_ASSERT_EQUAL(0, channel.instrument_number());
  TEST_ASSERT_FALSE(track.is_playing());
}

void test_should_handle_note_on(void) {
  FakeNotes notes;
  TrackState track;
  SynthChannel channel(config, notes, track);
  for (auto i = 0; i < 10; i++) {
    const Duration now = 10_ms * i;
    channel.handle(MidiChannelMessage::note_on(0, 69 + i, 10 * i), now);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, notes.started().size());
    assert_duration_equal(notes.started().back().time, now);
    TEST_ASSERT_EQUAL(69 + i, notes.started().back().mnote.number);
    TEST_ASSERT_EQUAL(10 * i, notes.started().back().mnote.velocity);
    TEST_ASSERT_TRUE(notes.started().back().instrument == default_instrument());
  }
}

void test_should_handle_note_off(void) {
  FakeNotes notes;
  TrackState track;
  SynthChannel channel(config, notes, track);
  track.on_receive(Duration::zero());
  for (auto i = 0; i < 10; i++) {
    const auto now = 10_ms * i;
    channel.handle(MidiChannelMessage::note_off(0, 69 + i, 10 * i), now);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, notes.released().size());
    assert_duration_equal(notes.released().back().time, now);
    TEST_ASSERT_EQUAL(69 + i, notes.released().back().mnote);
  }
}

void test_should_ignore_note_off_when_not_playing(void) {
  FakeNotes notes;
  TrackState track;
  SynthChannel channel(config, notes, track);
  for (auto i = 0; i < 10; i++) {
    const auto now = 10_ms * i;
    channel.handle(MidiChannelMessage::note_off(0, 69 + i, 10 * i), now);

    TEST_ASSERT_FALSE(track.is_playing());
    TEST_ASSERT_EQUAL(0, notes.released().size());
  }
}

void test_should_handle_instrument_change(void) {
  constexpr auto N = 10;
  FakeNotes notes;
  Instrument instruments[N];
  for (auto i = 0; i < N; i++) {
    instruments[i] = instrument(i);
    TrackState track;
    SynthChannel channel(config, notes, track, instruments, N);

    channel.handle(MidiChannelMessage::program_change(0, i), 10_ms);
    TEST_ASSERT_EQUAL(i, channel.instrument_number());
    TEST_ASSERT_FALSE(track.is_playing());

    channel.handle(MidiChannelMessage::note_on(0, 69 + i, 10 * i), 0_ms);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, notes.started().size());
    assert_instrument_equal(notes.started().back().instrument, instrument(i));
  }
}

void test_should_ignore_instrument_change_when_config_has_instrument(void) {
  constexpr auto N = 10;
  FakeNotes notes;
  Instrument instruments[N];
  for (auto i = 0; i < N; i++) {
    instruments[i] = instrument(i);
  }

  TrackState track;
  Config config{.instrument = 2};
  SynthChannel channel(config, notes, track, instruments, N);

  for (auto i = 0; i < N; i++) {
    channel.handle(MidiChannelMessage::program_change(0, i), 10_ms);
    TEST_ASSERT_EQUAL(2, channel.instrument_number());
    channel.handle(MidiChannelMessage::note_on(0, 69 + i, 10 * i), 0_ms);

    TEST_ASSERT_TRUE(track.is_playing());
    TEST_ASSERT_EQUAL(i + 1, notes.started().size());
    assert_instrument_equal(notes.started().back().instrument, instrument(2));
  }
}

void test_should_turnoff_when_needed(void) {
  const std::vector<ControlChange> cc_event_types{
      ControlChange::ALL_SOUND_OFF,
      ControlChange::ALL_NOTES_OFF,
      ControlChange::RESET_ALL_CONTROLLERS,
  };
  for (auto cc : cc_event_types) {
    for (auto ch = 0; ch < 16; ch++) {
      FakeNotes notes;
      TrackState track;
      SynthChannel channel(config, notes, track);
      TEST_ASSERT_EQUAL(0, notes.turned_off().size());
      channel.handle(MidiChannelMessage::control_change(ch, cc, 0), 10_ms);
      TEST_ASSERT_EQUAL(1, notes.turned_off().size());
      TEST_ASSERT_FALSE(track.is_playing());
    }
  }
}

void test_should_start_playing_the_first_note_on_message(void) {
  FakeNotes notes;
  TrackState track;
  SynthChannel channel(config, notes, track);
  TEST_ASSERT_FALSE(track.is_playing());
  channel.handle(MidiChannelMessage::note_on(0, 69, 127), 100_s);

  TEST_ASSERT_TRUE(track.is_playing());
  TEST_ASSERT_EQUAL(1, notes.started().size());
  assert_duration_equal(notes.started().back().time, Duration::zero());
  TEST_ASSERT_EQUAL(69, notes.started().back().mnote.number);
  TEST_ASSERT_EQUAL(127, notes.started().back().mnote.velocity);
  TEST_ASSERT_TRUE(notes.started().back().instrument == default_instrument());
}

void test_should_ignore_off_messages_when_not_playing(void) {
  FakeNotes notes;
  TrackState track;
  SynthChannel channel(config, notes, track);
  TEST_ASSERT_FALSE(track.is_playing());
  channel.handle(MidiChannelMessage::note_off(0, 69, 127), 100_s);
  TEST_ASSERT_FALSE(track.is_playing());
  TEST_ASSERT_EQUAL(0, notes.started().size());
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_should_handle_note_on);
  RUN_TEST(test_should_handle_note_off);
  RUN_TEST(test_should_ignore_note_off_when_not_playing);
  RUN_TEST(test_should_handle_instrument_change);
  RUN_TEST(test_should_ignore_instrument_change_when_config_has_instrument);
  RUN_TEST(test_should_turnoff_when_needed);
  RUN_TEST(test_should_start_playing_the_first_note_on_message);
  RUN_TEST(test_should_ignore_off_messages_when_not_playing);

  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
