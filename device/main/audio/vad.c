#include "vad.h"

#include <math.h>
#include "config/app_config.h"
#include "esp_log.h"
#include "esp_timer.h"

static stream_vad_state_t state = STREAM_VAD_WAITING_FOR_SPEECH;
static int64_t session_start = 0;
static int64_t last_speech = 0;
static int chunks_per_sec = 0;

static bool is_speech(const int16_t *audio_data, size_t samples);
static int32_t calculate_rms(const int16_t *audio_data, size_t samples);

void vad_init()
{
    int chunk_duration_ms = 100;
    chunks_per_sec = 1000 / chunk_duration_ms;
}

void vad_reset(void)
{
    state = STREAM_VAD_WAITING_FOR_SPEECH;
    session_start = esp_timer_get_time();
    last_speech = 0;
}

stream_vad_state_t vad_process_chunk(const int16_t *audio_data, size_t samples)
{
    int64_t now = esp_timer_get_time();
    bool has_speech = is_speech(audio_data, samples);

    switch (state) {
        case STREAM_VAD_WAITING_FOR_SPEECH:
            if (has_speech) {
                state = STREAM_VAD_SPEECH_DETECTED;
                last_speech = now;
            } else {
                int64_t elapsed_ms = (now - session_start) / 1000;
                if (elapsed_ms > APP_VAD_INITIAL_TIMEOUT_MS) {
                    state = STREAM_VAD_TIMEOUT;
                }
            }
            break;

        case STREAM_VAD_SPEECH_DETECTED:
            if (has_speech) {
                last_speech = now;
            } else {
                int64_t silence_ms = (now - last_speech) / 1000;
                if (silence_ms > APP_VAD_SILENCE_DURATION_MS) {
                    state = STREAM_VAD_SILENCE_DETECTED;
                }
            }
            break;

        case STREAM_VAD_SILENCE_DETECTED:
        case STREAM_VAD_TIMEOUT:
            break;
    }

    return state;
}

stream_vad_state_t vad_get_state(void)
{
    return state;
}

static bool is_speech(const int16_t *audio_data, size_t samples)
{
    int32_t rms = calculate_rms(audio_data, samples);
    return rms > APP_VAD_THRESHOLD;
}

static int32_t calculate_rms(const int16_t *audio_data, size_t samples)
{
    int64_t sum_squares = 0;

    for (size_t i = 0; i < samples; i++) {
        int32_t sample = audio_data[i];
        sum_squares += sample * sample;
    }

    return (int32_t)sqrt(sum_squares / samples);
}
