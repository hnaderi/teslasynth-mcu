#include "display.hpp"
#include "configuration/synth.hpp"
#include "core/lv_obj.h"
#include "esp_err.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/task.h"
#include "input/ble_midi.hpp"
#include "media/128x64.h"
#include "misc/lv_area.h"
#include "notes.hpp"
#include "widgets/label/lv_label.h"
#include <sys/lock.h>
#include <sys/param.h>
#include <unistd.h>

static const char *TAG = "display";

static lv_display_t *display;
extern lv_display_t *install_display();

lv_obj_t *main_screen, *splash_screen;

void init_ui() {
  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
}

void render_config(lv_obj_t *parent) {
  Config &config = get_config();

  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_label_set_text_fmt(label, "Max on: %s",
                        std::string(config.max_on_time).c_str());
  lv_obj_set_width(label, lv_display_get_horizontal_resolution(display));
  lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
}

void splash_load_cb(lv_event_t *e) {
  /* load main screen after 3000ms */
  lv_screen_load_anim(main_screen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 3000, false);
}

lv_obj_t *bluetooth_indicator;
static lv_timer_t *blink_timer;

static void blink_cb(lv_timer_t *t) {
  lv_obj_t *icon = static_cast<lv_obj_t *>(lv_timer_get_user_data(t));

  if (lv_obj_has_flag(icon, LV_OBJ_FLAG_HIDDEN))
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
}
static void ui_on_connected(void) {
  lv_timer_pause(blink_timer);
  lv_obj_clear_flag(bluetooth_indicator, LV_OBJ_FLAG_HIDDEN);
}
static void ui_on_disconnected(void) { lv_timer_resume(blink_timer); }

static void ble_event_handler(void *handler_args, esp_event_base_t base,
                              int32_t id, void *event_data) {
  if (id == BLE_DEVICE_CONNECTED) {
    lv_async_call((lv_async_cb_t)ui_on_connected, NULL);
  } else if (id == BLE_DEVICE_DISCONNECTED) {
    lv_async_call((lv_async_cb_t)ui_on_disconnected, NULL);
  }
}

void init_splash_screen() {
  ESP_LOGI(TAG, "splash screen");
  splash_screen = lv_obj_create(nullptr);

  lv_obj_t *icon = lv_image_create(splash_screen);
  lv_image_set_src(icon, &logo);
  lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 0, 0);
}
void init_main_screen() {
  ESP_LOGI(TAG, "main screen");
  main_screen = lv_obj_create(nullptr);

  bluetooth_indicator = lv_image_create(main_screen);
  lv_image_set_src(bluetooth_indicator, &bluetooth_icon);
  lv_obj_align(bluetooth_indicator, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *label = lv_label_create(main_screen);
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_label_set_text(label, "TeslaSynth");
  lv_obj_set_width(label, lv_display_get_horizontal_resolution(display));
  lv_obj_align(label, LV_ALIGN_TOP_RIGHT, 20, 0);

  blink_timer = lv_timer_create(blink_cb, 750, bluetooth_indicator);
  render_config(main_screen);
}

void setup_ui() {
  init_ui();
  display = install_display();
  ESP_LOGI(TAG, "starting the UI");
  if (lvgl_port_lock(0)) {
    /* Rotation of the screen */
    // lv_disp_set_rotation(display, LV_DISPLAY_ROTATION_0);

    init_splash_screen();
    init_main_screen();

    lv_obj_add_event_cb(splash_screen, splash_load_cb, LV_EVENT_SCREEN_LOADED,
                        NULL);
    lv_scr_load(splash_screen);
    // Release the mutex
    lvgl_port_unlock();
  }
  ESP_ERROR_CHECK(
      esp_event_handler_instance_register(EVENT_BLE_BASE, BLE_DEVICE_CONNECTED,
                                          ble_event_handler, nullptr, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      EVENT_BLE_BASE, BLE_DEVICE_DISCONNECTED, ble_event_handler, nullptr,
      nullptr));
}
