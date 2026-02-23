#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

// Notwendig für Wakeword (nimmt nur 16 kHz)
size_t audio_resample_44k_to_16k(const int16_t *input_44k, int16_t *output_16k, size_t input_samples);
