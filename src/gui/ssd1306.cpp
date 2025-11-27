#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "esp_lvgl_port_disp.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "soc/gpio_num.h"
#include <stdio.h>
#include <sys/lock.h>
#include <sys/param.h>
#include <unistd.h>

#if CONFIG_TESLASYNTH_DISPLAY_PANEL_SSD1306_128x64

#define I2C_BUS_PORT 0

#define DISPLAY_LCD_PIXEL_CLOCK_HZ (400 * 1000)
#define DISPLAY_PIN_NUM_RST -1
#define DISPLAY_I2C_HW_ADDR 0x3C
#define DISPLAY_LCD_CMD_BITS 8
#define DISPLAY_LCD_PARAM_BITS 8

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

namespace teslasynth::app::gui {
static const char *TAG = "DISPLAY";

esp_lcd_panel_io_handle_t install_io() {
  ESP_LOGI(TAG, "Initialize I2C bus");
  i2c_master_bus_handle_t i2c_bus = NULL;
  i2c_master_bus_config_t bus_config = {
      .i2c_port = I2C_BUS_PORT,
      .sda_io_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_DISPLAY_INTERFACE_I2C_SDA),
      .scl_io_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_DISPLAY_INTERFACE_I2C_SCL),
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .flags =
          {
              .enable_internal_pullup = true,
          },
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

  ESP_LOGI(TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_i2c_config_t io_config = {
      .dev_addr = DISPLAY_I2C_HW_ADDR,
      .control_phase_bytes = 1,               // According to SSD1306 datasheet
      .dc_bit_offset = 6,                     // According to SSD1306 datasheet
      .lcd_cmd_bits = DISPLAY_LCD_CMD_BITS,   // According to SSD1306 datasheet
      .lcd_param_bits = DISPLAY_LCD_CMD_BITS, // According to SSD1306 datasheet
      .scl_speed_hz = DISPLAY_LCD_PIXEL_CLOCK_HZ,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));
  return io_handle;
}

esp_lcd_panel_handle_t install_panel(esp_lcd_panel_io_handle_t io_handle) {
  ESP_LOGI(TAG, "Install SSD1306 panel driver");
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = DISPLAY_PIN_NUM_RST,
      .bits_per_pixel = 1,
  };
  esp_lcd_panel_ssd1306_config_t ssd1306_config = {
      .height = DISPLAY_HEIGHT,
  };
  panel_config.vendor_config = &ssd1306_config;
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));

  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
  return panel_handle;
}

lv_display_t *install_display() {
  auto io_handle = install_io();
  auto panel_handle = install_panel(io_handle);

  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size = DISPLAY_WIDTH * DISPLAY_HEIGHT,
      .double_buffer = true,
      .hres = DISPLAY_WIDTH,
      .vres = DISPLAY_HEIGHT,
      .monochrome = true,
      .rotation =
          {
              .swap_xy = false,
              .mirror_x = false,
              .mirror_y = false,
          },
      .flags =
          {
              .buff_dma = true,
              .swap_bytes = false,
          },
  };
  ESP_LOGI(TAG, "Install display");
  lv_display_t *disp_handle = lvgl_port_add_disp(&disp_cfg);
  return disp_handle;
}

} // namespace teslasynth::app::gui
#endif
