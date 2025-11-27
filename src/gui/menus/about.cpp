#include "esp_app_desc.h"
#include "esp_idf_version.h"
#include "gui/components.hpp"
#include "helpers/sysinfo.h"

namespace teslasynth::app::gui {

static lv_obj_t *create_software_info_section(lv_obj_t *menu) {
  lv_obj_t *sub_software_info_page =
      lv_menu_page_create(menu, "Software info.");
  lv_obj_set_style_pad_hor(
      sub_software_info_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  auto section = lv_menu_section_create(sub_software_info_page);

  auto app_version = esp_app_get_description();
  create_text_fmt(section, nullptr, "Version: %s", app_version->version);
  create_text_fmt(section, nullptr, "Compiled at: %s %s", app_version->date,
                  app_version->time);
  create_text_fmt(section, nullptr, "IDF Version: %s", esp_get_idf_version());
  return sub_software_info_page;
}

static lv_obj_t *create_hardware_info_section(lv_obj_t *menu) {
  lv_obj_t *sub_hardware_info_page =
      lv_menu_page_create(menu, "Hardware info.");
  lv_obj_set_style_pad_hor(
      sub_hardware_info_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  auto section = lv_menu_section_create(sub_hardware_info_page);

  ChipInfo info;
  auto err = get_chip_info(info);
  if (err == ESP_OK) {
    create_text_fmt(section, nullptr, "Chip model: %s", info.model);
    create_text_fmt(section, nullptr, "Cores: %d", info.cores);
    create_text(section, nullptr, "Features: ");
    create_text_fmt(section, nullptr, "%s%s%s%s%" PRIu32 "%s\r\n",
                    info.wifi ? "/802.11bgn" : "", info.ble ? "/BLE" : "",
                    info.bt ? "/BT" : "",
                    info.emb_flash ? "/Embedded-Flash:" : "/External-Flash:",
                    info.flash_size, " MB");
    create_text_fmt(section, nullptr, "Revision: %d", info.revision);
  } else {
    create_text(section, nullptr, "Unknown hardware!");
  }

  return sub_hardware_info_page;
}

static lv_obj_t *create_legal_info_section(lv_obj_t *menu) {
  lv_obj_t *sub_legal_info_page = lv_menu_page_create(menu, "Legal info.");
  lv_obj_set_style_pad_hor(
      sub_legal_info_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  auto section = lv_menu_section_create(sub_legal_info_page);
  create_text(section, NULL, "");
  return sub_legal_info_page;
}

lv_obj_t *create_about_section(lv_obj_t *menu) {
  lv_obj_t *sub_software_info_page = create_software_info_section(menu);
  lv_obj_t *sub_hardware_info_page = create_hardware_info_section(menu);
  lv_obj_t *sub_legal_info_page = create_legal_info_section(menu);

  lv_obj_t *sub_about_page = lv_menu_page_create(menu, "About");
  lv_obj_set_style_pad_hor(
      sub_about_page,
      lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), LV_PART_MAIN),
      0);
  lv_menu_separator_create(sub_about_page);
  auto section = lv_menu_section_create(sub_about_page);
  auto cont = create_text(section, NULL, "Software information");
  lv_menu_set_load_page_event(menu, cont, sub_software_info_page);
  cont = create_text(section, NULL, "Hardware information");
  lv_menu_set_load_page_event(menu, cont, sub_hardware_info_page);
  cont = create_text(section, NULL, "Legal information");
  lv_menu_set_load_page_event(menu, cont, sub_legal_info_page);

  return sub_about_page;
}

}
