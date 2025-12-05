#include "configuration/synth.hpp"

#include "lvgl.h"
#include "midi_synth.hpp"
#include "notes.hpp"

namespace teslasynth::app::gui {

struct ConfigSliderContext {
  uint32_t value;
  const char *const fmt;
  lv_obj_t *label = nullptr;
};

static void slider_event_cb(lv_event_t *e) {
  lv_obj_t *slider = lv_event_get_target_obj(e);
  ConfigSliderContext *ctx =
      static_cast<ConfigSliderContext *>(lv_event_get_user_data(e));

  ctx->value = lv_slider_get_value(slider);
  lv_label_set_text_fmt(ctx->label, ctx->fmt, ctx->value);
  lv_obj_align_to(ctx->label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void create_config_slider(lv_obj_t *parent, ConfigSliderContext &ctx,
                          uint32_t min, uint32_t max) {
  lv_obj_t *obj = lv_menu_cont_create(parent);

  ctx.label = lv_label_create(obj);
  lv_label_set_text_fmt(ctx.label, ctx.fmt, ctx.value);
  lv_label_set_long_mode(ctx.label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
  lv_obj_set_flex_grow(ctx.label, 1);

  auto img = lv_image_create(obj);
  lv_image_set_src(img, LV_SYMBOL_SETTINGS);
  lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

  lv_obj_t *slider = lv_slider_create(obj);
  lv_obj_set_flex_grow(slider, 1);
  lv_slider_set_range(slider, min, max);
  lv_slider_set_value(slider, ctx.value, LV_ANIM_OFF);
  lv_obj_set_width(slider, LV_PCT(80));

  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, &ctx);
}

lv_obj_t *create_configuration_section(lv_obj_t *menu) {
  lv_obj_t *sub_configuration_page = lv_menu_page_create(menu, "Config");
  lv_obj_set_style_pad_hor(
      sub_configuration_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_configuration_page);
  auto section = lv_menu_section_create(sub_configuration_page);

  // TODO new menu
  const Config config; //= app::configuration::get_config();
  static ConfigSliderContext max_on_time =
                                 {
                                     .value = config.max_on_time.micros(),
                                     .fmt = "Max on-time: %" PRIi32 " us",
                                 },
                             min_deadtime =
                                 {
                                     .value = config.min_deadtime.micros(),
                                     .fmt = "Min dead-time: %" PRIi32 " us",
                                 },
                             max_notes = {
                                 .value = config.notes,
                                 .fmt = "Max notes: %" PRIi32,
                             };
  create_config_slider(section, max_on_time, 1, 200);
  create_config_slider(section, min_deadtime, 1, 200);
  create_config_slider(section, max_notes, 1, midisynth::Config::max_notes);

  lv_obj_t *buttons = lv_obj_create(section);
  lv_obj_set_flex_flow(buttons, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(buttons, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(buttons, 10, 0);
  lv_obj_set_style_border_width(buttons, 0, 0);
  lv_obj_set_width(buttons, LV_PCT(100));

  lv_obj_t *btn_reset = lv_btn_create(buttons);
  lv_obj_set_size(btn_reset, 70, 40);
  lv_obj_t *lbl_reset = lv_label_create(btn_reset);
  lv_label_set_text(lbl_reset, "Reset");
  lv_obj_center(lbl_reset);

  lv_obj_t *btn_save = lv_btn_create(buttons);
  lv_obj_set_size(btn_save, 70, 40);
  lv_obj_t *lbl_save = lv_label_create(btn_save);
  lv_label_set_text(lbl_save, "Save");
  lv_obj_center(lbl_save);

  return sub_configuration_page;
}

} // namespace teslasynth::app::gui
