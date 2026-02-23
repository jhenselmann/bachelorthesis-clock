#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Collects audio into PSRAM buffer, sends as complete WAV

// Max recording time: 30 seconds @ 16kHz mono 16-bit = 960KB
#define AUDIO_COLLECTOR_MAX_SECONDS 30
#define AUDIO_COLLECTOR_SAMPLE_RATE 16000
#define AUDIO_COLLECTOR_BUFFER_SIZE (AUDIO_COLLECTOR_MAX_SECONDS * AUDIO_COLLECTOR_SAMPLE_RATE * sizeof(int16_t))

typedef void (*audio_collector_done_callback_t)(void);
typedef void (*audio_collector_timeout_callback_t)(void);

typedef struct {
    audio_collector_done_callback_t done_callback;
    audio_collector_timeout_callback_t timeout_callback;
} audio_collector_config_t;

esp_err_t audio_collector_init(const audio_collector_config_t *config);
esp_err_t audio_collector_start(void);
esp_err_t audio_collector_stop(void);
bool audio_collector_is_active(void);
void audio_collector_feed(const int16_t *audio_data, size_t length);
esp_err_t audio_collector_get_wav(const uint8_t **out_data, size_t *out_length);
uint32_t audio_collector_get_duration_ms(void);
void audio_collector_clear(void);
