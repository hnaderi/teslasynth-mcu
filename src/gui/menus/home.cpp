#include "configuration/synth.hpp"
#include "lvgl.h"
#include "midi_synth.hpp"
#include "notes.hpp"

namespace teslasynth::app::gui {
static lv_obj_t *label, *label2;

static void render_home_section(void *) {
  if (label == nullptr || label2 == nullptr)
    return;
  // TODO new menus
  const Config config; //= teslasynth::app::configuration::get_config();

  lv_label_set_text_fmt(label, "Max on: %s",
                        std::string(config.max_on_time).c_str());
  lv_label_set_text_fmt(label2, "Notes: %u", config.notes);
}

lv_obj_t *create_home_section(lv_obj_t *menu) {
  lv_obj_t *sub_status_page = lv_menu_page_create(menu, "Home");
  lv_obj_set_style_pad_hor(
      sub_status_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_status_page);
  auto section = lv_menu_section_create(sub_status_page);

  label = lv_label_create(section);
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(label, lv_display_get_horizontal_resolution(nullptr));
  lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

  label2 = lv_label_create(section);
  lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);

  lv_obj_set_width(label2, lv_display_get_horizontal_resolution(nullptr));
  lv_obj_align(label2, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  render_home_section(nullptr);

  return sub_status_page;
}

} // namespace teslasynth::app::gui
