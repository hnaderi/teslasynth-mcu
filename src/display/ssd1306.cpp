#include "ssd1306.h"
#include "configuration/synth.hpp"
#include "display.hpp"
#include "display/media/128x64.h"
#include "freertos/idf_additions.h"
#include "notes.hpp"
#include "portmacro.h"
#include <cstdint>
#include <cstring>
#include <stdio.h>

SSD1306_t dev;

void init() {
  i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
  ssd1306_init(&dev, 128, 64);
}

void render_logo() {
  ssd1306_clear_screen(&dev, false);
  ssd1306_contrast(&dev, 0xff);
  ssd1306_bitmaps(&dev, 0, 0, logo, 128, 64, false);
}

void render_overview() {
  Config &config = get_config();
  char line[32];
  ssd1306_clear_screen(&dev, false);
  ssd1306_bitmaps(&dev, 0, 0, bluetooth_icon, 16, 16, false);
  ssd1306_display_text(&dev, 1, "   Teslasynth   ", 16, false);

  snprintf(line, 16, "Max on: %uus",
           static_cast<uint16_t>(config.max_on_time.micros()));
  ssd1306_display_text(&dev, 3, line, strlen(line), false);

  snprintf(line, 32, "Deadtime: %uus",
           static_cast<uint16_t>(config.min_deadtime.micros()));
  ssd1306_display_text(&dev, 4, line, strlen(line), false);
}

void display(void *pvParams) {
  render_logo();
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  render_overview();
  vTaskDelay(portMAX_DELAY);
}

void setup_display(void) {
  init();
  xTaskCreate(display, "display", 4 * 1024, nullptr, 1, nullptr);
}
