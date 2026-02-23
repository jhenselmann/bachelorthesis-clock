#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

// Buffers audio during WebSocket connection setup (~10 seconds)

#define AUDIO_PREBUFFER_SIZE (320 * 1024)

esp_err_t audio_prebuffer_init(void);
void audio_prebuffer_start(void);
void audio_prebuffer_stop(void);
esp_err_t audio_prebuffer_write(const int16_t *data, size_t length);
esp_err_t audio_prebuffer_flush(void);
void audio_prebuffer_clear(void);
bool audio_prebuffer_is_active(void);
size_t audio_prebuffer_get_level(void);
