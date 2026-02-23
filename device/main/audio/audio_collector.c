#include "audio_collector.h"
#include "vad.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "AUDIO_COLLECTOR";

typedef struct __attribute__((packed)) {
    char riff[4];
    uint32_t file_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
} wav_header_t;

#define WAV_HEADER_SIZE sizeof(wav_header_t)

static uint8_t *buffer = NULL;
static size_t buffer_size = 0;
static size_t write_pos = 0;
static volatile bool is_active = false;
static SemaphoreHandle_t mutex = NULL;
static audio_collector_config_t cfg = {0};

static void write_wav_header(size_t pcm_data_size)
{
    wav_header_t *h = (wav_header_t *)buffer;
    memcpy(h->riff, "RIFF", 4);
    h->file_size = pcm_data_size + WAV_HEADER_SIZE - 8;
    memcpy(h->wave, "WAVE", 4);
    memcpy(h->fmt, "fmt ", 4);
    h->fmt_size = 16;
    h->audio_format = 1;
    h->num_channels = 1;
    h->sample_rate = AUDIO_COLLECTOR_SAMPLE_RATE;
    h->byte_rate = AUDIO_COLLECTOR_SAMPLE_RATE * 2;
    h->block_align = 2;
    h->bits_per_sample = 16;
    memcpy(h->data, "data", 4);
    h->data_size = pcm_data_size;
}

esp_err_t audio_collector_init(const audio_collector_config_t *config)
{
    if (buffer) return ESP_OK;

    if (config) cfg = *config;

    buffer_size = WAV_HEADER_SIZE + AUDIO_COLLECTOR_BUFFER_SIZE;
    buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    if (!buffer) return ESP_ERR_NO_MEM;

    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
        heap_caps_free(buffer);
        buffer = NULL;
        return ESP_ERR_NO_MEM;
    }

    vad_init();
    ESP_LOGI(TAG, "Audio collector initialized: %zu KB buffer (max %d sec)",
             buffer_size / 1024, AUDIO_COLLECTOR_MAX_SECONDS);
    return ESP_OK;
}

esp_err_t audio_collector_start(void)
{
    if (!buffer) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(mutex, portMAX_DELAY);
    write_pos = 0;
    is_active = true;
    vad_reset();
    xSemaphoreGive(mutex);

    ESP_LOGI(TAG, "Collection started");
    return ESP_OK;
}

esp_err_t audio_collector_stop(void)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    is_active = false;
    xSemaphoreGive(mutex);
    return ESP_OK;
}

bool audio_collector_is_active(void)
{
    return is_active;
}

void audio_collector_feed(const int16_t *audio_data, size_t length)
{
    if (!is_active || !audio_data || length == 0) return;

    xSemaphoreTake(mutex, portMAX_DELAY);

    if (!is_active) {
        xSemaphoreGive(mutex);
        return;
    }

    size_t max_pcm_size = buffer_size - WAV_HEADER_SIZE;
    if (write_pos + length > max_pcm_size) {
        is_active = false;
        xSemaphoreGive(mutex);
        if (cfg.done_callback) cfg.done_callback();
        return;
    }

    memcpy(buffer + WAV_HEADER_SIZE + write_pos, audio_data, length);
    write_pos += length;

    stream_vad_state_t vad_state = vad_process_chunk(audio_data, length / sizeof(int16_t));

    if (vad_state == STREAM_VAD_SILENCE_DETECTED || vad_state == STREAM_VAD_TIMEOUT) {
        bool is_timeout = (vad_state == STREAM_VAD_TIMEOUT);
        is_active = false;
        xSemaphoreGive(mutex);

        if (is_timeout) {
            if (cfg.timeout_callback) cfg.timeout_callback();
        } else {
            if (cfg.done_callback) cfg.done_callback();
        }
        return;
    }

    xSemaphoreGive(mutex);
}

esp_err_t audio_collector_get_wav(const uint8_t **out_data, size_t *out_length)
{
    if (!out_data || !out_length) return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(mutex, portMAX_DELAY);
    if (write_pos == 0) {
        xSemaphoreGive(mutex);
        return ESP_ERR_NOT_FOUND;
    }

    write_wav_header(write_pos);
    *out_data = buffer;
    *out_length = WAV_HEADER_SIZE + write_pos;
    xSemaphoreGive(mutex);

    ESP_LOGI(TAG, "WAV ready: %zu bytes", *out_length);
    return ESP_OK;
}

uint32_t audio_collector_get_duration_ms(void)
{
    return (write_pos * 1000) / (AUDIO_COLLECTOR_SAMPLE_RATE * sizeof(int16_t));
}

void audio_collector_clear(void)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    write_pos = 0;
    is_active = false;
    xSemaphoreGive(mutex);
}
