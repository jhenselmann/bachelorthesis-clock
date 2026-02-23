#include "ui_scenario.h"
#include "scenario_common.h"
#include "ring_overlay.h"
#include "homescreen_ui.h"
#include "alarm_manager.h"
#include "ui/ui_constants.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "bsp/display.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "SCENARIO_B";

// UI elements for icons
static lv_obj_t *s_mic_icon_obj = NULL;
static lv_obj_t *s_spinner_obj = NULL;
static lv_obj_t *s_alarm_title_label = NULL;
static lv_obj_t *s_alarm_time_label = NULL;
static lv_obj_t *s_alarm_countdown_label = NULL;
static lv_obj_t *s_error_icon_obj = NULL;
static lv_obj_t *s_alarm_labels[3] = {NULL, NULL, NULL};
static lv_obj_t *s_transcript_label = NULL;
static lv_obj_t *s_error_title_label = NULL;
static lv_obj_t *s_error_attribution_label = NULL;
static lv_obj_t *s_error_recovery_label = NULL;

// Alarm display state
static struct {
    bool show_single;
    bool show_list;
    uint8_t hour;
    uint8_t minute;
} pending_alarm = {0};

static lv_timer_t *countdown_timer = NULL;
static lv_timer_t *alarm_hide_timer = NULL;

static void scenario_b_show_transcript(const char *text);

static void clear_icons(void)
{
    scenario_clear_obj(&s_mic_icon_obj);
    scenario_clear_obj(&s_spinner_obj);
    scenario_clear_obj(&s_alarm_title_label);
    scenario_clear_obj(&s_alarm_time_label);
    scenario_clear_obj(&s_alarm_countdown_label);
    scenario_clear_obj(&s_error_icon_obj);
    scenario_clear_obj(&s_transcript_label);
    scenario_clear_obj(&s_error_title_label);
    scenario_clear_obj(&s_error_attribution_label);
    scenario_clear_obj(&s_error_recovery_label);
    for (int i = 0; i < 3; i++) {
        scenario_clear_obj(&s_alarm_labels[i]);
    }
}

static void format_countdown(char *buf, size_t buf_size, int alarm_hour, int alarm_minute)
{
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    int current_hour = timeinfo.tm_hour;
    int current_min = timeinfo.tm_min;

    int current_mins = current_hour * 60 + current_min;
    int alarm_mins = alarm_hour * 60 + alarm_minute;

    if (alarm_mins <= current_mins) {
        alarm_mins += 24 * 60;  // Tomorrow
    }

    int diff = alarm_mins - current_mins;
    int hours = diff / 60;
    int mins = diff % 60;

    if (hours > 0 && mins > 0) {
        snprintf(buf, buf_size, "in %dh %dmin", hours, mins);
    } else if (hours > 0) {
        snprintf(buf, buf_size, "in %dh", hours);
    } else {
        snprintf(buf, buf_size, "in %dmin", mins);
    }
}

static void countdown_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (!pending_alarm.show_single || !s_alarm_countdown_label) {
        return;
    }

    char countdown_str[32];
    format_countdown(countdown_str, sizeof(countdown_str), pending_alarm.hour, pending_alarm.minute);

    bsp_display_lock(0);
    if (s_alarm_countdown_label) {
        lv_label_set_text(s_alarm_countdown_label, countdown_str);
    }
    bsp_display_unlock();
}

static void alarm_hide_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    bsp_display_lock(0);

    if (countdown_timer) {
        lv_timer_del(countdown_timer);
        countdown_timer = NULL;
    }

    alarm_hide_timer = NULL;

    for (int i = 0; i < 3; i++) {
        scenario_clear_obj(&s_alarm_labels[i]);
    }

    scenario_clear_obj(&s_alarm_time_label);
    scenario_clear_obj(&s_alarm_countdown_label);

    homescreen_ui_set_alarm_shift(false);

    bsp_display_unlock();

    ESP_LOGI(TAG, "Alarms hidden after 10 seconds");
}

static void scenario_b_init(void)
{
    ESP_LOGI(TAG, "Scenario B");
}

static void show_single_alarm_with_countdown(void)
{
    homescreen_ui_set_alarm_shift(true);

    char time_str[32];
    snprintf(time_str, sizeof(time_str), LV_SYMBOL_BELL "  %02d:%02d",
             pending_alarm.hour, pending_alarm.minute);

    char countdown_str[32];
    format_countdown(countdown_str, sizeof(countdown_str), pending_alarm.hour, pending_alarm.minute);

    bsp_display_lock(0);

    s_alarm_time_label = lv_label_create(lv_scr_act());
    lv_label_set_text(s_alarm_time_label, time_str);
    lv_obj_set_style_text_color(s_alarm_time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_alarm_time_label, &lv_font_montserrat_48, 0);
    lv_obj_align(s_alarm_time_label, LV_ALIGN_CENTER, 0, 80);

    s_alarm_countdown_label = lv_label_create(lv_scr_act());
    lv_label_set_text(s_alarm_countdown_label, countdown_str);
    lv_obj_set_style_text_color(s_alarm_countdown_label, lv_color_hex(COLOR_LIGHT_GRAY), 0);
    lv_obj_set_style_text_font(s_alarm_countdown_label, &lv_font_montserrat_32, 0);
    lv_obj_align(s_alarm_countdown_label, LV_ALIGN_CENTER, 0, 150);

    if (!countdown_timer) {
        countdown_timer = lv_timer_create(countdown_timer_cb, 60000, NULL);
    }

    bsp_display_unlock();

    if (alarm_hide_timer) {
        lv_timer_del(alarm_hide_timer);
        alarm_hide_timer = NULL;
    }

    alarm_hide_timer = lv_timer_create(alarm_hide_timer_cb, 10000, NULL);
    lv_timer_set_repeat_count(alarm_hide_timer, 1);

    ESP_LOGI(TAG, "Showing alarm %02d:%02d (hide in 10s)", pending_alarm.hour, pending_alarm.minute);
}

static void show_upcoming_alarms(void)
{
    alarm_entry_t next_alarms[3];
    int count = alarm_manager_get_next_alarms(next_alarms, 3);

    if (count > 0) {
        homescreen_ui_set_alarm_shift(true);

        int start_y = 80;
        int spacing = 60;

        bsp_display_lock(0);
        for (int i = 0; i < count; i++) {
            char time_str[32];
            snprintf(time_str, sizeof(time_str), LV_SYMBOL_BELL "  %02d:%02d",
                     next_alarms[i].hour, next_alarms[i].minute);

            s_alarm_labels[i] = lv_label_create(lv_scr_act());
            lv_label_set_text(s_alarm_labels[i], time_str);
            lv_obj_set_style_text_color(s_alarm_labels[i], lv_color_white(), 0);
            lv_obj_set_style_text_font(s_alarm_labels[i], &lv_font_montserrat_48, 0);
            lv_obj_align(s_alarm_labels[i], LV_ALIGN_CENTER, 0, start_y + (i * spacing));
        }
        bsp_display_unlock();

        if (alarm_hide_timer) {
            lv_timer_del(alarm_hide_timer);
            alarm_hide_timer = NULL;
        }

        alarm_hide_timer = lv_timer_create(alarm_hide_timer_cb, 10000, NULL);
        lv_timer_set_repeat_count(alarm_hide_timer, 1);

        ESP_LOGI(TAG, "Showing %d alarms (hide in 10s)", count);
    } else {
        homescreen_ui_set_alarm_shift(false);
    }
}

static void scenario_b_show_idle(void)
{
    homescreen_ui_set_compact(false);

    bsp_display_lock(0);
    clear_icons();
    ring_overlay_hide();
    bsp_display_unlock();

    if (pending_alarm.show_single) {
        pending_alarm.show_single = false;
        show_single_alarm_with_countdown();
    } else if (pending_alarm.show_list) {
        pending_alarm.show_list = false;
        show_upcoming_alarms();
    }

    ESP_LOGD(TAG, "Idle - full clock");
}

static void scenario_b_show_listening(void)
{
    pending_alarm.show_single = false;
    pending_alarm.show_list = false;

    homescreen_ui_set_compact(true);

    bsp_display_lock(0);

    if (countdown_timer) {
        lv_timer_del(countdown_timer);
        countdown_timer = NULL;
    }
    if (alarm_hide_timer) {
        lv_timer_del(alarm_hide_timer);
        alarm_hide_timer = NULL;
    }

    clear_icons();
    ring_overlay_show(RING_COLOR_BLUE);

    s_mic_icon_obj = scenario_create_mic_icon();

    bsp_display_unlock();
    ESP_LOGD(TAG, "Listening - compact clock + mic icon");
}

static void scenario_b_show_processing(void)
{
    homescreen_ui_set_compact(true);

    bsp_display_lock(0);

    if (countdown_timer) {
        lv_timer_del(countdown_timer);
        countdown_timer = NULL;
    }
    if (alarm_hide_timer) {
        lv_timer_del(alarm_hide_timer);
        alarm_hide_timer = NULL;
    }

    clear_icons();
    ring_overlay_show(RING_COLOR_BLUE);

    s_spinner_obj = scenario_create_spinner();

    bsp_display_unlock();

    ESP_LOGD(TAG, "Processing - compact clock + spinner");
}

static void scenario_b_show_transcript(const char *text)
{
    if (!text || strlen(text) == 0) {
        return;
    }

    bsp_display_lock(0);

    if (!s_transcript_label) {
        s_transcript_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_color(s_transcript_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(s_transcript_label, &lv_font_montserrat_32, 0);
        lv_obj_set_style_text_align(s_transcript_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(s_transcript_label, 900);
        lv_label_set_long_mode(s_transcript_label, LV_LABEL_LONG_WRAP);
    }

    char quoted_text[256];
    snprintf(quoted_text, sizeof(quoted_text), "\"%s\"", text);
    lv_label_set_text(s_transcript_label, quoted_text);
    lv_obj_align(s_transcript_label, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET);

    if (s_spinner_obj) {
        lv_obj_set_size(s_spinner_obj, 60, 60);
        lv_obj_set_style_arc_width(s_spinner_obj, 6, LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(s_spinner_obj, 6, LV_PART_MAIN);
        lv_obj_align(s_spinner_obj, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET + 80);
    }

    bsp_display_unlock();
    ESP_LOGD(TAG, "Processing - showing transcript: %s", text);
}

static void scenario_b_show_response(void)
{
    homescreen_ui_set_compact(true);

    bsp_display_lock(0);

    if (countdown_timer) {
        lv_timer_del(countdown_timer);
        countdown_timer = NULL;
    }
    if (alarm_hide_timer) {
        lv_timer_del(alarm_hide_timer);
        alarm_hide_timer = NULL;
    }

    clear_icons();
    ring_overlay_show(RING_COLOR_GREEN);

    if (alarm_manager_get_show_list()) {
        pending_alarm.show_list = true;

        s_alarm_title_label = lv_label_create(lv_scr_act());
        lv_label_set_text(s_alarm_title_label, "Your alarms:");
        lv_obj_set_style_text_color(s_alarm_title_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(s_alarm_title_label, &lv_font_montserrat_48, 0);
        lv_obj_align(s_alarm_title_label, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET - 70);

        alarm_entry_t next_alarms[3];
        int count = alarm_manager_get_next_alarms(next_alarms, 3);

        if (count > 0) {
            scenario_display_alarm_list(s_alarm_labels, 3, SCENARIO_ICON_Y_OFFSET + 10);
        } else {
            scenario_display_no_alarms(&s_alarm_time_label);
        }
    } else {
        uint8_t hour, minute;
        if (alarm_manager_get_last_set(&hour, &minute)) {
            pending_alarm.show_single = true;
            pending_alarm.hour = hour;
            pending_alarm.minute = minute;

            scenario_display_single_alarm(hour, minute, &s_alarm_title_label, &s_alarm_time_label);
        }
    }

    bsp_display_unlock();
}

static void scenario_b_show_error(const char *message)
{
    (void)message;

    pending_alarm.show_single = false;
    pending_alarm.show_list = false;

    homescreen_ui_set_compact(true);

    bsp_display_lock(0);

    if (countdown_timer) {
        lv_timer_del(countdown_timer);
        countdown_timer = NULL;
    }
    if (alarm_hide_timer) {
        lv_timer_del(alarm_hide_timer);
        alarm_hide_timer = NULL;
    }

    clear_icons();
    ring_overlay_show(RING_COLOR_RED);

    s_error_icon_obj = scenario_create_error_icon();
    lv_obj_align(s_error_icon_obj, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET - 80);

    const char *error_title = "Server unreachable";
    const char *error_attribution = "Your alarm clock is working fine";
    const char *error_recovery = "Please try again";

    s_error_title_label = lv_label_create(lv_scr_act());
    lv_label_set_text(s_error_title_label, error_title);
    lv_obj_set_style_text_color(s_error_title_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_error_title_label, &lv_font_montserrat_48, 0);
    lv_obj_align(s_error_title_label, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET + 10);

    s_error_attribution_label = lv_label_create(lv_scr_act());
    lv_label_set_text(s_error_attribution_label, error_attribution);
    lv_obj_set_style_text_color(s_error_attribution_label, lv_color_hex(COLOR_LIGHT_GRAY), 0);
    lv_obj_set_style_text_font(s_error_attribution_label, &lv_font_montserrat_32, 0);
    lv_obj_align(s_error_attribution_label, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET + 100);

    s_error_recovery_label = lv_label_create(lv_scr_act());
    lv_label_set_text(s_error_recovery_label, error_recovery);
    lv_obj_set_style_text_color(s_error_recovery_label, lv_color_hex(COLOR_GRAY), 0);
    lv_obj_set_style_text_font(s_error_recovery_label, &lv_font_montserrat_32, 0);
    lv_obj_align(s_error_recovery_label, LV_ALIGN_CENTER, 0, SCENARIO_ICON_Y_OFFSET + 150);

    bsp_display_unlock();
    ESP_LOGD(TAG, "Error - detailed attribution display");
}

static void scenario_b_hide(void)
{
    homescreen_ui_set_compact(false);
    homescreen_ui_set_alarm_shift(false);

    bsp_display_lock(0);

    if (countdown_timer) {
        lv_timer_del(countdown_timer);
        countdown_timer = NULL;
    }
    if (alarm_hide_timer) {
        lv_timer_del(alarm_hide_timer);
        alarm_hide_timer = NULL;
    }

    clear_icons();
    ring_overlay_hide();
    bsp_display_unlock();
}

static const ui_scenario_funcs_t s_scenario_b_funcs = {
    .init = scenario_b_init,
    .show_idle = scenario_b_show_idle,
    .show_listening = scenario_b_show_listening,
    .show_processing = scenario_b_show_processing,
    .show_transcript = scenario_b_show_transcript,
    .show_response = scenario_b_show_response,
    .show_error = scenario_b_show_error,
    .hide = scenario_b_hide,
};

const ui_scenario_funcs_t* ui_scenario_b_get_funcs(void)
{
    return &s_scenario_b_funcs;
}
