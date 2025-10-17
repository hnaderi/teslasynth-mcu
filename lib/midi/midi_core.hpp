#pragma once
#include <cstdint>
#include <sstream>
#include <string>

enum MidiType : uint8_t {
  InvalidType = 0x00,    ///< For notifying errors
  NoteOff = 0x80,        ///< Channel Message - Note Off
  NoteOn = 0x90,         ///< Channel Message - Note On
  AfterTouchPoly = 0xA0, ///< Channel Message - Polyphonic AfterTouch
  ControlChange = 0xB0,  ///< Channel Message - Control Change / Channel Mode
  ProgramChange = 0xC0,  ///< Channel Message - Program Change
  AfterTouchChannel =
      0xD0,               ///< Channel Message - Channel (monophonic) AfterTouch
  PitchBend = 0xE0,       ///< Channel Message - Pitch Bend

  SystemExclusive = 0xF0, ///< System Exclusive
  SystemExclusiveStart = SystemExclusive, ///< System Exclusive Start
  TimeCodeQuarterFrame = 0xF1, ///< System Common - MIDI Time Code Quarter Frame
  SongPosition = 0xF2,         ///< System Common - Song Position Pointer
  SongSelect = 0xF3,           ///< System Common - Song Select
  Undefined_F4 = 0xF4,
  Undefined_F5 = 0xF5,
  TuneRequest = 0xF6,        ///< System Common - Tune Request
  SystemExclusiveEnd = 0xF7, ///< System Exclusive End
  Clock = 0xF8,              ///< System Real Time - Timing Clock
  Undefined_F9 = 0xF9,
  Tick = Undefined_F9, ///< System Real Time - Timing Tick (1 tick = 10
                       ///< milliseconds)
  Start = 0xFA,        ///< System Real Time - Start
  Continue = 0xFB,     ///< System Real Time - Continue
  Stop = 0xFC,         ///< System Real Time - Stop
  Undefined_FD = 0xFD,
  ActiveSensing = 0xFE, ///< System Real Time - Active Sensing
  SystemReset = 0xFF,   ///< System Real Time - System Reset
};

/// Channel message types (status upper nibble)
enum class MidiMessageType : uint8_t {
  NoteOff = 0x80,
  NoteOn = 0x90,
  AfterTouchPoly = 0xA0,
  ControlChange = 0xB0,
  ProgramChange = 0xC0,
  AfterTouchChannel = 0xD0,
  PitchBend = 0xE0,
};

/// Common 7-bit MIDI data byte wrapper
struct MidiData {
  uint8_t value;
  constexpr MidiData(uint8_t v = 0) : value(v & 0x7F) {}
  constexpr operator uint8_t() const { return value; }
};

struct MidiChannelNumber {
  uint8_t value;
  constexpr MidiChannelNumber(uint8_t v = 0) : value(v & 0x0F) {}
  constexpr operator uint8_t() const { return value; }
};

struct MidiStatus {
  uint8_t value;
  constexpr MidiStatus(uint8_t v = 0) : value(v | 0x80) {}
  constexpr MidiStatus(MidiMessageType type, MidiChannelNumber ch)
      : MidiStatus(static_cast<uint8_t>(type) | ch) {}
  constexpr operator uint8_t() const { return value; }

  constexpr bool is_channel() const { return (value & 0xF0) != 0xF0; }
  constexpr MidiChannelNumber channel() const {
    return MidiChannelNumber(value);
  }
  constexpr MidiMessageType channel_status_type() const {
    return static_cast<MidiMessageType>(value & 0xF0);
  }

  constexpr bool is_system() const { return (value & 0xF0) == 0xF0; }
  constexpr bool is_system_realtime() const { return (value & 0xF8) == 0xF8; }

  static constexpr bool is_status(uint8_t v) { return v & 0x80; }
  static constexpr MidiStatus min() { return 0; }

  inline operator std::string() const {
    std::stringstream stream;
    stream << std::hex << value;
    return stream.str();
  }
};

struct MidiChannelMessage {
  MidiMessageType type;
  MidiChannelNumber channel;
  MidiData data0, data1;

  static constexpr MidiChannelMessage note_on(uint8_t ch, uint8_t note,
                                              uint8_t vel) {
    return {
        .type = MidiMessageType::NoteOn,
        .channel = ch,
        .data0 = note,
        .data1 = vel,
    };
  }

  static constexpr MidiChannelMessage note_off(uint8_t ch, uint8_t note,
                                               uint8_t vel) {
    return {
        .type = MidiMessageType::NoteOff,
        .channel = ch,
        .data0 = note,
        .data1 = vel,
    };
  }

  static constexpr MidiChannelMessage after_touch(uint8_t ch, uint8_t note,
                                                  uint8_t value) {
    return {
        .type = MidiMessageType::AfterTouchPoly,
        .channel = ch,
        .data0 = note,
        .data1 = value,
    };
  }

  static constexpr MidiChannelMessage after_touch_channel(uint8_t ch,
                                                          uint8_t value) {
    return {
        .type = MidiMessageType::AfterTouchChannel,
        .channel = ch,
        .data0 = value,
        .data1 = 0,
    };
  }

  static constexpr MidiChannelMessage program_change(uint8_t ch,
                                                     uint8_t value) {
    return {
        .type = MidiMessageType::ProgramChange,
        .channel = ch,
        .data0 = value,
        .data1 = 0,
    };
  }

  constexpr bool operator==(const MidiChannelMessage &b) const {
    return type == b.type && channel == b.channel && data0 == b.data0 &&
           data1 == b.data1;
  }

  constexpr bool operator!=(const MidiChannelMessage &b) const {
    return !(*this == b);
  }

  inline operator std::string() const {
    std::stringstream stream;
    stream << "Channel: " << std::to_string(channel) << " ";
    switch (type) {
    case MidiMessageType::NoteOn:
      stream << "Note On " << std::to_string(data0) << ", "
             << std::to_string(data1);
      break;
    case MidiMessageType::NoteOff:
      stream << "Note Off " << std::to_string(data0) << ", "
             << std::to_string(data1);
      break;
    case MidiMessageType::ControlChange:
      stream << "Control change " << std::to_string(data0) << ", "
             << std::to_string(data1);
      break;
    case MidiMessageType::ProgramChange:
      stream << "Program change " << std::to_string(data0);
      break;
    case MidiMessageType::AfterTouchChannel:
      stream << "After touch " << std::to_string(data0);
      break;
    case MidiMessageType::AfterTouchPoly:
      stream << "After touch poly " << std::to_string(data0) << ", "
             << std::to_string(data1);
      break;
    case MidiMessageType::PitchBend:
      stream << "Pitch " << std::to_string(data0) << ", "
             << std::to_string(data1);
      break;
    default:
      stream << "Unknown type: " << std::to_string(static_cast<uint8_t>(type));
    }
    return stream.str();
  }
};
