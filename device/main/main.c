#include "esp_log.h"

#include "config/app_config.h"
#include "network/wifi.h"
#include "audio/wakeword.h"
#include "audio/websocket_stream.h"
#include "audio/audio_playback.h"
#include "audio/conversation_controller.h"
#include "alarm/alarm_manager.h"
#include "alarm/alarm_playback.h"
#include "ui/ui_manager.h"
#include "ui/settings_ui.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting");

    wakeword_init();
    audio_playback_init();

    wifi_init_and_connect(APP_WIFI_SSID, APP_WIFI_PASSWORD);

    websocket_stream_config_t ws_config = {
        .ws_url = APP_WS_URL,
        .auth_token = APP_WS_AUTH_TOKEN,
        .language = "de",
        .sample_rate = 16000,
    };
    websocket_stream_init(&ws_config);

    ui_manager_init();
    settings_ui_init();

    alarm_manager_init();
    alarm_playback_init();
    alarm_manager_start();

    ui_show_homescreen();
    conversation_controller_init();

    wakeword_start();

    ESP_LOGI(TAG, "System Ready");
}
