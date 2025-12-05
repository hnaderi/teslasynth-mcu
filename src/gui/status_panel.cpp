#include "configuration/synth.hpp"
#include "core/lv_obj.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "font/lv_symbol_def.h"
#include "freertos/task.h"
#include "input/ble_midi.hpp"
#include "misc/lv_area.h"
#include "notes.hpp"
#include "synthesizer_events.hpp"
#include "widgets/label/lv_label.h"
#include <cstdint>
#include <sys/lock.h>
#include <sys/param.h>
#include <unistd.h>

#if CONFIG_TESLASYNTH_GUI_STATUS_PANEL

LV_IMG_DECLARE(teslasynth_tiny);

namespace teslasynth::app::gui {
extern lv_display_t *install_display();

static const char *TAG = "GUI";
static lv_display_t *display;

lv_obj_t *main_screen, *splash_screen;

void init_ui() {
  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
}

lv_obj_t *label1, *label2;
void render_config(void *) {
  if (label1 == nullptr || label2 == nullptr)
    return;
  // TODO new
  const Config config; //= app::configuration::get_config();

  lv_label_set_text_fmt(label1, "Max on: %s",
                        std::string(config.max_on_time).c_str());
  lv_label_set_text_fmt(label2, "Notes: %u", config.notes);
}

void splash_load_cb(lv_event_t *e) {
  /* load main screen after 3000ms */
  lv_screen_load_anim(main_screen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 3000, false);
}

lv_obj_t *bluetooth_indicator, *play_indicator;
static lv_timer_t *blink_timer;

static void blink_cb(lv_timer_t *t) {
  lv_obj_t *icon = static_cast<lv_obj_t *>(lv_timer_get_user_data(t));

  if (lv_obj_has_flag(icon, LV_OBJ_FLAG_HIDDEN))
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
}
static void ui_on_connection_changed(void *event) {
  if (static_cast<bool>(event)) {
    lv_timer_pause(blink_timer);
    lv_obj_clear_flag(bluetooth_indicator, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_timer_resume(blink_timer);
  }
}

static void ui_on_track_play_changed(void *event) {
  if (static_cast<bool>(event)) {
    lv_image_set_src(play_indicator, &LV_SYMBOL_PLAY);
  } else {
    lv_image_set_src(play_indicator, &LV_SYMBOL_PAUSE);
  }
}

void init_splash_screen() {
  ESP_LOGI(TAG, "splash screen");
  splash_screen = lv_obj_create(nullptr);

  lv_obj_t *icon = lv_image_create(splash_screen);
  lv_image_set_src(icon, &teslasynth_tiny);
  lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 0, 0);
}
void init_main_screen() {
  ESP_LOGI(TAG, "main screen");
  main_screen = lv_obj_create(nullptr);

  bluetooth_indicator = lv_image_create(main_screen);
  lv_image_set_src(bluetooth_indicator, &LV_SYMBOL_BLUETOOTH);
  lv_obj_align(bluetooth_indicator, LV_ALIGN_TOP_LEFT, 0, 0);

  play_indicator = lv_image_create(main_screen);
  lv_image_set_src(play_indicator, &LV_SYMBOL_PAUSE);
  lv_obj_align(play_indicator, LV_ALIGN_TOP_LEFT, 20, 0);

  lv_obj_t *tslabel = lv_label_create(main_screen);
  lv_label_set_long_mode(tslabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_label_set_text(tslabel, "TeslaSynth");
  lv_obj_set_width(tslabel, lv_display_get_horizontal_resolution(display));
  lv_obj_align(tslabel, LV_ALIGN_TOP_RIGHT, 40, 0);

  blink_timer = lv_timer_create(blink_cb, 750, bluetooth_indicator);

  label1 = lv_label_create(main_screen);
  lv_label_set_long_mode(label1, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(label1, lv_display_get_horizontal_resolution(display));
  lv_obj_align(label1, LV_ALIGN_LEFT_MID, 0, 0);

  label2 = lv_label_create(main_screen);
  lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);

  lv_obj_set_width(label2, lv_display_get_horizontal_resolution(display));
  lv_obj_align(label2, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  render_config(nullptr);
}

static void ble_event_handler(void *, esp_event_base_t, int32_t id, void *) {
  lv_async_call(ui_on_connection_changed, (void *)(id == BLE_DEVICE_CONNECTED));
}
static void track_event_handler(void *, esp_event_base_t, int32_t id, void *) {
  lv_async_call(ui_on_track_play_changed, (void *)(id == SYNTHESIZER_PLAYING));
}
static void config_update_handler(void *, esp_event_base_t, int32_t, void *) {
  lv_async_call(render_config, nullptr);
}

void init() {
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

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      EVENT_SYNTHESIZER_BASE, SYNTHESIZER_PLAYING, track_event_handler, nullptr,
      nullptr));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      EVENT_SYNTHESIZER_BASE, SYNTHESIZER_STOPPED, track_event_handler, nullptr,
      nullptr));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      EVENT_SYNTHESIZER_BASE, SYNTHESIZER_CONFIG_UPDATED, config_update_handler,
      nullptr, nullptr));
}

} // namespace teslasynth::app::gui

#endif
