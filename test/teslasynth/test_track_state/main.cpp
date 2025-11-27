#include "core.hpp"
#include "midi_synth.hpp"
#include "synthesizer/helpers/assertions.hpp"
#include <unity.h>

using namespace teslasynth::midisynth;

void test_empty(void) {
  TrackState track;
  TEST_ASSERT_FALSE(track.is_playing());
  assert_duration_equal(track.received_time(), Duration::zero());
  assert_duration_equal(track.started_time(), Duration::zero());
}

void test_tick(void) {
  TrackState track;
  assert_duration_equal(track.on_receive(1_ms), Duration::zero());
  TEST_ASSERT_TRUE(track.is_playing());
  assert_duration_equal(track.started_time(), 1_ms);
  assert_duration_equal(track.received_time(), Duration::zero());

  assert_duration_equal(track.on_receive(3_ms), 2_ms);
  TEST_ASSERT_TRUE(track.is_playing());
  assert_duration_equal(track.started_time(), 1_ms);
  assert_duration_equal(track.received_time(), 2_ms);

  assert_duration_equal(track.on_receive(500_us), Duration::zero());
  TEST_ASSERT_TRUE(track.is_playing());
  assert_duration_equal(track.started_time(), 1_ms);
  assert_duration_equal(track.received_time(), 2_ms);
}

void test_stop(void) {
  TrackState track;
  assert_duration_equal(track.on_receive(1_ms), Duration::zero());
  TEST_ASSERT_TRUE(track.is_playing());
  assert_duration_equal(track.on_play(1_ms), 1_ms);
  TEST_ASSERT_TRUE(track.is_playing());
  track.stop();
  TEST_ASSERT_FALSE(track.is_playing());
  assert_duration_equal(track.received_time(), Duration::zero());
  assert_duration_equal(track.started_time(), Duration::zero());
  assert_duration_equal(track.played_time(), Duration::zero());
}

void test_playback(void) {
  TrackState track;
  assert_duration_equal(track.on_play(10_ms), Duration::zero());

  assert_duration_equal(track.on_receive(10_ms), Duration::zero());
  assert_duration_equal(track.on_receive(100_ms), 90_ms);
  assert_duration_equal(track.on_play(10_ms), 10_ms);

  TEST_ASSERT_TRUE(track.is_playing());
  assert_duration_equal(track.started_time(), 10_ms);
  assert_duration_equal(track.received_time(), 90_ms);
  assert_duration_equal(track.played_time(), 10_ms);

  assert_duration_equal(track.on_play(5_ms), 5_ms);
  TEST_ASSERT_TRUE(track.is_playing());
  assert_duration_equal(track.started_time(), 10_ms);
  assert_duration_equal(track.received_time(), 90_ms);
  assert_duration_equal(track.played_time(), 15_ms);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_empty);
  RUN_TEST(test_tick);
  RUN_TEST(test_stop);
  RUN_TEST(test_playback);
  UNITY_END();
}
int main(int argc, char **argv) { app_main(); }
