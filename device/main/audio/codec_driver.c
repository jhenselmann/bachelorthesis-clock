#include "codec_driver.h"
#include "esp_log.h"
#include "esp_codec_dev.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "CODEC_DRIVER";

static esp_codec_dev_handle_t play_dev_handle = NULL;
static esp_codec_dev_handle_t record_dev_handle = NULL;
static bool is_initialized = false;

esp_err_t codec_driver_init(void)
{
    ESP_LOGI(TAG, "Initializing audio codecs (ES8311 + ES7210)");
    if (is_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    play_dev_handle = bsp_audio_codec_speaker_init();
    record_dev_handle = bsp_audio_codec_microphone_init();
    is_initialized = true;
    esp_err_t ret = codec_driver_set_sample_rate(16000, 16, I2S_SLOT_MODE_STEREO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set default sample rate");
        is_initialized = false;
        return ret;
    }
    ret = esp_codec_dev_set_out_vol(play_dev_handle, 80);
    return ESP_OK;
}

esp_err_t codec_driver_set_sample_rate(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    if (!is_initialized) {
        ESP_LOGE(TAG, "Codec not initialized, call codec_driver_init() first");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Setting sample rate: %lu Hz, %lu-bit, %s", rate, bits_cfg, ch == I2S_SLOT_MODE_MONO ? "Mono" : "Stereo");

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };

    if (play_dev_handle) {
        esp_codec_dev_close(play_dev_handle);
    }

    if (record_dev_handle) {
        esp_codec_dev_close(record_dev_handle);
        esp_codec_dev_set_in_gain(record_dev_handle, 36.0f);
    }

    if (play_dev_handle) {
        esp_codec_dev_open(play_dev_handle, &fs);
    }

    if (record_dev_handle) {
        esp_codec_dev_open(record_dev_handle, &fs);
    }
    return ESP_OK;
}

esp_err_t codec_driver_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
    esp_err_t ret = esp_codec_dev_read(record_dev_handle, audio_buffer, len);
    if (bytes_read) {
        *bytes_read = (ret == ESP_OK) ? len : 0;
    }
    return ret;
}

esp_err_t codec_driver_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    if (bytes_written) {
        *bytes_written = (ret == ESP_OK) ? len : 0;
    }
    return ret;
}
