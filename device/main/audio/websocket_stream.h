#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    const char *ws_url;
    const char *auth_token;
    const char *language;
    int sample_rate;
} websocket_stream_config_t;

typedef void (*ws_audio_callback_t)(const uint8_t *data, size_t len);
typedef void (*ws_response_callback_t)(bool success);
typedef void (*ws_transcript_callback_t)(const char *text);
typedef void (*ws_action_callback_t)(const char *action, const char *entities_json);

void websocket_stream_init(const websocket_stream_config_t *config);
void websocket_stream_set_callbacks(ws_audio_callback_t audio, ws_response_callback_t response,
                                    ws_transcript_callback_t transcript, ws_action_callback_t action);

esp_err_t websocket_stream_connect(void);
void websocket_stream_disconnect(void);
bool websocket_stream_is_connected(void);

esp_err_t websocket_stream_start_recording(void);
esp_err_t websocket_stream_stop_recording(void);
esp_err_t websocket_stream_send_audio(const uint8_t *data, size_t len);
void websocket_stream_send_complete(bool success);
