#include "core.hpp"
#include "esp_console.h"
#include "freertos/task.h"
#include "notes.hpp"
#include "synth_config.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <string.h>

static const char *TAG = "configuration_cmd";

namespace keys {
static constexpr const char *min_on_time = "min-on-time";
static constexpr const char *max_on_time = "max-on-time";
static constexpr const char *min_deadtime = "min-deadtime";
static constexpr const char *tuning = "tuning";
static constexpr const char *notes = "notes";
}; // namespace keys

static bool parse_duration(const char *s, Duration32 *out) {
  char *end;
  long long val = strtoll(s, &end, 0);
  if (end == s)
    return false; // no digits

  if (*end == '\0' || strcmp(end, "us") == 0) {
    *out = Duration32::micros(val);
    return true;
  }
  if (strcmp(end, "ms") == 0) {
    *out = Duration32::millis(val);
    return true;
  }
  if (strcmp(end, "s") == 0) {
    *out = Duration32::seconds(val);
    return true;
  }

  return false; // unknown suffix
}

static bool parse_hertz(const char *s, Hertz *out) {
  char *end;
  float val = strtof(s, &end);
  if (end == s)
    return false;

  if (*end == '\0' || strcmp(end, "Hz") == 0) {
    *out = Hertz(val);
    return true;
  }
  return false;
}

static bool parse_notes(const char *s, uint8_t *out) {
  char *end;
  unsigned long val = strtoul(s, &end, 0);
  if (end == s)
    return false;

  if (*end == '\0' && val <= Config::max_notes && val >= 1) {
    *out = val;
    return true;
  }
  return false;
}

inline int invalid_duration(const char *value) {
  printf("Invalid duration value %s, valid values unsigned integer values "
         "followed by an optional time unit [us (default), ms, s]",
         value);
  return 1;
}

inline int invalid_frequency(const char *value) {
  printf("Invalid frequency value %s\n"
         "Valid values are floating point numbers followed by an optional unit "
         "[Hz]",
         value);
  return 1;
}

void register_configuration_commands(void);

#define cstr(value) std::string(value).c_str()

static int print_config() {
  const Config &config = get_config();
  printf("Configuration:\n"
         "\t%s = %u\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = %s\n",
         keys::notes, config.notes, keys::min_on_time, cstr(config.min_on_time),
         keys::max_on_time, cstr(config.max_on_time), keys::min_deadtime,
         cstr(config.min_deadtime), keys::tuning, cstr(config.a440));
  return 0;
}

#define read_duration(out)                                                     \
  if (!parse_duration(value, out)) {                                           \
    return invalid_duration(value);                                            \
  }

static int set_config(int argc, char **argv) {
  Config &config = get_config();
  for (int i = 0; i < argc; i++) {
    char *eq = strchr(argv[i], '=');
    if (!eq) {
      printf("Missing '=' in argument: %s\n", argv[i]);
      return 1;
    }
    *eq = 0;
    char *key = argv[i], *value = eq + 1;

    if (strcmp(key, keys::min_on_time) == 0) {
      read_duration(&config.min_on_time);
    } else if (strcmp(key, keys::max_on_time) == 0) {
      read_duration(&config.max_on_time);
    } else if (strcmp(key, keys::min_deadtime) == 0) {
      read_duration(&config.min_deadtime);
    } else if (strcmp(key, keys::tuning) == 0) {
      if (!parse_hertz(value, &config.a440)) {
        return invalid_frequency(value);
      }
    } else if (strcmp(key, keys::notes) == 0) {
      if (!parse_notes(value, &config.notes)) {
        printf("Invalid notes value %s, must be a number in [1, %i]", value,
               Config::max_notes);
        return 1;
      }
    } else {
      printf("Unknown config: %s", key);
      return 1;
    }
  }

  save_config();
  return 0;
}

static int config_cmd(int argc, char **argv) {
  if (argc > 2 && strcmp(argv[1], "set") == 0) {
    return set_config(argc - 2, argv + 2);
  }
  if (argc == 2 && strcmp(argv[1], "show") == 0) {
    print_config();
    return 0;
  }
  if (argc == 2 && strcmp(argv[1], "reset") == 0) {
    reset_config();
    print_config();
    return 0;
  }
  printf("Usage: config <set|show|reset>\n");
  return 0;
}

void register_configuration_commands(void) {
  const esp_console_cmd_t cfg_cmd = {
      .command = "config",
      .help = "Configuration commands",
      .hint = "set <key1>=<val1> [<key2>=<val2> …] | show", // <-- colored hint
      .func = config_cmd,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cfg_cmd));
}
