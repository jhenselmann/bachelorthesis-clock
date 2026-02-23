#include "audio_playback.h"
#include "codec_driver.h"
#include "conversation_controller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>
#include "mp3dec.h"

static const char *TAG = "AUDIO_PLAYBACK";

#define PLAYBACK_BUFFER_SIZE 1024
#define MAX_MP3_BUFFER_SIZE (2048 * 1024)

static bool initialized = false;
static float volume = 0.7f;
static volatile bool playing = false;
static TaskHandle_t playback_task_handle = NULL;

// Streaming buffer for server responses
static uint8_t *mp3_buffer = NULL;
static size_t mp3_pos = 0;
static SemaphoreHandle_t mutex = NULL;

static inline int16_t clamp16(int32_t val)
{
    if (val > 32767) return 32767;
    if (val < -32768) return -32768;
    return (int16_t)val;
}

static void on_playback_complete(void)
{
    conversation_controller_on_playback_complete(true);
}

static void playback_task(void *arg)
{
    int16_t *pcm = (int16_t *)arg;
    size_t total = *(size_t *)((uint8_t *)arg - sizeof(size_t));  // Hack: samples stored before pcm
    size_t pos = 0;

    int16_t *buf = malloc(PLAYBACK_BUFFER_SIZE * 2 * sizeof(int16_t));
    if (!buf) goto cleanup;

    while (playing && pos < total) {
        size_t n = PLAYBACK_BUFFER_SIZE;
        if (pos + n > total) n = total - pos;

        for (size_t i = 0; i < n; i++) {
            buf[i * 2 + 0] = clamp16((int32_t)(pcm[(pos + i) * 2 + 0] * volume));
            buf[i * 2 + 1] = clamp16((int32_t)(pcm[(pos + i) * 2 + 1] * volume));
        }

        size_t written;
        if (codec_driver_write(buf, n * 4, &written, portMAX_DELAY) != ESP_OK) break;
        pos += n;
    }

    free(buf);

cleanup:
    // Free the allocation (which starts at samples count)
    free((uint8_t *)arg - sizeof(size_t));
    playing = false;
    playback_task_handle = NULL;

    on_playback_complete();
    vTaskDelete(NULL);
}

static esp_err_t play_mp3_internal(const uint8_t *mp3_data, size_t mp3_size)
{
    HMP3Decoder decoder = MP3InitDecoder();
    if (!decoder) return ESP_FAIL;

    size_t pcm_buffer_size = mp3_size * 12;
    // Allocate: [size_t samples][pcm data...]
    uint8_t *alloc = heap_caps_malloc(sizeof(size_t) + pcm_buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!alloc) {
        MP3FreeDecoder(decoder);
        return ESP_ERR_NO_MEM;
    }

    int16_t *pcm = (int16_t *)(alloc + sizeof(size_t));
    const uint8_t *ptr = mp3_data;
    size_t left = mp3_size;
    size_t total = 0;

    while (left > 0) {
        int offset = MP3FindSyncWord((unsigned char *)ptr, left);
        if (offset < 0) break;

        ptr += offset;
        left -= offset;

        int err = MP3Decode(decoder, (unsigned char **)&ptr, (int *)&left, pcm + total, 0);
        if (err == ERR_MP3_NONE) {
            MP3FrameInfo info;
            MP3GetLastFrameInfo(decoder, &info);
            total += info.outputSamps;
        } else if (err == ERR_MP3_INDATA_UNDERFLOW) {
            break;
        } else {
            break;
        }

        if ((total * sizeof(int16_t)) >= pcm_buffer_size - 10000) break;
    }

    MP3FreeDecoder(decoder);

    if (total == 0) {
        free(alloc);
        return ESP_FAIL;
    }

    // Store samples count before pcm data
    *(size_t *)alloc = total / 2;
    playing = true;

    if (xTaskCreatePinnedToCore(playback_task, "playback", 4096, pcm, 4, &playback_task_handle, 0) != pdPASS) {
        free(alloc);
        playing = false;
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t audio_playback_init(void)
{
    if (initialized) return ESP_OK;

    mutex = xSemaphoreCreateMutex();
    if (!mutex) return ESP_ERR_NO_MEM;

    initialized = true;
    ESP_LOGI(TAG, "Audio playback initialized");
    return ESP_OK;
}

esp_err_t play_mp3_file(const uint8_t *mp3_data, size_t mp3_size)
{
    if (!initialized || !mp3_data || mp3_size == 0) return ESP_ERR_INVALID_ARG;

    if (playing) {
        audio_playback_stop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    return play_mp3_internal(mp3_data, mp3_size);
}

void audio_playback_on_chunk(const uint8_t *data, size_t length)
{
    if (!initialized || !data || length == 0) return;

    xSemaphoreTake(mutex, portMAX_DELAY);

    if (!mp3_buffer) {
        mp3_buffer = malloc(MAX_MP3_BUFFER_SIZE);
        if (!mp3_buffer) {
            xSemaphoreGive(mutex);
            return;
        }
        mp3_pos = 0;
    }

    if (mp3_pos + length <= MAX_MP3_BUFFER_SIZE) {
        memcpy(mp3_buffer + mp3_pos, data, length);
        mp3_pos += length;
    }

    xSemaphoreGive(mutex);
}

esp_err_t audio_playback_play_buffered(void)
{
    if (!initialized) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(mutex, portMAX_DELAY);

    if (!mp3_buffer || mp3_pos == 0) {
        xSemaphoreGive(mutex);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Playing buffered MP3: %zu bytes", mp3_pos);

    esp_err_t ret = play_mp3_internal(mp3_buffer, mp3_pos);

    free(mp3_buffer);
    mp3_buffer = NULL;
    mp3_pos = 0;

    xSemaphoreGive(mutex);
    return ret;
}

esp_err_t audio_playback_stop(void)
{
    playing = false;
    if (playback_task_handle) vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

bool audio_playback_is_playing(void)
{
    return playing;
}
