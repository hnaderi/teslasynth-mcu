#include "esp_console.h"
#include <stdio.h>
#include <string.h>

namespace teslasynth::app::cli {
extern void register_configuration_commands(void);
extern void register_system_common(void);

void init(void) {
  esp_console_repl_t *repl = NULL;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  repl_config.prompt = "teslasynth>";
  repl_config.max_cmdline_length = 1024;

  /* Register commands */
  esp_console_register_help_command();
  register_system_common();
  register_configuration_commands();

  esp_console_dev_uart_config_t hw_config =
      ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

  ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

} // namespace teslasynth::app::cli
