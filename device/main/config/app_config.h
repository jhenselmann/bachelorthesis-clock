#pragma once

// WiFi
#define APP_WIFI_SSID "iPhone von Johannes"
#define APP_WIFI_PASSWORD "Johannes2210?"
#define APP_WIFI_MAX_RETRIES 5

// WebSocket
#define APP_WS_URL "wss://smuhr.tailbcecf4.ts.net/voice-ws"
#define APP_WS_AUTH_TOKEN "dG9XdZc3ttZtX6ZvuKjT"
#define APP_WS_RECONNECT_TIMEOUT_MS 5000
#define APP_WS_NETWORK_TIMEOUT_MS 10000
#define APP_WS_PING_INTERVAL_SEC 30

// VAD Configuration
#define APP_VAD_INITIAL_TIMEOUT_MS 5000
#define APP_VAD_SILENCE_DURATION_MS 2000
#define APP_VAD_THRESHOLD 150

// UI Scenario
#define APP_UI_SCENARIO_A 0
#define APP_UI_SCENARIO_B 1
#define APP_UI_SCENARIO APP_UI_SCENARIO_B
