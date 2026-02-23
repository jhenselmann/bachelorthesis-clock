#include "websocket_stream.h"

#include <string.h>
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "cJSON.h"
#include "alarm_manager.h"

static const char *TAG = "WS_STREAM";

static websocket_stream_config_t config;
static esp_websocket_client_handle_t client;
static SemaphoreHandle_t send_mutex;
static volatile bool connected;
static volatile bool awaiting_response;

static ws_audio_callback_t audio_cb;
static ws_response_callback_t response_cb;
static ws_transcript_callback_t transcript_cb;
static ws_action_callback_t action_cb;

static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data);
static void handle_text(const char *data, int len);
static esp_err_t send_json(const char *type, cJSON *payload);

void websocket_stream_init(const websocket_stream_config_t *cfg)
{
    config = *cfg;
    if (!send_mutex) {
        send_mutex = xSemaphoreCreateMutex();
    }
}

void websocket_stream_set_callbacks(ws_audio_callback_t audio, ws_response_callback_t response, ws_transcript_callback_t transcript, ws_action_callback_t action)
{
    audio_cb = audio;
    response_cb = response;
    transcript_cb = transcript;
    action_cb = action;
}

esp_err_t websocket_stream_connect(void)
{
    if (connected) return ESP_OK;

    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s\r\n", config.auth_token);

    esp_websocket_client_config_t ws_cfg = {
        .uri = config.ws_url,
        .headers = auth_header,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .reconnect_timeout_ms = 5000,
        .network_timeout_ms = 10000,
    };

    client = esp_websocket_client_init(&ws_cfg);
    if (!client) return ESP_FAIL;

    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, event_handler, NULL);
    return esp_websocket_client_start(client);
}

void websocket_stream_disconnect(void)
{
    awaiting_response = false;
    if (client) {
        esp_websocket_client_stop(client);
        esp_websocket_client_destroy(client);
        client = NULL;
    }
    connected = false;
}

bool websocket_stream_is_connected(void)
{
    return connected;
}

esp_err_t websocket_stream_start_recording(void)
{
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "language", config.language);
    cJSON_AddNumberToObject(payload, "sampleRate", config.sample_rate);

    alarm_entry_t alarms[ALARM_MAX_COUNT];
    int count = alarm_manager_get_next_alarms(alarms, ALARM_MAX_COUNT);
    cJSON_AddNumberToObject(payload, "alarm_count", count);

    cJSON *arr = cJSON_AddArrayToObject(payload, "alarms");
    for (int i = 0; i < count && i < 3; i++) {
        cJSON *a = cJSON_CreateObject();
        cJSON_AddNumberToObject(a, "hour", alarms[i].hour);
        cJSON_AddNumberToObject(a, "minute", alarms[i].minute);
        cJSON_AddItemToArray(arr, a);
    }

    esp_err_t err = send_json("start_recording", payload);
    cJSON_Delete(payload);
    return err;
}

esp_err_t websocket_stream_stop_recording(void)
{
    awaiting_response = true;
    return send_json("stop_recording", NULL);
}

esp_err_t websocket_stream_send_audio(const uint8_t *data, size_t len)
{
    if (xSemaphoreTake(send_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    const size_t chunk = 4096;
    esp_err_t ret = ESP_OK;

    for (size_t sent = 0; sent < len; sent += chunk) {
        size_t n = (len - sent > chunk) ? chunk : (len - sent);
        if (esp_websocket_client_send_bin(client, (char *)(data + sent), n, pdMS_TO_TICKS(5000)) < 0) {
            ret = ESP_FAIL;
            break;
        }
    }

    xSemaphoreGive(send_mutex);
    return ret;
}

void websocket_stream_send_complete(bool success)
{
    if (!client || !connected) return;

    cJSON *payload = cJSON_CreateObject();
    cJSON_AddBoolToObject(payload, "success", success);
    send_json("client_complete", payload);
    cJSON_Delete(payload);

    vTaskDelay(pdMS_TO_TICKS(100));
    esp_websocket_client_close(client, portMAX_DELAY);
    connected = false;
}

static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    esp_websocket_event_data_t *evt = data;

    switch (id) {
        case WEBSOCKET_EVENT_CONNECTED:
            connected = true;
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            connected = false;
            if (awaiting_response) {
                awaiting_response = false;
                if (response_cb) response_cb(false);
            }
            break;

        case WEBSOCKET_EVENT_DATA:
            if (evt->op_code == 0x01) {
                handle_text(evt->data_ptr, evt->data_len);
            } else if (evt->op_code == 0x02 && audio_cb) {
                audio_cb((uint8_t *)evt->data_ptr, evt->data_len);
            }
            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "WebSocket error");
            break;

        default:
            break;
    }
}

static void handle_text(const char *data, int len)
{
    cJSON *json = cJSON_ParseWithLength(data, len);
    if (!json) return;

    cJSON *type = cJSON_GetObjectItem(json, "type");
    if (!type || !cJSON_IsString(type)) {
        cJSON_Delete(json);
        return;
    }

    const char *t = type->valuestring;

    if (strcmp(t, "transcript") == 0) {
        cJSON *text = cJSON_GetObjectItem(json, "text");
        if (text && cJSON_IsString(text) && transcript_cb) {
            transcript_cb(text->valuestring);
        }
    }
    else if (strcmp(t, "response_start") == 0) {
        cJSON *action = cJSON_GetObjectItem(json, "action");
        cJSON *entities = cJSON_GetObjectItem(json, "entities");

        if (action && cJSON_IsString(action) && action_cb) {
            char *entities_str = entities ? cJSON_PrintUnformatted(entities) : NULL;
            action_cb(action->valuestring, entities_str);
            free(entities_str);
        }
    }
    else if (strcmp(t, "response_end") == 0) {
        awaiting_response = false;
        if (response_cb) response_cb(true);
    }
    else if (strcmp(t, "error") == 0) {
        awaiting_response = false;
        if (response_cb) response_cb(false);
    }

    cJSON_Delete(json);
}

static esp_err_t send_json(const char *type, cJSON *payload)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", type);

    // Copy fields from payload into root
    if (payload) {
        cJSON *item = payload->child;
        while (item) {
            cJSON_AddItemToObject(root, item->string, cJSON_Duplicate(item, true));
            item = item->next;
        }
    }

    char *str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!str) return ESP_ERR_NO_MEM;

    int ret = esp_websocket_client_send_text(client, str, strlen(str), portMAX_DELAY);
    free(str);
    return ret < 0 ? ESP_FAIL : ESP_OK;
}
