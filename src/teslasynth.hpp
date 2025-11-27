#pragma once

#include "freertos/idf_additions.h"

namespace teslasynth::app {

namespace devices {

namespace storage {
void init();
}

namespace rmt {
void init(void);
}

namespace ble_midi {
StreamBufferHandle_t init();
}

} // namespace devices

namespace synth {
void init(StreamBufferHandle_t sbuf);
}

namespace gui {
void init(void);
}

namespace cli {
void init(void);
}

} // namespace teslasynth::app
