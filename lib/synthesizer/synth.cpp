#include "synth.hpp"
#include "core.hpp"
#include "instruments.hpp"

SynthChannel::SynthChannel(const Config &config, Notes &notes)
    : _notes(notes), _config(config) {}

void SynthChannel::on_note_on(uint8_t number, uint8_t velocity, Duration time) {
  if (velocity == 0)
    on_note_off(number, velocity, time);
  else {
    _notes.start({number, velocity}, time, instruments[_instrument], _config);
  }
}

void SynthChannel::on_note_off(uint8_t number, uint8_t velocity,
                               Duration time) {
  _notes.release(number, time);
}

void SynthChannel::on_program_change(uint8_t value) {}

void SynthChannel::on_control_change(uint8_t value) {}
