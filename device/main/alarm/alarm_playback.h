#pragma once

#include <stdbool.h>
#include "esp_err.h"

// Plays alarm.mp3 in loop, pauses wakeword

typedef void (*alarm_stopped_callback_t)(void);

esp_err_t alarm_playback_init(void);
esp_err_t alarm_playback_start(void);
esp_err_t alarm_playback_stop(void);
bool alarm_playback_is_active(void);
void alarm_playback_register_stopped_callback(alarm_stopped_callback_t callback);
