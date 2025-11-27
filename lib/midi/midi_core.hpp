#pragma once
#include <cstdint>
#include <string>

namespace teslasynth::midi {

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

enum class ControlChange : uint8_t {
  // Continuous Controllers MSB (0-31)
  BANK_SELECT_MSB = 0,
  MODULATION_MSB = 1,
  BREATH_MSB = 2,
  FOOT_CONTROLLER_MSB = 4,
  PORTAMENTO_TIME_MSB = 5,
  DATA_ENTRY_MSB = 6,
  CHANNEL_VOLUME_MSB = 7,
  BALANCE_MSB = 8,
  PAN_MSB = 10,
  EXPRESSION_MSB = 11,
  EFFECT_CONTROL_1_MSB = 12,
  EFFECT_CONTROL_2_MSB = 13,
  GENERAL_PURPOSE_1 = 16,
  GENERAL_PURPOSE_2 = 17,
  GENERAL_PURPOSE_3 = 18,
  GENERAL_PURPOSE_4 = 19,

  // Switches (64-69)
  DAMPER_PEDAL = 64, // Sustain
  PORTAMENTO_SWITCH = 65,
  SOSTENUTO_SWITCH = 66,
  SOFT_PEDAL = 67,
  LEGATO_SWITCH = 68,
  HOLD_2 = 69,

  // Sound Controllers (70-79)
  SOUND_VARIATION = 70,
  RESONANCE = 71,
  RELEASE_TIME = 72,
  ATTACK_TIME = 73,
  BRIGHTNESS = 74,
  DECAY_TIME = 75,
  VIBRATO_RATE = 76,
  VIBRATO_DEPTH = 77,
  VIBRATO_DELAY = 78,

  // Effects (91-95)
  EFFECTS_1_DEPTH = 91, // Reverb
  EFFECTS_2_DEPTH = 92, // Tremolo
  EFFECTS_3_DEPTH = 93, // Chorus
  EFFECTS_4_DEPTH = 94, // Celeste/Detune
  EFFECTS_5_DEPTH = 95, // Phaser

  // Parameter Management (96-101)
  DATA_INCREMENT = 96,
  DATA_DECREMENT = 97,
  NRPN_LSB = 98,
  NRPN_MSB = 99,
  RPN_LSB = 100,
  RPN_MSB = 101,

  // Channel Mode Messages (120-127)
  ALL_SOUND_OFF = 120,
  RESET_ALL_CONTROLLERS = 121,
  LOCAL_CONTROL = 122,
  ALL_NOTES_OFF = 123,
  OMNI_MODE_OFF = 124,
  OMNI_MODE_ON = 125,
  MONO_MODE_ON = 126,
  POLY_MODE_ON = 127
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

  inline operator std::string() const { return std::to_string(value); }
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

  static constexpr MidiChannelMessage
  control_change(uint8_t ch, ControlChange number, uint8_t value) {
    return {
        .type = MidiMessageType::ControlChange,
        .channel = ch,
        .data0 = static_cast<uint8_t>(number),
        .data1 = value,
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
    std::string res = "Channel: " + std::to_string(channel) + " ";
    switch (type) {
    case MidiMessageType::NoteOn:
      res += "Note On " + std::to_string(data0) + ", " + std::to_string(data1);
      break;
    case MidiMessageType::NoteOff:
      res += "Note Off " + std::to_string(data0) + ", " + std::to_string(data1);
      break;
    case MidiMessageType::ControlChange:
      res += "Control change " + std::to_string(data0) + ", " +
             std::to_string(data1);
      break;
    case MidiMessageType::ProgramChange:
      res += "Program change " + std::to_string(data0);
      break;
    case MidiMessageType::AfterTouchChannel:
      res += "After touch " + std::to_string(data0);
      break;
    case MidiMessageType::AfterTouchPoly:
      res += "After touch poly " + std::to_string(data0) + ", " +
             std::to_string(data1);
      break;
    case MidiMessageType::PitchBend:
      res += "Pitch " + std::to_string(data0) + ", " + std::to_string(data1);
      break;
    default:
      res += "Unknown type: " + std::to_string(static_cast<uint8_t>(type));
    }
    return res;
  }

  constexpr bool is_control() const {
    return type == MidiMessageType::ControlChange;
  }
  constexpr bool is_channel_mode_control() const {
    return is_control() && data0 >= 120;
  }
};

} // namespace teslasynth::midi
