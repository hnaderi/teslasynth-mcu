#include "rmt_driver.hpp"
#include "driver/rmt_common.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_types.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/rmt_types.h"
#include "midi_synth.hpp"
#include "sdkconfig.h"
#include "soc/gpio_num.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stddef.h>
#include <stdio.h>

namespace teslasynth::app::devices::rmt {
using teslasynth::midisynth::Pulse;

#define RMT_BUZZER_RESOLUTION_HZ 1'000'000

namespace {

const char *TAG = "RMT-DRIVER";

inline void symbol_for_idx(Pulse const *current, rmt_symbol_word_t *symbol) {
  if (current->is_zero()) {
    *symbol = {
        .duration0 = std::max<uint16_t>(1, current->off.micros() - 1),
        .level0 = 0,
        .duration1 = 1,
        .level1 = 0,
    };
  } else {
    *symbol = {
        .duration0 = current->on.micros(),
        .level0 = 1,
        .duration1 = std::max<uint16_t>(1, current->off.micros()),
        .level1 = 0,
    };
  }
}

size_t callback(const void *data, size_t data_size, size_t symbols_written,
                size_t symbols_free, rmt_symbol_word_t *symbols, bool *done,
                void *arg) {
  const Pulse *input = static_cast<const Pulse *>(data);
  const size_t data_length = data_size / sizeof(Pulse);

  size_t written = 0;
  for (; written < symbols_free && symbols_written + written < data_length;
       written++) {
    Pulse const *current = &input[symbols_written + written];
    symbol_for_idx(current, &symbols[written]);
  }
  if (written + symbols_written == data_length)
    *done = true;
  return written;
}
rmt_channel_handle_t channels[CONFIG_TESLASYNTH_OUTPUT_COUNT];
rmt_encoder_handle_t encoders[CONFIG_TESLASYNTH_OUTPUT_COUNT];

constexpr rmt_transmit_config_t tx_config = {
    .loop_count = 0,
    .flags =
        {
            .eot_level = 0,
            .queue_nonblocking = 1,
        },
};

const uint8_t output_pins[] = {
#if CONFIG_TESLASYNTH_OUTPUT_COUNT >= 1
    CONFIG_TESLASYNTH_OUTPUT_GPIO_PIN1,
#endif
#if CONFIG_TESLASYNTH_OUTPUT_COUNT >= 2
    CONFIG_TESLASYNTH_OUTPUT_GPIO_PIN2,
#endif
#if CONFIG_TESLASYNTH_OUTPUT_COUNT >= 3
    CONFIG_TESLASYNTH_OUTPUT_GPIO_PIN3,
#endif
#if CONFIG_TESLASYNTH_OUTPUT_COUNT >= 4
    CONFIG_TESLASYNTH_OUTPUT_GPIO_PIN4,
#endif
};
} // namespace

void enable(void) {
  ESP_LOGI(TAG, "Enable RMT TX channel(s)");
  for (uint8_t i = 0; i < CONFIG_TESLASYNTH_OUTPUT_COUNT; ++i) {
    ESP_ERROR_CHECK(rmt_enable(channels[i]));
  }
}

void disable(void) {
  ESP_LOGI(TAG, "Disable RMT TX channel(s)");
  for (uint8_t i = 0; i < CONFIG_TESLASYNTH_OUTPUT_COUNT; ++i) {
    ESP_ERROR_CHECK(rmt_disable(channels[i]));
  }
}

void init(void) {
  ESP_LOGI(TAG, "Create %u RMT TX channel(s)", CONFIG_TESLASYNTH_OUTPUT_COUNT);

  constexpr rmt_simple_encoder_config_t encoder_config = {
      .callback = callback,
      .arg = NULL,
      .min_chunk_size = 1,
  };
  rmt_tx_channel_config_t tx_chan_config = {
      .gpio_num = gpio_num_t::GPIO_NUM_NC,
      .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
      .resolution_hz = RMT_BUZZER_RESOLUTION_HZ,
      .mem_block_symbols = 64,
      .trans_queue_depth = 10, // set the maximum number of transactions that
                               // can pend in the background
      .flags =
          {
              .invert_out = false,
              .with_dma = false,
              .io_loop_back = false,
              .io_od_mode = false,
              .allow_pd = false,
          },
  };

  for (uint8_t i = 0; i < CONFIG_TESLASYNTH_OUTPUT_COUNT; ++i) {
    tx_chan_config.gpio_num = static_cast<gpio_num_t>(output_pins[i]);
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &channels[i]));
    ESP_ERROR_CHECK(rmt_new_simple_encoder(&encoder_config, &encoders[i]));
  }

  enable();
}

void pulse_write(const midisynth::Pulse *pulse, size_t len, uint8_t ch) {
#if CONFIG_TESLASYNTH_DEBUG
  static size_t counter = 0, min_i = std::numeric_limits<size_t>::max(),
                max_i = 0, total = 0;
  min_i = std::min(len, min_i);
  max_i = std::max(len, max_i);
  total += len;
  if (counter++ % 100 == 0) {
    ESP_LOGI(TAG, "RMT items stats, min: %u, max: %u, total: %u, avg: %u",
             min_i, max_i, total, total / counter);
  }
#endif

  if (len == 0)
    return;
  ESP_ERROR_CHECK_WITHOUT_ABORT(rmt_transmit(channels[ch], encoders[ch], pulse,
                                             len * sizeof(Pulse), &tx_config));
}
} // namespace teslasynth::app::devices::rmt
