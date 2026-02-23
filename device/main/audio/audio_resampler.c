#include "audio_resampler.h"

// Einfaches Downsampling
size_t audio_resample_44k_to_16k(const int16_t *input_44k, int16_t *output_16k, size_t input_samples)
{
    size_t output_samples = (input_samples * 16000 + 22050) / 44100;

    for (size_t i = 0; i < output_samples; i++) {
        size_t src = i * 44100 / 16000;
        output_16k[i * 2 + 0] = input_44k[src * 2 + 0];
        output_16k[i * 2 + 1] = input_44k[src * 2 + 1];
    }

    return output_samples;
}
