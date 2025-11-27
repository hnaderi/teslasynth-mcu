
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "esp_lvgl_port_touch.h"
#include "freertos/task.h"
#include "hardware.h"
#include "misc/lv_types.h"
#include "sdkconfig.h"
#include "soc/gpio_num.h"
#include <stdio.h>
#include <sys/lock.h>
#include <sys/param.h>
#include <unistd.h>

#if CONFIG_TESLASYNTH_TOUCH_ENABLED

#if CONFIG_TESLASYNTH_TOUCH_PANEL_STMPE610
#include "esp_lcd_touch_stmpe610.h"
#elif CONFIG_TESLASYNTH_TOUCH_PANEL_XPT2046
#include "esp_lcd_touch_xpt2046.h"
#endif

#define TOUCH_SPI SPI3_HOST
#define TOUCH_CLOCK_HZ ESP_LCD_TOUCH_SPI_CLOCK_HZ

namespace teslasynth::app::gui {
static const char *TAG = "DISPLAY";
namespace touch {
esp_lcd_panel_io_handle_t install_io() {
  static const int SPI_MAX_TRANSFER_SIZE = 32768;
  const spi_bus_config_t buscfg_touch = {
      .mosi_io_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_TOUCH_INTERFACE_SPI_MOSI),
      .miso_io_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_TOUCH_INTERFACE_SPI_MISO),
      .sclk_io_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_TOUCH_INTERFACE_SPI_CLK),
      .quadwp_io_num = GPIO_NUM_NC,
      .quadhd_io_num = GPIO_NUM_NC,
      .data4_io_num = GPIO_NUM_NC,
      .data5_io_num = GPIO_NUM_NC,
      .data6_io_num = GPIO_NUM_NC,
      .data7_io_num = GPIO_NUM_NC,
      .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
      .flags = SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MISO |
               SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_MASTER |
               SPICOMMON_BUSFLAG_GPIO_PINS,
      .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
      .intr_flags = ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM};

  ESP_ERROR_CHECK(
      spi_bus_initialize(TOUCH_SPI, &buscfg_touch, SPI_DMA_CH_AUTO));

  esp_lcd_panel_io_handle_t tp_io_handle = NULL;
  const esp_lcd_panel_io_spi_config_t tp_io_config = {
      .cs_gpio_num = CONFIG_TESLASYNTH_TOUCH_INTERFACE_SPI_CS,
      .dc_gpio_num = CONFIG_TESLASYNTH_TOUCH_INTERFACE_SPI_DC,
      .spi_mode = 0,
      .pclk_hz = ESP_LCD_TOUCH_SPI_CLOCK_HZ,
      .trans_queue_depth = 3,
      .on_color_trans_done = NULL,
      .user_ctx = NULL,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .flags =
          {
              .dc_high_on_cmd = 0,
              .dc_low_on_data = 0,
              .dc_low_on_param = 0,
              .octal_mode = 0,
              .quad_mode = 0,
              .sio_mode = 0,
              .lsb_first = 0,
              .cs_high_active = 0,
          },
  };

  ESP_ERROR_CHECK(
      esp_lcd_new_panel_io_spi(TOUCH_SPI, &tp_io_config, &tp_io_handle));

  return tp_io_handle;
}

esp_lcd_touch_handle_t install_panel(esp_lcd_panel_io_handle_t tp_io_handle) {
  esp_lcd_touch_handle_t tp = NULL;

  const esp_lcd_touch_config_t tp_cfg = {
      .x_max = DISPLAY_HEIGHT,
      .y_max = DISPLAY_WIDTH,
      .rst_gpio_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_TOUCH_INTERFACE_SPI_RS),
      .int_gpio_num =
          static_cast<gpio_num_t>(CONFIG_TESLASYNTH_TOUCH_INTERFACE_SPI_IRQ),
      .levels = {.reset = 0, .interrupt = 0},
      .flags =
          {
              .swap_xy = true,
              .mirror_x = true,
              .mirror_y = true,
          },
      // .process_coordinates = process_coordinates,
      .interrupt_callback = NULL};

#ifdef CONFIG_TESLASYNTH_TOUCH_PANEL_STMPE610
  ESP_LOGI(TAG, "Initialize touch controller STMPE610");
  ESP_ERROR_CHECK(esp_lcd_touch_new_spi_stmpe610(tp_io_handle, &tp_cfg, &tp));
#elif CONFIG_TESLASYNTH_TOUCH_PANEL_XPT2046
  ESP_LOGI(TAG, "Initialize touch controller XPT2046");
  ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));
#endif

  return tp;
}
} // namespace touch

lv_indev_t *install_touch(lv_display_t *display) {
  auto io_handle = touch::install_io();
  auto tp = touch::install_panel(io_handle);

  const lvgl_port_touch_cfg_t touch_cfg = {.disp = display, .handle = tp};
  static lv_indev_t *indev = lvgl_port_add_touch(&touch_cfg);
  return indev;
}
} // namespace teslasynth::app::gui

#endif
