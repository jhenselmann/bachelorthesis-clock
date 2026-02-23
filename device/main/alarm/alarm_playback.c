#include "alarm_playback.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "mp3dec.h"

#include "codec_driver.h"
#include "audio_playback.h"
#include "wakeword.h"
#include "conversation_controller.h"

static const char *TAG = "ALARM_PLAY";

#define BUFFER_SIZE 1024
#define TASK_STACK (4 * 1024)

extern const uint8_t alarm_mp3_start[] asm("_binary_alarm_mp3_start");
extern const uint8_t alarm_mp3_end[] asm("_binary_alarm_mp3_end");

static bool initialized = false;
static volatile bool alarm_active = false;
static int16_t *pcm_buffer = NULL;
static size_t pcm_samples = 0;
static alarm_stopped_callback_t stopped_callback = NULL;
static TaskHandle_t alarm_task = NULL;

static esp_err_t decode_alarm_mp3(void);

static void alarm_playback_task(void *arg)
{
    int16_t *pcm = (int16_t *)arg;
    size_t total_frames = pcm_samples / 2;

    int16_t *write_buf = malloc(BUFFER_SIZE * 2 * sizeof(int16_t));
    if (!write_buf) {
        alarm_active = false;
        alarm_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    while (alarm_active) {
        size_t pos = 0;
        while (alarm_active && pos < total_frames) {
            size_t chunk = BUFFER_SIZE;
            if (pos + chunk > total_frames) {
                chunk = total_frames - pos;
            }

            memcpy(write_buf, &pcm[pos * 2], chunk * 2 * sizeof(int16_t));

            size_t written = 0;
            codec_driver_write(write_buf, chunk * 2 * sizeof(int16_t), &written, portMAX_DELAY);
            pos += chunk;
        }
    }

    free(write_buf);
    alarm_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t alarm_playback_init(void)
{
    if (initialized) {
        return ESP_OK;
    }

    esp_err_t err = decode_alarm_mp3();
    if (err != ESP_OK) {
        return err;
    }

    initialized = true;
    return ESP_OK;
}

esp_err_t alarm_playback_start(void)
{
    if (!initialized || alarm_active) {
        return alarm_active ? ESP_OK : ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting alarm");

    wakeword_pause();

    if (conversation_controller_is_active()) {
        conversation_controller_abort("Alarm triggered");
    }

    audio_playback_stop();

    alarm_active = true;

    BaseType_t ret = xTaskCreatePinnedToCore(alarm_playback_task, "alarm",
        TASK_STACK, pcm_buffer, 4, &alarm_task, 0);

    if (ret != pdPASS) {
        alarm_active = false;
        wakeword_resume();
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t alarm_playback_stop(void)
{
    if (!alarm_active) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping alarm");

    alarm_active = false;

    if (alarm_task) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    wakeword_resume();

    if (stopped_callback) {
        stopped_callback();
    }

    return ESP_OK;
}

bool alarm_playback_is_active(void)
{
    return alarm_active;
}

void alarm_playback_register_stopped_callback(alarm_stopped_callback_t callback)
{
    stopped_callback = callback;
}

static esp_err_t decode_alarm_mp3(void)
{
    size_t mp3_size = alarm_mp3_end - alarm_mp3_start;

    HMP3Decoder decoder = MP3InitDecoder();
    if (!decoder) {
        return ESP_FAIL;
    }

    size_t buf_size = mp3_size * 12;
    pcm_buffer = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!pcm_buffer) {
        MP3FreeDecoder(decoder);
        return ESP_ERR_NO_MEM;
    }

    const uint8_t *ptr = alarm_mp3_start;
    size_t left = mp3_size;
    pcm_samples = 0;

    while (left > 0) {
        int offset = MP3FindSyncWord((unsigned char *)ptr, left);
        if (offset < 0) break;

        ptr += offset;
        left -= offset;

        int16_t *out = pcm_buffer + pcm_samples;
        int err = MP3Decode(decoder, (unsigned char **)&ptr, (int *)&left, out, 0);

        if (err == ERR_MP3_NONE) {
            MP3FrameInfo info;
            MP3GetLastFrameInfo(decoder, &info);
            pcm_samples += info.outputSamps;
        } else if (err == ERR_MP3_INDATA_UNDERFLOW) {
            break;
        } else {
            break;
        }

        if ((pcm_samples * sizeof(int16_t)) >= buf_size - 10000) {
            break;
        }
    }

    MP3FreeDecoder(decoder);

    if (pcm_samples == 0) {
        free(pcm_buffer);
        pcm_buffer = NULL;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Decoded %zu samples", pcm_samples);
    return ESP_OK;
}
