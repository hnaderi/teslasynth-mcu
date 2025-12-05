#include "application.hpp"
#include "configuration/synth.hpp"
#include "core.hpp"
#include "esp_console.h"
#include "freertos/task.h"
#include "instruments.hpp"
#include "midi_synth.hpp"
#include "notes.hpp"
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdio.h>
#include <string.h>

namespace teslasynth::app::cli {
using namespace synth;
using namespace app::configuration;

namespace keys {
static constexpr const char *max_on_time = "max-on-time";
static constexpr const char *min_deadtime = "min-deadtime";
static constexpr const char *max_duty = "max-duty";
static constexpr const char *duty_window = "duty-window";
static constexpr const char *tuning = "tuning";
static constexpr const char *notes = "notes";
static constexpr const char *instrument = "instrument";
}; // namespace keys

static bool parse_duration(const char *s, Duration16 *out) {
  char *end;
  auto val = strtoul(s, &end, 0);
  if (end == s)
    return false; // no digits

  if (*end == '\0' || strcmp(end, "us") == 0) {
    *out = Duration16::micros(val);
    return true;
  }
  if (strcmp(end, "ms") == 0) {
    *out = Duration16::millis(val);
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

static bool parse_instrument(const char *s, std::optional<uint8_t> *out) {
  char *end;
  long val = strtol(s, &end, 0);
  size_t len = strlen(s);
  if (end == s && len > 0) {
    return false;
  }

  if (val < 0 || len == 0) {
    *out = {};
    return true;
  } else if (*end == '\0' && static_cast<size_t>(val) < instruments_size) {
    *out = val;
    return true;
  }
  return false;
}

inline int invalid_duration(const char *value) {
  printf("Invalid duration value: %s\n"
         "Valid values unsigned integer values "
         "followed by an optional time unit [us (default), ms]\n",
         value);
  return 1;
}

inline int invalid_frequency(const char *value) {
  printf("Invalid frequency value: %s\n"
         "Valid values are floating point numbers followed by an optional unit "
         "[Hz]\n",
         value);
  return 1;
}

inline int invalid_instrument(const char *value) {
  printf("Invalid instrument value: %s\n"
         "Valid values are optional integer numbers, negative values are "
         "considered as no value. Max allowed value is %du\n",
         value, instruments_size);
  return 1;
}

void register_configuration_commands(UIHandle *handle);
static UIHandle handle_;

#define cstr(value) std::string(value).c_str()
#define instrument_value(config)                                               \
  (config.instrument.has_value() ? std::to_string(*config.instrument).c_str()  \
                                 : "")

static void print_channel_config(uint8_t nr, const Config &config) {
  printf("Channel[%u] configuration:\n"
         "\t%s = %u\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = %s\n"
         "\t%s = <%s>\n",
         nr + 1, keys::notes, config.notes, keys::max_on_time,
         cstr(config.max_on_time), keys::min_deadtime,
         cstr(config.min_deadtime), keys::max_duty, cstr(config.max_duty),
         keys::duty_window, cstr(config.duty_window), keys::instrument,
         instrument_value(config));
}

static int print_config() {
  auto config = handle_.config_read();
  printf("Synth configuration:\n"
         "\t%s = %s\n"
         "\t%s = <%s>\n",
         keys::tuning, cstr(config.synth().a440), keys::instrument,
         instrument_value(config.synth()));

  for (auto i = 0; i < config.channels_size(); i++) {
    print_channel_config(i, config.channel(i));
  }

  return 0;
}

#define read_duration(out)                                                     \
  if (!parse_duration(value, out)) {                                           \
    return invalid_duration(value);                                            \
  }

static int set_config(int argc, char **argv) {
  Config config;
  for (int i = 0; i < argc; i++) {
    char *eq = strchr(argv[i], '=');
    if (!eq) {
      printf("Missing '=' in argument: %s\n", argv[i]);
      return 1;
    }
    *eq = 0;
    char *key = argv[i], *value = eq + 1;

    if (strcmp(key, keys::max_on_time) == 0) {
      read_duration(&config.max_on_time);
    } else if (strcmp(key, keys::min_deadtime) == 0) {
      read_duration(&config.min_deadtime);
    } else if (strcmp(key, keys::notes) == 0) {
      if (!parse_notes(value, &config.notes)) {
        printf("Invalid notes value %s, must be a number in [1, %i]", value,
               Config::max_notes);
        return 1;
      }
    } else if (strcmp(key, keys::instrument) == 0) {
      if (!parse_instrument(value, &config.instrument)) {
        return invalid_instrument(value);
      }
    } else {
      printf("Unknown config: %s", key);
      return 1;
    }
  }

  // update_config(config);
  // save_config();
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
    handle_.config_set(AppConfig{});
    print_config();
    return 0;
  }
  printf("Usage: config <set|show|reset>\n");
  return 0;
}

void register_configuration_commands(UIHandle handle) {
  // TODO refactor
  handle_ = handle;
  const esp_console_cmd_t cfg_cmd = {
      .command = "config",
      .help = "Configuration commands",
      .hint = "set <key1>=<val1> [<key2>=<val2> …] | show | reset",
      .func = config_cmd,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cfg_cmd));
}

} // namespace teslasynth::app::cli
