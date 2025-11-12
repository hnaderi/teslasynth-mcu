#include "cli.hpp"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include <stdio.h>
#include <string.h>

/*
 * We warn if a secondary serial console is enabled. A secondary serial console
 * is always output-only and hence not very useful for interactive console
 * applications. If you encounter this warning, consider disabling the secondary
 * serial console in menuconfig unless you know what you are doing.
 */
#if SOC_USB_SERIAL_JTAG_SUPPORTED
#if !CONFIG_ESP_CONSOLE_SECONDARY_NONE
#warning                                                                       \
    "A secondary serial console is not useful when using the console component. Please disable it in menuconfig."
#endif
#endif

static const char *TAG = "cli";
#define PROMPT_STR "teslasynth"

/* Console command history can be stored to and loaded from a file.
 * The easiest way to do this is to use FATFS filesystem on top of
 * wear_levelling library.
 */
#if CONFIG_CONSOLE_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem(void) {
  static wl_handle_t wl_handle;
  const esp_vfs_fat_mount_config_t mount_config = {
      .format_if_mount_failed = true,
      .max_files = 4,
  };
  esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage",
                                                   &mount_config, &wl_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
    return;
  }
}
#endif // CONFIG_STORE_HISTORY

void init_cli(void) {
  esp_console_repl_t *repl = NULL;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  /* Prompt to be printed before each line.
   * This can be customized, made dynamic, etc.
   */
  repl_config.prompt = PROMPT_STR ">";
  repl_config.max_cmdline_length = 1024;

#if CONFIG_CONSOLE_STORE_HISTORY
  initialize_filesystem();
  repl_config.history_save_path = HISTORY_PATH;
  ESP_LOGI(TAG, "Command history enabled");
#else
  ESP_LOGI(TAG, "Command history disabled");
#endif

  /* Register commands */
  esp_console_register_help_command();
  register_system_common();

  esp_console_dev_uart_config_t hw_config =
      ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

  ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
