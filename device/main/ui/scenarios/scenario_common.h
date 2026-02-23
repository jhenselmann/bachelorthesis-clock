// Shared utilities for scenarios B and C
#pragma once

#include "lvgl.h"
#include <stdint.h>

#define SCENARIO_ICON_Y_OFFSET 120

lv_obj_t* scenario_create_mic_icon(void);
lv_obj_t* scenario_create_spinner(void);
lv_obj_t* scenario_create_error_icon(void);

void scenario_display_alarm_list(lv_obj_t **label_array, int max_labels, int y_offset);
void scenario_display_single_alarm(uint8_t hour, uint8_t minute,
                                   lv_obj_t **title_label, lv_obj_t **time_label);
void scenario_display_no_alarms(lv_obj_t **label);

void scenario_clear_obj(lv_obj_t **obj);
