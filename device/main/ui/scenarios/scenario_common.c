#include "scenario_common.h"
#include "ui_constants.h"
#include "mic_icon.h"
#include "error_icon.h"
#include "alarm_manager.h"
#include "config/app_config.h"
#include <stdio.h>

LV_FONT_DECLARE(montserrat_72);
LV_FONT_DECLARE(montserrat_96);

lv_obj_t* scenario_create_mic_icon(void)
{
    lv_obj_t *icon = lv_image_create(lv_scr_act());
    lv_image_set_src(icon, &mic_icon);
    lv_obj_set_style_image_recolor(icon, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(icon, LV_OPA_COVER, 0);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET);
    return icon;
}

lv_obj_t* scenario_create_spinner(void)
{
    lv_obj_t *spinner = lv_spinner_create(lv_scr_act());
    lv_spinner_set_anim_params(spinner, 1000, 60);
    lv_obj_set_size(spinner, 100, 100);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET);

    lv_obj_set_style_arc_color(spinner, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(COLOR_BLUE), LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 10, LV_PART_MAIN);

    return spinner;
}

lv_obj_t* scenario_create_error_icon(void)
{
    lv_obj_t *icon = lv_image_create(lv_scr_act());
    lv_image_set_src(icon, &error_icon);
    lv_obj_set_style_image_recolor(icon, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(icon, LV_OPA_COVER, 0);
    return icon;
}

void scenario_display_alarm_list(lv_obj_t **label_array, int max_labels, int y_offset)
{
    alarm_entry_t next_alarms[3];
    int count = alarm_manager_get_next_alarms(next_alarms, 3);

    if (count > max_labels) {
        count = max_labels;
    }

    int spacing = 60;

    for (int i = 0; i < count; i++) {
        char time_str[32];
        snprintf(time_str, sizeof(time_str), LV_SYMBOL_BELL "  %02d:%02d",
                 next_alarms[i].hour, next_alarms[i].minute);

        label_array[i] = lv_label_create(lv_scr_act());
        lv_label_set_text(label_array[i], time_str);
        lv_obj_set_style_text_color(label_array[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(label_array[i], &lv_font_montserrat_48, 0);
        lv_obj_align(label_array[i], LV_ALIGN_CENTER, 0, y_offset + (i * spacing));
    }
}

void scenario_display_single_alarm(uint8_t hour, uint8_t minute,
                                   lv_obj_t **title_label, lv_obj_t **time_label)
{
    const char *alarm_title = "Alarm set:";
    const lv_font_t *title_font = &lv_font_montserrat_48;

    *title_label = lv_label_create(lv_scr_act());
    lv_label_set_text(*title_label, alarm_title);
    lv_obj_set_style_text_color(*title_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(*title_label, title_font, 0);
    lv_obj_align(*title_label, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET - 50);

    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hour, minute);

    *time_label = lv_label_create(lv_scr_act());
    lv_label_set_text(*time_label, time_str);
    lv_obj_set_style_text_color(*time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(*time_label, &montserrat_96, 0);
    lv_obj_align(*time_label, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET + 50);
}

void scenario_display_no_alarms(lv_obj_t **label)
{
    const char *no_alarms_text = "No alarms set";

    *label = lv_label_create(lv_scr_act());
    lv_label_set_text(*label, no_alarms_text);
    lv_obj_set_style_text_color(*label, lv_color_white(), 0);
    lv_obj_set_style_text_font(*label, &lv_font_montserrat_48, 0);
    lv_obj_align(*label, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET + 30);
}

void scenario_clear_obj(lv_obj_t **obj)
{
    if (*obj) {
        lv_obj_del(*obj);
        *obj = NULL;
    }
}
