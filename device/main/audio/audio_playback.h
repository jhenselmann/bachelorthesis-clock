#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

esp_err_t audio_playback_init(void);
esp_err_t play_mp3_file(const uint8_t *mp3_data, size_t mp3_size);
esp_err_t audio_playback_stop(void);
bool audio_playback_is_playing(void);

// For streaming server responses
void audio_playback_on_chunk(const uint8_t *data, size_t length);
esp_err_t audio_playback_play_buffered(void);
