#include "rmt_driver.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/rmt_types.h"
#include "soc/gpio_num.h"
#include <cstdint>
#include <stddef.h>
#include <stdio.h>

#define RMT_BUZZER_RESOLUTION_HZ 2'000'000
#define RMT_BUZZER_GPIO_NUM GPIO_NUM_4

static const char *TAG = "RMT-DRIVER";

typedef struct {
  uint32_t freq_hz;
  uint32_t duration_ms;
} buzzer_musical_score_t;

/**
 * @brief Musical Score: Beethoven's Ode to joy
 */
static const buzzer_musical_score_t score[] = {
    {740, 400}, {740, 600}, {784, 400}, {880, 400}, {880, 400}, {784, 400},
    {740, 400}, {659, 400}, {587, 400}, {587, 400}, {659, 400}, {740, 400},
    {740, 400}, {740, 200}, {659, 200}, {659, 800},

    {740, 400}, {740, 600}, {784, 400}, {880, 400}, {880, 400}, {784, 400},
    {740, 400}, {659, 400}, {587, 400}, {587, 400}, {659, 400}, {740, 400},
    {659, 400}, {659, 200}, {587, 200}, {587, 800},

    {659, 400}, {659, 400}, {740, 400}, {587, 400}, {659, 400}, {740, 200},
    {784, 200}, {740, 400}, {587, 400}, {659, 400}, {740, 200}, {784, 200},
    {740, 400}, {659, 400}, {587, 400}, {659, 400}, {440, 400}, {440, 400},

    {740, 400}, {740, 600}, {784, 400}, {880, 400}, {880, 400}, {784, 400},
    {740, 400}, {659, 400}, {587, 400}, {587, 400}, {659, 400}, {740, 400},
    {659, 400}, {659, 200}, {587, 200}, {587, 800},
};

struct encoder_state {
  uint32_t time = 0;
  size_t idx = 0;
};

uint32_t symbol_for_idx(buzzer_musical_score_t const *current,
                        rmt_symbol_word_t *symbol) {
  uint32_t rmt_raw_symbol_period = RMT_BUZZER_RESOLUTION_HZ / current->freq_hz;
  uint16_t rmt_raw_symbol_on = rmt_raw_symbol_period / 2;
  uint16_t rmt_raw_symbol_off = rmt_raw_symbol_period - rmt_raw_symbol_on;
  *symbol = {
      .duration0 = rmt_raw_symbol_on,
      .level0 = 1,
      .duration1 = rmt_raw_symbol_off,
      .level1 = 0,
  };
  return rmt_raw_symbol_period / 2;
}

static size_t callback(const void *data, size_t data_size,
                       size_t symbols_written, size_t symbols_free,
                       rmt_symbol_word_t *symbols, bool *done, void *arg) {
  size_t written = 0;
  encoder_state *state = static_cast<encoder_state *>(arg);
  const buzzer_musical_score_t *input =
      static_cast<const buzzer_musical_score_t *>(data);

  const size_t data_length = data_size / sizeof(buzzer_musical_score_t);
  if (state->idx >= data_length) {
    *done = true;
    *state = {};
    return written;
  }

  buzzer_musical_score_t const *current = &input[state->idx];
  rmt_symbol_word_t current_symbol;
  uint32_t current_period = symbol_for_idx(current, &current_symbol);

  for (; written < symbols_free; written++) {
    if (current->duration_ms * 1000 <= state->time) {
      state->idx++;
      state->time = 0;
      if (state->idx < data_length) {
        current = &input[state->idx];
        current_period = symbol_for_idx(current, &current_symbol);
      } else
        break;
    }
    symbols[written] = current_symbol;
    state->time += current_period;
  }
  return written;
}

void rmt_driver(void) {
  ESP_LOGI(TAG, "Create RMT TX channel");
  rmt_channel_handle_t buzzer_chan = NULL;
  rmt_tx_channel_config_t tx_chan_config = {
      .gpio_num = RMT_BUZZER_GPIO_NUM,
      .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
      .resolution_hz = RMT_BUZZER_RESOLUTION_HZ,
      .mem_block_symbols = 64,
      .trans_queue_depth = 10, // set the maximum number of transactions that
                               // can pend in the background
  };
  ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &buzzer_chan));

  ESP_LOGI(TAG, "Install musical score encoder");

  rmt_encoder_handle_t encoder = NULL;
  encoder_state state;
  rmt_simple_encoder_config_t encoder_config = {
      .callback = callback, .arg = &state, .min_chunk_size = 1};
  ESP_ERROR_CHECK(rmt_new_simple_encoder(&encoder_config, &encoder));

  ESP_LOGI(TAG, "Enable RMT TX channel");
  ESP_ERROR_CHECK(rmt_enable(buzzer_chan));
  ESP_LOGI(TAG, "Playing Beethoven's Ode to joy...");

  const rmt_transmit_config_t tx_config = {
      .loop_count = 0, .flags = {.eot_level = 0, .queue_nonblocking = 0}};
  ESP_ERROR_CHECK(
      rmt_transmit(buzzer_chan, encoder, score, sizeof(score), &tx_config));
}
