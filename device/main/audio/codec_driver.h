#pragma once

#include "esp_err.h"
#include "driver/i2s_std.h"
#include <stddef.h>

esp_err_t codec_driver_init(void);
esp_err_t codec_driver_set_sample_rate(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);
esp_err_t codec_driver_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms);
esp_err_t codec_driver_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);
