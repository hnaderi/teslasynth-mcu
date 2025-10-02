#include "synth.hpp"
#include "instruments.hpp"

SynthChannel::SynthChannel(const Config &config)
    : notes(config), _config(config) {}

void SynthChannel::on_note_on(uint8_t number, uint8_t velocity, uint32_t time) {
  if (velocity == 0)
    on_note_off(number, velocity, time);
  else {
    notes.start(number, velocity, instrument, time);
  }
}
void SynthChannel::on_note_off(uint8_t number, uint8_t velocity,
                               uint32_t time) {
  notes.release(number, time);
}
void SynthChannel::on_program_change(uint8_t value) {}
void SynthChannel::on_control_change(uint8_t value) {}

uint16_t SynthChannel::render(Pulse *buffer, uint16_t max_size,
                              uint16_t delta) {
  NotePulse pulse;
  auto next = notes.next();
  next.tick(_config, pulse);
  return 0;
}
