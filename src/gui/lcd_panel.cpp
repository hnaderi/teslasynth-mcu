#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "esp_lvgl_port_disp.h"
#include "freertos/task.h"
#include "hardware.h"
#include "sdkconfig.h"
#include "soc/gpio_num.h"
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

static const char *TAG = "DISPLAY";

#define LCD_HOST SPI2_HOST

esp_lcd_panel_io_handle_t install_io() {
  ESP_LOGI(TAG, "Turn off LCD backlight");
  gpio_config_t bk_gpio_config = {
      .pin_bit_mask = 1ULL << CONFIG_TESLASYNTH_DISPLAY_BACKLIGHT_PIN,
      .mode = GPIO_MODE_OUTPUT,
  };
  ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

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
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
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
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));

  // user can flush pre-defined pattern to the screen before we turn on the
  // screen or backlight
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  return panel_handle;
}

lv_display_t *install_display() {
  auto io_handle = install_io();
  auto panel_handle = install_panel(io_handle);

  ESP_LOGI(TAG, "Turn on LCD backlight");
  gpio_set_level(
      static_cast<gpio_num_t>(CONFIG_TESLASYNTH_DISPLAY_BACKLIGHT_PIN),
      LCD_BK_LIGHT_ON_LEVEL);

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
              .mirror_x = false,
              .mirror_y = false,
          },
      .color_format = LV_COLOR_FORMAT_RGB565,
      .flags =
          {
              .buff_dma = true,
              .swap_bytes = false,
          },
  };
  return lvgl_port_add_disp(&disp_cfg);
}

#endif
