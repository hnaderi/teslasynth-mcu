#include "midi_core.hpp"
#include <iostream>
#include <ostream>
#include <unity.h>

using namespace teslasynth::midi;

void print_sizes_midi(void) {
  std::cout << "MidiByte: " << sizeof(MidiData) << std::endl
            << "MidiChannelMessage: " << sizeof(MidiChannelMessage)
            << std::endl;
}

void midi_data(void) {
  TEST_ASSERT_EQUAL(0, MidiData(0));
  TEST_ASSERT_EQUAL(10, MidiData(10));
  TEST_ASSERT_EQUAL(100, MidiData(100));
  TEST_ASSERT_EQUAL(127, MidiData(127));

  TEST_ASSERT_EQUAL(0, MidiData(128));
  TEST_ASSERT_EQUAL(2, MidiData(130));
}

void midi_channel_number(void) {
  TEST_ASSERT_EQUAL(0, MidiChannelNumber(0));
  TEST_ASSERT_EQUAL(1, MidiChannelNumber(1));
  TEST_ASSERT_EQUAL(10, MidiChannelNumber(10));
  TEST_ASSERT_EQUAL(15, MidiChannelNumber(15));

  TEST_ASSERT_EQUAL(0, MidiChannelNumber(16));
  TEST_ASSERT_EQUAL(1, MidiChannelNumber(17));
}

void midi_status(void) {
  for (int i = 0; i < 0x80; i++)
    TEST_ASSERT_FALSE(MidiStatus::is_status(i));
  for (int i = 0x80; i < 0xFF; i++)
    TEST_ASSERT_TRUE(MidiStatus::is_status(i));

  TEST_ASSERT_EQUAL(MidiMessageType::NoteOff, MidiStatus(0));
  TEST_ASSERT_EQUAL(MidiMessageType::NoteOff, MidiStatus(0x80));
  TEST_ASSERT_EQUAL(MidiMessageType::NoteOn, MidiStatus(0x90));
  TEST_ASSERT_EQUAL(0xF0, MidiStatus(0xF0));
  TEST_ASSERT_EQUAL(0xFF, MidiStatus(0xFF));

  TEST_ASSERT_EQUAL(0x80, MidiStatus::min());
}

void midi_status_channel(void) {
  for (int t = 0; t < 7; t++) {
    for (int i = 0; i < 16; i++) {
      auto status = MidiStatus(t << 4 | i);
      TEST_ASSERT_TRUE(status.is_channel());
      TEST_ASSERT_FALSE(status.is_system());
      TEST_ASSERT_EQUAL(t << 4 | 0x80, status.channel_status_type());
      TEST_ASSERT_EQUAL(i, status.channel());
    }
  }
  for (int i = 0; i < 16; i++) {
    auto status = MidiStatus(0xF0 | i);
    TEST_ASSERT_FALSE(status.is_channel());
  }
}

void midi_status_system(void) {
  for (int i = 0; i < 16; i++) {
    auto status = MidiStatus(0xF0 | i);
    TEST_ASSERT_FALSE(status.is_channel());
    TEST_ASSERT_TRUE(status.is_system());
  }
}

void midi_status_system_realtime(void) {
  for (int i = 0; i < 8; i++) {
    auto status = MidiStatus(0xF8 | i);
    TEST_ASSERT_FALSE(status.is_channel());
    TEST_ASSERT_TRUE(status.is_system());
    TEST_ASSERT_TRUE(status.is_system_realtime());
  }
}

void midi_channel_mode_control(void) {
  for (int i = 0; i < 16; i++) {
    for (int c = 120; c < 128; c++) {
      MidiChannelMessage msg{
          .type = MidiMessageType::ControlChange,
          .channel = i,
          .data0 = c,
          .data1 = 0,
      };
      TEST_ASSERT_TRUE(msg.is_control());
      TEST_ASSERT_TRUE(msg.is_channel_mode_control());
    }
  }
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(print_sizes_midi);
  RUN_TEST(midi_data);
  RUN_TEST(midi_channel_number);
  RUN_TEST(midi_status);
  RUN_TEST(midi_status_channel);
  RUN_TEST(midi_status_system);
  RUN_TEST(midi_status_system_realtime);
  RUN_TEST(midi_channel_mode_control);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
