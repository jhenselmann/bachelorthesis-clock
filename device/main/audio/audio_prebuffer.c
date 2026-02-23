#include "audio_prebuffer.h"
#include "websocket_stream.h"
#include "utils/thread_utils.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "AUDIO_PREBUF";

static uint8_t *buffer = NULL;
static size_t buffer_size = 0;
static size_t write_pos = 0;
static bool is_active = false;
static SemaphoreHandle_t mutex = NULL;

esp_err_t audio_prebuffer_init(void)
{
    if (buffer != NULL) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    // Allocate in PSRAM for large buffer
    buffer = heap_caps_malloc(AUDIO_PREBUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (buffer) {
        buffer_size = AUDIO_PREBUFFER_SIZE;
    } else {
        ESP_LOGE(TAG, "Failed to allocate %d bytes in PSRAM", AUDIO_PREBUFFER_SIZE);
        // Fallback to internal RAM (smaller)
        buffer_size = AUDIO_PREBUFFER_SIZE / 4;  // 80KB fallback
        buffer = malloc(buffer_size);
        if (!buffer) {
            ESP_LOGE(TAG, "Failed to allocate prebuffer");
            buffer_size = 0;
            return ESP_ERR_NO_MEM;
        }
        ESP_LOGW(TAG, "Using reduced prebuffer size (internal RAM): %zu KB", buffer_size / 1024);
    }

    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
        free(buffer);
        buffer = NULL;
        buffer_size = 0;
        return ESP_ERR_NO_MEM;
    }

    write_pos = 0;
    is_active = false;

    ESP_LOGI(TAG, "Prebuffer initialized: %zu KB", buffer_size / 1024);
    return ESP_OK;
}

void audio_prebuffer_start(void)
{
    if (!buffer || !mutex) {
        ESP_LOGE(TAG, "Not initialized");
        return;
    }

    MUTEX_TAKE(mutex);
    write_pos = 0;
    is_active = true;
    MUTEX_GIVE(mutex);

    ESP_LOGD(TAG, "Prebuffering started");
}

void audio_prebuffer_stop(void)
{
    if (!mutex) return;

    MUTEX_TAKE(mutex);
    is_active = false;
    MUTEX_GIVE(mutex);

    ESP_LOGD(TAG, "Prebuffering stopped (data retained: %zu bytes)", write_pos);
}

esp_err_t audio_prebuffer_write(const int16_t *data, size_t length)
{
    if (!buffer || !mutex || !data || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    MUTEX_TAKE(mutex);

    if (!is_active) {
        MUTEX_GIVE(mutex);
        return ESP_OK;  // Not buffering, silently ignore
    }

    // Check if there's space
    if (write_pos + length > buffer_size) {
        // Buffer full - drop oldest data by shifting (simple approach)
        size_t drop_size = buffer_size / 4;

        // Guard against underflow: ensure we have enough data to drop
        if (write_pos <= drop_size) {
            // Buffer nearly empty but chunk too large - shouldn't happen normally
            ESP_LOGW(TAG, "Prebuffer: chunk too large for available space");
            MUTEX_GIVE(mutex);
            return ESP_ERR_NO_MEM;
        }

        ESP_LOGW(TAG, "Prebuffer full, dropping oldest %zu bytes", drop_size);
        size_t keep_size = write_pos - drop_size;
        memmove(buffer, buffer + drop_size, keep_size);
        write_pos = keep_size;
    }

    // Write new data
    memcpy(buffer + write_pos, data, length);
    write_pos += length;

    MUTEX_GIVE(mutex);
    return ESP_OK;
}

esp_err_t audio_prebuffer_flush(void)
{
    if (!buffer || !mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    MUTEX_TAKE(mutex);

    if (write_pos == 0) {
        MUTEX_GIVE(mutex);
        ESP_LOGD(TAG, "Nothing to flush");
        return ESP_OK;
    }

    size_t total_bytes = write_pos;
    ESP_LOGI(TAG, "Flushing prebuffer: %zu bytes (%.1f seconds)",
             total_bytes, (float)total_bytes / (16000 * 2));

    // Send in chunks to avoid blocking too long
    const size_t chunk_size = 1024;  // 512 samples
    size_t sent = 0;

    while (sent < total_bytes) {
        size_t to_send = (total_bytes - sent > chunk_size) ? chunk_size : (total_bytes - sent);

        esp_err_t err = websocket_stream_send_audio((int16_t *)(buffer + sent), to_send);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to send prebuffer chunk at offset %zu", sent);
            // Continue anyway, don't block on errors
        }

        sent += to_send;

        // Small delay to avoid overwhelming WebSocket
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Clear buffer
    write_pos = 0;
    is_active = false;

    MUTEX_GIVE(mutex);

    ESP_LOGI(TAG, "Prebuffer flushed successfully");
    return ESP_OK;
}

void audio_prebuffer_clear(void)
{
    if (!mutex) return;

    MUTEX_TAKE(mutex);
    write_pos = 0;
    is_active = false;
    MUTEX_GIVE(mutex);

    ESP_LOGI(TAG, "Prebuffer cleared");
}

bool audio_prebuffer_is_active(void)
{
    if (!mutex) return false;

    MUTEX_TAKE(mutex);
    bool active = is_active;
    MUTEX_GIVE(mutex);

    return active;
}

size_t audio_prebuffer_get_level(void)
{
    if (!mutex) return 0;

    MUTEX_TAKE(mutex);
    size_t level = write_pos;
    MUTEX_GIVE(mutex);

    return level;
}
