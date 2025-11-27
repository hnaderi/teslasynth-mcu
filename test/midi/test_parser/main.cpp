#include "midi_core.hpp"
#include "midi_parser.hpp"
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <unity.h>
#include <vector>

using namespace teslasynth::midi;

using Messages = std::vector<MidiChannelMessage>;

template <typename T> inline std::string __msg_for(T a, T b) {
  return std::string("Obtained: " + std::string(a) +
                     " Expected: " + std::string(b));
}

inline void __assert_midi_message_equal(MidiChannelMessage a,
                                        MidiChannelMessage b, int line) {
  UNITY_TEST_ASSERT(a == b, line, __msg_for(a, b).c_str());
}
inline void __assert_midi_message_not_equal(MidiChannelMessage a,
                                            MidiChannelMessage b, int line) {
  UNITY_TEST_ASSERT(a != b, line, __msg_for(a, b).c_str());
}

#define assert_midi_message_equal(a, b)                                        \
  __assert_midi_message_equal(a, b, __LINE__);

namespace message_types {
namespace channel {
std::vector<MidiMessageType> two{
    MidiMessageType::NoteOff,        MidiMessageType::NoteOn,
    MidiMessageType::AfterTouchPoly, MidiMessageType::ControlChange,
    MidiMessageType::PitchBend,
};
std::vector<MidiMessageType> one{
    MidiMessageType::ProgramChange,
    MidiMessageType::AfterTouchChannel,
};
std::vector<MidiMessageType> all{
    MidiMessageType::NoteOff,        MidiMessageType::NoteOn,
    MidiMessageType::AfterTouchPoly, MidiMessageType::ControlChange,
    MidiMessageType::ProgramChange,  MidiMessageType::AfterTouchChannel,
    MidiMessageType::PitchBend,
};
} // namespace channel
} // namespace message_types

struct Random {
  std::mt19937 gen;
  std::uniform_int_distribution<> value_distr{0, 127};
  std::uniform_int_distribution<> ch_distr{0, 15};

  uint8_t channel() { return ch_distr(gen); }
  uint8_t value() { return value_distr(gen); }

  void fill_data(std::vector<uint8_t> &input, size_t size) {
    for (size_t i = 0; i < size; i++) {
      uint8_t value0 = value();
      input.push_back(value0);
    }
  }

  size_t fill_data_for_type(std::vector<uint8_t> &input, MidiMessageType type,
                            size_t size) {
    MidiStatus status(type, channel());
    input.push_back(status);
    auto coeff = (type == MidiMessageType::AfterTouchChannel ||
                  type == MidiMessageType::ProgramChange)
                     ? 1
                     : 2;

    fill_data(input, coeff * size);
    return size;
  }

  size_t fill_data_for_all_types(std::vector<uint8_t> &input) {
    size_t total = 0;
    for (auto type : message_types::channel::all) {
      size_t size = value();
      total += fill_data_for_type(input, type, size);
    }
    return total;
  }
} rng;

void parser_empty(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  TEST_ASSERT_FALSE(parser.has_status());
  TEST_ASSERT_EQUAL(0, msgs.size());
}

void parser_feed_note_on(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  auto status = MidiStatus(MidiMessageType::NoteOn, 0);
  const uint8_t data[] = {status, 69, 127};
  parser.feed(data, 3);
  TEST_ASSERT_TRUE(parser.has_status());
  TEST_ASSERT_EQUAL(status, parser.status());
  TEST_ASSERT_EQUAL(1, msgs.size());
  assert_midi_message_equal(msgs.back(),
                            MidiChannelMessage::note_on(0, 69, 127));
}

void parser_feed_note_off(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  auto status = MidiStatus(MidiMessageType::NoteOff, 5);
  const uint8_t data[] = {status, 80, 100};
  parser.feed(data, 3);
  TEST_ASSERT_TRUE(parser.has_status());
  TEST_ASSERT_EQUAL(status, parser.status());
  TEST_ASSERT_EQUAL(1, msgs.size());
  assert_midi_message_equal(msgs.back(),
                            MidiChannelMessage::note_off(5, 80, 100));
}

void parser_feed_program_change(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  auto status = MidiStatus(MidiMessageType::ProgramChange, 5);
  const uint8_t data[] = {status, 20};
  parser.feed(data, 2);
  TEST_ASSERT_TRUE(parser.has_status());
  TEST_ASSERT_EQUAL(status, parser.status());
  TEST_ASSERT_EQUAL(1, msgs.size());
  assert_midi_message_equal(msgs.back(),
                            MidiChannelMessage::program_change(5, 20));
}

void parser_feed_after_touch(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  auto status = MidiStatus(MidiMessageType::AfterTouchPoly, 10);
  const uint8_t data[] = {status, 70, 100};
  parser.feed(data, 3);
  TEST_ASSERT_TRUE(parser.has_status());
  TEST_ASSERT_EQUAL(status, parser.status());
  TEST_ASSERT_EQUAL(1, msgs.size());
  assert_midi_message_equal(msgs.back(),
                            MidiChannelMessage::after_touch(10, 70, 100));
}

void parser_feed_after_touch_channel(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  auto status = MidiStatus(MidiMessageType::AfterTouchChannel, 11);
  const uint8_t data[] = {status, 90};
  parser.feed(data, 2);
  TEST_ASSERT_TRUE(parser.has_status());
  TEST_ASSERT_EQUAL(status, parser.status());
  TEST_ASSERT_EQUAL(1, msgs.size());
  assert_midi_message_equal(msgs.back(),
                            MidiChannelMessage::after_touch_channel(11, 90));
}

void parser_feed_bytes_with_status(void) {
  for (auto type : message_types::channel::two) {
    Messages msgs;
    MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
    uint8_t channel = rng.channel();
    uint8_t value0 = rng.value(), value1 = rng.value();
    MidiStatus status(type, channel);
    const uint8_t data[] = {status, value0, value1};

    parser.feed(data, 3);

    TEST_ASSERT_TRUE(parser.has_status());
    TEST_ASSERT_EQUAL(status, parser.status());
    TEST_ASSERT_EQUAL(1, msgs.size());
    MidiChannelMessage expected{
        .type = type,
        .channel = channel,
        .data0 = value0,
        .data1 = value1,
    };
    assert_midi_message_equal(msgs.back(), expected);
  }
}

void parser_feed_bytes_with_status_single_value(void) {
  for (auto type : message_types::channel::one) {
    Messages msgs;
    MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
    uint8_t channel = rng.channel();
    uint8_t value0 = rng.value();
    MidiStatus status(type, channel);
    const uint8_t data[] = {status, value0};

    parser.feed(data, 2);

    TEST_ASSERT_TRUE(parser.has_status());
    TEST_ASSERT_EQUAL(status, parser.status());
    TEST_ASSERT_EQUAL(1, msgs.size());
    MidiChannelMessage expected{
        .type = type,
        .channel = channel,
        .data0 = value0,
        .data1 = 0,
    };
    assert_midi_message_equal(msgs.back(), expected);
  }
}

void parser_feed_bytes_with_running_status(void) {
  for (auto type : message_types::channel::two) {
    Messages msgs;
    MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
    uint8_t channel = rng.channel();
    MidiStatus status(type, channel);

    size_t size = rng.value();
    std::vector<uint8_t> input{status};
    rng.fill_data(input, 2 * size);

    parser.feed(input.data(), input.size());

    TEST_ASSERT_TRUE(parser.has_status());
    TEST_ASSERT_EQUAL(status, parser.status());
    TEST_ASSERT_EQUAL(size, msgs.size());
    for (size_t i = 0; i < size; i++) {
      MidiChannelMessage expected{
          .type = type,
          .channel = channel,
          .data0 = input[2 * i + 1],
          .data1 = input[2 * i + 2],
      };
      assert_midi_message_equal(msgs[i], expected);
    }
  }
}

void parser_feed_bytes_with_running_status_single_value(void) {
  for (auto type : message_types::channel::one) {
    Messages msgs;
    MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
    uint8_t channel = rng.channel();
    MidiStatus status(type, channel);

    size_t size = rng.value();
    std::vector<uint8_t> input{status};
    rng.fill_data(input, size);

    parser.feed(input.data(), input.size());

    TEST_ASSERT_TRUE(parser.has_status());
    TEST_ASSERT_EQUAL(status, parser.status());
    TEST_ASSERT_EQUAL(size, msgs.size());
    for (size_t i = 0; i < size; i++) {
      MidiChannelMessage expected{
          .type = type,
          .channel = channel,
          .data0 = input[i + 1],
          .data1 = 0,
      };
      assert_midi_message_equal(msgs[i], expected);
    }
  }
}

void parser_ignores_data_with_no_status(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  TEST_ASSERT_FALSE(parser.has_status());
  TEST_ASSERT_EQUAL(0, msgs.size());

  size_t size = rng.value();
  std::vector<uint8_t> input;
  rng.fill_data(input, size);
  parser.feed(input.data(), input.size());

  TEST_ASSERT_FALSE(parser.has_status());
  TEST_ASSERT_EQUAL(0, msgs.size());
}

void parser_ignores_status_with_no_data(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  TEST_ASSERT_FALSE(parser.has_status());
  TEST_ASSERT_EQUAL(0, msgs.size());

  std::vector<uint8_t> input;
  for (auto type : message_types::channel::all)
    for (size_t i = 0; i < rng.value(); i++)
      input.push_back(MidiStatus(type, rng.channel()));

  parser.feed(input.data(), input.size());

  TEST_ASSERT_TRUE(parser.has_status());
  TEST_ASSERT_EQUAL(0, msgs.size());
}

void parser_feed_bytes_with_status_many(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });
  std::vector<uint8_t> input;

  auto total = rng.fill_data_for_all_types(input);

  parser.feed(input.data(), input.size());

  TEST_ASSERT_TRUE(parser.has_status());
  TEST_ASSERT_EQUAL(total, msgs.size());
  TEST_ASSERT_NOT_EQUAL(0, total);
}

void parser_clears_status_on_non_realtime_system_messages(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });

  for (auto type : message_types::channel::all) {
    MidiStatus status(type, rng.channel());
    uint8_t input[]{status};
    parser.feed(input, 1);
    TEST_ASSERT_TRUE(parser.has_status());
    TEST_ASSERT_TRUE(status == parser.status());
    for (size_t i = 0; i < 8; i++) {
      input[0] = MidiStatus(0xF0 | i);
      parser.feed(input, 1);
      TEST_ASSERT_FALSE(parser.has_status());
    }
  }

  TEST_ASSERT_EQUAL(0, msgs.size());
}

void parser_does_not_clear_status_on_realtime_system_messages(void) {
  Messages msgs;
  MidiParser parser([&](const MidiChannelMessage &m) { msgs.push_back(m); });

  for (auto type : message_types::channel::all) {
    MidiStatus status(type, rng.channel());
    uint8_t input[]{status};
    parser.feed(input, 1);
    TEST_ASSERT_TRUE(parser.has_status());
    TEST_ASSERT_TRUE(status == parser.status());
    for (size_t i = 8; i < 16; i++) {
      input[0] = MidiStatus(0xF0 | i);
      parser.feed(input, 1);
      TEST_ASSERT_TRUE(parser.has_status());
      TEST_ASSERT_TRUE(status == parser.status());
    }
  }

  TEST_ASSERT_EQUAL(0, msgs.size());
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(parser_empty);
  RUN_TEST(parser_feed_note_on);
  RUN_TEST(parser_feed_note_off);
  RUN_TEST(parser_feed_program_change);
  RUN_TEST(parser_feed_after_touch);
  RUN_TEST(parser_feed_after_touch_channel);

  RUN_TEST(parser_feed_bytes_with_status);
  RUN_TEST(parser_feed_bytes_with_running_status);
  RUN_TEST(parser_feed_bytes_with_status_single_value);
  RUN_TEST(parser_feed_bytes_with_running_status_single_value);

  RUN_TEST(parser_ignores_data_with_no_status);
  RUN_TEST(parser_ignores_status_with_no_data);

  RUN_TEST(parser_feed_bytes_with_status_many);

  RUN_TEST(parser_clears_status_on_non_realtime_system_messages);
  RUN_TEST(parser_does_not_clear_status_on_realtime_system_messages);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
