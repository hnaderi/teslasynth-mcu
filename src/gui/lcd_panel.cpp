#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "esp_lvgl_port_disp.h"
#include "freertos/task.h"
#include "hal/ledc_types.h"
#include "hardware.h"
#include "sdkconfig.h"
#include "soc/gpio_num.h"
#include <driver/ledc.h>
#include <stdio.h>
#include <sys/lock.h>
#include <sys/param.h>
#include <unistd.h>

#if CONFIG_TESLASYNTH_GUI_FULL

#if CONFIG_TESLASYNTH_DISPLAY_PANEL_ILI9341
#include "esp_lcd_ili9341.h"
#elif CONFIG_TESLASYNTH_DISPLAY_PANEL_ST7789
#include "esp_lcd_panel_st7789.h"
#endif

namespace teslasynth::app::gui {

static const char *TAG = "DISPLAY";

constexpr auto ledc_channel =
    static_cast<ledc_channel_t>(LCD_BACKLIGHT_LEDC_CH);
esp_err_t lcd_display_brightness_init(void) {
  const ledc_channel_config_t LCD_backlight_channel = {
      .gpio_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_DISPLAY_BACKLIGHT_PIN),
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = ledc_channel,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = ledc_timer_t::LEDC_TIMER_1,
      .duty = 0,
      .hpoint = 0,
      .flags = {
          .output_invert = LCD_BK_LIGHT_OFF_LEVEL,
      }};

  const ledc_timer_config_t LCD_backlight_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_10_BIT,
      .timer_num = ledc_timer_t::LEDC_TIMER_1,
      .freq_hz = 5000,
      .clk_cfg = LEDC_AUTO_CLK,
  };

  ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
  ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));

  return ESP_OK;
}

esp_err_t lcd_display_brightness_set(int brightness_percent) {
  if (brightness_percent > 100) {
    brightness_percent = 100;
  }
  if (brightness_percent < 0) {
    brightness_percent = 0;
  }

  ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness_percent);

  uint32_t duty_cycle = (1023 * brightness_percent) / 100;

  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_channel, duty_cycle));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_channel));

  return ESP_OK;
}

esp_err_t lcd_display_backlight_off(void) {
  ESP_LOGI(TAG, "Turn off LCD backlight");
  return lcd_display_brightness_set(0);
}

esp_err_t lcd_display_backlight_on(void) {
  ESP_LOGI(TAG, "Turn on LCD backlight");
  return lcd_display_brightness_set(100);
}

#define LCD_HOST SPI2_HOST

esp_lcd_panel_io_handle_t install_io() {
  lcd_display_brightness_init();

  ESP_LOGI(TAG, "Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .mosi_io_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_DISPLAY_INTERFACE_SPI_MOSI),
      .miso_io_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_DISPLAY_INTERFACE_SPI_MISO),
      .sclk_io_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_DISPLAY_INTERFACE_SPI_CLK),
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = DISPLAY_HEIGHT * 80 * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
      .cs_gpio_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_DISPLAY_INTERFACE_SPI_CS),
      .dc_gpio_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_DISPLAY_INTERFACE_SPI_DC),
      .spi_mode = 0,
      .pclk_hz = 20 * 1000 * 1000, // 20MHz
      .trans_queue_depth = 10,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
  };
  // Attach the LCD to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io_handle));
  return io_handle;
}

esp_lcd_panel_handle_t install_panel(esp_lcd_panel_io_handle_t io_handle) {
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_DISPLAY_INTERFACE_SPI_RS),
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16,
  };
#if CONFIG_TESLASYNTH_DISPLAY_PANEL_ILI9341
  ESP_LOGI(TAG, "Install ILI9341 panel driver");
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
#elif CONFIG_TESLASYNTH_DISPLAY_PANEL_ST7789
  ESP_LOGI(TAG, "Install ST7789 panel driver");
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
#endif
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(
      esp_lcd_panel_mirror(panel_handle, LCD_MIRROR_X, LCD_MIRROR_Y));

  // user can flush pre-defined pattern to the screen before we turn on the
  // screen or backlight
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  return panel_handle;
}

lv_display_t *install_display() {
  auto io_handle = install_io();
  auto panel_handle = install_panel(io_handle);

  ESP_LOGI(TAG, "Install the display");
  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = io_handle,
      .panel_handle = panel_handle,
      .buffer_size = DISPLAY_HEIGHT * 50,
      .double_buffer = true,
      .hres = DISPLAY_WIDTH,
      .vres = DISPLAY_HEIGHT,
      .monochrome = false,
      .rotation =
          {
              .swap_xy = false,
              .mirror_x = LCD_MIRROR_X,
              .mirror_y = LCD_MIRROR_Y,
          },
      .color_format = LV_COLOR_FORMAT_RGB565,
      .flags =
          {
              .buff_dma = true,
              .swap_bytes = true,
          },
  };
  return lvgl_port_add_disp(&disp_cfg);
}

}
#endif
