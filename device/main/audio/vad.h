#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Voice Activity Detection - detects speech/silence in audio

typedef enum {
    STREAM_VAD_WAITING_FOR_SPEECH,
    STREAM_VAD_SPEECH_DETECTED,
    STREAM_VAD_SILENCE_DETECTED,
    STREAM_VAD_TIMEOUT
} stream_vad_state_t;

void vad_init(void);
void vad_reset(void);
stream_vad_state_t vad_process_chunk(const int16_t *audio_data, size_t samples);
stream_vad_state_t vad_get_state(void);
