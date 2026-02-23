#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define ALARM_MAX_COUNT 10

typedef struct {
    uint8_t hour;
    uint8_t minute;
    bool enabled;
    bool triggered;
} alarm_entry_t;

typedef void (*alarm_trigger_callback_t)(int alarm_index);

// Core functions
esp_err_t alarm_manager_init(void);
esp_err_t alarm_manager_start(void);  // Start timer and register callbacks
int alarm_manager_add(uint8_t hour, uint8_t minute);
esp_err_t alarm_manager_remove(int index);
esp_err_t alarm_manager_remove_all(void);
const alarm_entry_t* alarm_manager_get_all(int *count);
void alarm_manager_register_callback(alarm_trigger_callback_t callback);
void alarm_manager_check(void);
void alarm_manager_dismiss(int index);

// Get next N alarms sorted by time (returns count written)
int alarm_manager_get_next_alarms(alarm_entry_t *out_alarms, int max_count);

// UI state (auto-clears when read)
void alarm_manager_set_last_set(uint8_t hour, uint8_t minute);
bool alarm_manager_get_last_set(uint8_t *hour, uint8_t *minute);
void alarm_manager_set_show_list(bool show);
bool alarm_manager_get_show_list(void);
