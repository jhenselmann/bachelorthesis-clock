#include "conversation_controller.h"

#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "cJSON.h"
#include "websocket_stream.h"
#include "ui_manager.h"
#include "voice_assistant_ui.h"
#include "audio_playback.h"
#include "audio_collector.h"
#include "alarm_manager.h"

static const char *TAG = "CONV_CTRL";

extern const uint8_t ok_mp3_start[] asm("_binary_ok_mp3_start");
extern const uint8_t ok_mp3_end[] asm("_binary_ok_mp3_end");
extern const uint8_t error_mp3_start[] asm("_binary_error_mp3_start");
extern const uint8_t error_mp3_end[] asm("_binary_error_mp3_end");

static conversation_state_t state = CONV_STATE_IDLE;
static TimerHandle_t watchdog_timer = NULL;

#define TIMEOUT_CONNECTING_MS   20000
#define TIMEOUT_PROCESSING_MS   60000
#define TIMEOUT_RESPONSE_MS     120000

static void on_recording_done(void);
static void on_recording_timeout(void);
static void on_audio_received(const uint8_t *data, size_t len);
static void on_response_received(bool success);
static void on_transcript_received(const char *text);
static void on_action_received(const char *action, const char *entities_json);
static void transition_to_state(conversation_state_t new_state);
static void watchdog_timer_callback(TimerHandle_t timer);
static void start_watchdog(uint32_t timeout_ms);
static void stop_watchdog(void);
static void send_audio_task(void *arg);

esp_err_t conversation_controller_init(void)
{
    websocket_stream_set_callbacks(on_audio_received, on_response_received,
                                   on_transcript_received, on_action_received);

    audio_collector_config_t cfg = {
        .done_callback = on_recording_done,
        .timeout_callback = on_recording_timeout,
    };
    esp_err_t err = audio_collector_init(&cfg);
    if (err != ESP_OK) {
        return err;
    }

    watchdog_timer = xTimerCreate("conv_wd", pdMS_TO_TICKS(TIMEOUT_PROCESSING_MS),
                                   pdFALSE, NULL, watchdog_timer_callback);
    if (!watchdog_timer) {
        return ESP_ERR_NO_MEM;
    }

    state = CONV_STATE_IDLE;
    return ESP_OK;
}

esp_err_t conversation_controller_start(void)
{
    if (state != CONV_STATE_IDLE) {
        ESP_LOGW(TAG, "Already active");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = audio_collector_start();
    if (err != ESP_OK) {
        return err;
    }

    transition_to_state(CONV_STATE_LISTENING);
    ui_show_listening();
    return ESP_OK;
}

static void on_recording_done(void)
{
    if (state != CONV_STATE_LISTENING) {
        return;
    }

    ESP_LOGI(TAG, "Recording complete: %lu ms", audio_collector_get_duration_ms());

    size_t mp3_size = ok_mp3_end - ok_mp3_start;
    play_mp3_file(ok_mp3_start, mp3_size);

    transition_to_state(CONV_STATE_CONNECTING);
    ui_show_processing();

    xTaskCreate(send_audio_task, "send_audio", 8192, NULL, 5, NULL);
}

static void on_recording_timeout(void)
{
    conversation_controller_abort("No speech detected");
}

static void send_audio_task(void *arg)
{
    esp_err_t err;

    err = websocket_stream_connect();
    if (err != ESP_OK) {
        conversation_controller_abort("Connection failed");
        vTaskDelete(NULL);
        return;
    }

    // Wait for connection
    for (int i = 0; i < 50 && !websocket_stream_is_connected(); i++) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (!websocket_stream_is_connected()) {
        conversation_controller_abort("Connection timeout");
        vTaskDelete(NULL);
        return;
    }

    err = websocket_stream_start_recording();
    if (err != ESP_OK) {
        conversation_controller_abort("Start recording failed");
        vTaskDelete(NULL);
        return;
    }

    const uint8_t *wav_data;
    size_t wav_length;
    err = audio_collector_get_wav(&wav_data, &wav_length);
    if (err != ESP_OK) {
        conversation_controller_abort("No audio data");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Sending %zu bytes", wav_length);
    err = websocket_stream_send_audio(wav_data, wav_length);
    if (err != ESP_OK) {
        conversation_controller_abort("Send failed");
        vTaskDelete(NULL);
        return;
    }

    websocket_stream_stop_recording();
    audio_collector_clear();

    transition_to_state(CONV_STATE_PROCESSING);
    vTaskDelete(NULL);
}

void conversation_controller_stop_recording(void)
{
    if (state != CONV_STATE_LISTENING) {
        return;
    }
    audio_collector_stop();
    on_recording_done();
}

void conversation_controller_abort(const char *reason)
{
    if (state == CONV_STATE_ERROR || state == CONV_STATE_IDLE) {
        return;
    }

    ESP_LOGW(TAG, "Abort: %s", reason ? reason : "unknown");
    transition_to_state(CONV_STATE_ERROR);

    audio_collector_clear();
    websocket_stream_disconnect();

    size_t mp3_size = error_mp3_end - error_mp3_start;
    play_mp3_file(error_mp3_start, mp3_size);

    ui_show_error(reason);

    // Wait for error sound
    vTaskDelay(pdMS_TO_TICKS(2000));
    for (int i = 0; i < 80 && audio_playback_is_playing(); i++) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    audio_playback_stop();
    voice_assistant_ui_hide();
    ui_show_homescreen();
    transition_to_state(CONV_STATE_IDLE);
}

void conversation_controller_on_playback_complete(bool success)
{
    if (state != CONV_STATE_RESPONSE) {
        return;
    }

    websocket_stream_send_complete(success);
    ui_show_homescreen();
    transition_to_state(CONV_STATE_IDLE);
}

conversation_state_t conversation_controller_get_state(void)
{
    return state;
}

bool conversation_controller_is_active(void)
{
    return state != CONV_STATE_IDLE && state != CONV_STATE_ERROR;
}

static void on_audio_received(const uint8_t *data, size_t len)
{
    audio_playback_on_chunk(data, len);
}

static void on_response_received(bool success)
{
    if (!success) {
        conversation_controller_abort("Server error");
        return;
    }

    transition_to_state(CONV_STATE_RESPONSE);
    ui_show_response();

    esp_err_t ret = audio_playback_play_buffered();
    if (ret != ESP_OK) {
        conversation_controller_abort("Playback failed");
    }
}

static void on_transcript_received(const char *text)
{
    ui_show_transcript(text);
}

static void on_action_received(const char *action, const char *entities_json)
{
    ESP_LOGI(TAG, "Action: %s", action);

    if (strcmp(action, "set_alarm") == 0 && entities_json) {
        cJSON *entities = cJSON_Parse(entities_json);
        if (entities) {
            cJSON *time_obj = cJSON_GetObjectItem(entities, "time");
            if (time_obj) {
                cJSON *hour = cJSON_GetObjectItem(time_obj, "hour");
                cJSON *minute = cJSON_GetObjectItem(time_obj, "minute");
                if (hour && minute) {
                    int h = (int)hour->valuedouble;
                    int m = (int)minute->valuedouble;
                    int idx = alarm_manager_add((uint8_t)h, (uint8_t)m);
                    if (idx >= 0) {
                        ESP_LOGI(TAG, "Alarm set: %02d:%02d", h, m);
                        alarm_manager_set_last_set((uint8_t)h, (uint8_t)m);
                    }
                }
            }
            cJSON_Delete(entities);
        }
    }
    else if (strcmp(action, "get_alarm") == 0) {
        alarm_manager_set_show_list(true);
    }
}

static void transition_to_state(conversation_state_t new_state)
{
    if (new_state == state) {
        return;
    }

    state = new_state;

    switch (new_state) {
        case CONV_STATE_CONNECTING:
            start_watchdog(TIMEOUT_CONNECTING_MS);
            break;
        case CONV_STATE_PROCESSING:
            start_watchdog(TIMEOUT_PROCESSING_MS);
            break;
        case CONV_STATE_RESPONSE:
            start_watchdog(TIMEOUT_RESPONSE_MS);
            break;
        default:
            stop_watchdog();
            break;
    }
}

static void watchdog_timer_callback(TimerHandle_t timer)
{
    const char *reason = "Timeout";
    switch (state) {
        case CONV_STATE_CONNECTING: reason = "Connection timeout"; break;
        case CONV_STATE_PROCESSING: reason = "Server timeout"; break;
        case CONV_STATE_RESPONSE: reason = "Playback timeout"; break;
        default: break;
    }
    conversation_controller_abort(reason);
}

static void start_watchdog(uint32_t timeout_ms)
{
    if (!watchdog_timer) return;
    xTimerStop(watchdog_timer, 0);
    xTimerChangePeriod(watchdog_timer, pdMS_TO_TICKS(timeout_ms), 0);
    xTimerStart(watchdog_timer, 0);
}

static void stop_watchdog(void)
{
    if (watchdog_timer) {
        xTimerStop(watchdog_timer, 0);
    }
}
