#include "wakeword.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "codec_driver.h"
#include "conversation_controller.h"
#include "audio_collector.h"
#include "audio_resampler.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "model_path.h"

static const char *TAG = "WAKEWORD";

static esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data = NULL;

// State flags
static volatile bool tasks_running = false;
static volatile bool paused = false;
static volatile bool manual_trigger_pending = false;

static void feed_task(void *arg)
{
    esp_afe_sr_data_t *afe = arg;
    int chunksize = afe_handle->get_feed_chunksize(afe);
    int nch = afe_handle->get_feed_channel_num(afe);
    assert(nch == 2);

    const float ratio = 44100.0f / 16000.0f;
    size_t i2s_chunksize = (size_t)(chunksize * ratio + 0.5f);

    int16_t *buf_44k = malloc(i2s_chunksize * sizeof(int16_t) * 2);
    int16_t *buf_16k = malloc(chunksize * sizeof(int16_t) * 2);
    assert(buf_44k && buf_16k);

    while (tasks_running) {
        size_t bytes_read = 0;
        esp_err_t err = codec_driver_read(buf_44k, i2s_chunksize * sizeof(int16_t) * 2, &bytes_read, portMAX_DELAY);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "I2S read failed");
            continue;
        }

        audio_resample_44k_to_16k(buf_44k, buf_16k, i2s_chunksize);
        afe_handle->feed(afe, buf_16k);
    }

    free(buf_44k);
    free(buf_16k);
    vTaskDelete(NULL);
}

static void detect_task(void *arg)
{
    esp_afe_sr_data_t *afe = arg;
    int chunksize = afe_handle->get_fetch_chunksize(afe);

    while (tasks_running) {
        afe_fetch_result_t *res = afe_handle->fetch(afe);
        if (!res || res->ret_value == ESP_FAIL) {
            ESP_LOGE(TAG, "Fetch error");
            break;
        }

        if (audio_collector_is_active() && res->data) {
            audio_collector_feed(res->data, chunksize * sizeof(int16_t));
            continue;
        }

        bool manual = manual_trigger_pending;
        if (manual) {
            manual_trigger_pending = false;
        }

        if (res->wakeup_state == WAKENET_DETECTED || manual) {
            if (paused) {
                continue;
            }

            if (manual) {
                ESP_LOGI(TAG, "Manual trigger");
            } else {
                ESP_LOGI(TAG, "Wake word detected");
            }

            esp_err_t err = conversation_controller_start();
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to start conversation");
            }
        }
    }

    vTaskDelete(NULL);
}

void wakeword_init(void)
{
    ESP_ERROR_CHECK(codec_driver_init());
    ESP_ERROR_CHECK(codec_driver_set_sample_rate(44100, 16, 2));

    srmodel_list_t *models = esp_srmodel_init("model");
    afe_config_t *afe_config = afe_config_init("MM", models, AFE_TYPE_SR, AFE_MODE_LOW_COST);

    afe_handle = esp_afe_handle_from_config(afe_config);
    afe_data = afe_handle->create_from_config(afe_config);

    afe_config_free(afe_config);
}

void wakeword_start(void)
{
    if (tasks_running) {
        return;
    }

    ESP_LOGI(TAG, "Starting detection");

    tasks_running = true;
    xTaskCreatePinnedToCore(&feed_task, "feed", 16 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_task, "detect", 8 * 1024, (void*)afe_data, 5, NULL, 1);
}

void wakeword_trigger_manual(void)
{
    if (paused || audio_collector_is_active() || manual_trigger_pending) {
        return;
    }
    manual_trigger_pending = true;
}

void wakeword_pause(void)
{
    paused = true;
}

void wakeword_resume(void)
{
    paused = false;
}
