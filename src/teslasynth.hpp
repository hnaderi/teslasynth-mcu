#pragma once

#include "freertos/FreeRTOS.h"

StreamBufferHandle_t init_ble_midi();
void init_synth(StreamBufferHandle_t sbuf);
void init_gui(void);
void init_cli(void);
