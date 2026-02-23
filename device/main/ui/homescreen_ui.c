#include "homescreen_ui.h"
#include "settings_ui.h"
#include "ui_constants.h"

#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "bsp/display.h"
#include "bsp/esp-bsp.h"
#include "alarm/alarm_playback.h"

LV_FONT_DECLARE(orbitron_192);

static const char *TAG = "HOMESCREEN_UI";

static lv_obj_t *time_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *alarm_stop_button = NULL;
static lv_obj_t *touch_area = NULL;
static esp_timer_handle_t clock_timer = NULL;
static esp_timer_handle_t longpress_timer = NULL;
static bool compact_mode = false;
static bool alarm_shift = false;
static bool longpress_active = false;

#define ALARM_BUTTON_WIDTH 200
#define ALARM_BUTTON_HEIGHT 60
#define LONGPRESS_DURATION_MS 3000

// Y positions for layout
#define CLOCK_Y_NORMAL    -30
#define CLOCK_Y_COMPACT   -120
#define DATE_Y_NORMAL     90
#define DATE_Y_COMPACT    0
#define ALARM_BTN_Y       180

// Long-press timer callback - opens settings
static void longpress_timer_callback(void *arg)
{
    ESP_LOGI(TAG, "Long press - opening settings");
    longpress_active = false;

    // Hide homescreen and show settings
    homescreen_ui_hide();
    settings_ui_show();
}

// Touch event handler for long-press detection
static void touch_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        // Start long-press timer
        if (longpress_timer && !longpress_active) {
            longpress_active = true;
            esp_timer_start_once(longpress_timer, LONGPRESS_DURATION_MS * 1000);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        // Cancel long-press timer
        if (longpress_timer && longpress_active) {
            esp_timer_stop(longpress_timer);
            longpress_active = false;
        }
    }
}

static void alarm_stop_button_clicked(lv_event_t *e)
{
    ESP_LOGI(TAG, "Alarm stop button pressed");
    alarm_playback_stop();
}

static void create_alarm_stop_button(void)
{
    alarm_stop_button = lv_btn_create(lv_scr_act());
    lv_obj_set_size(alarm_stop_button, ALARM_BUTTON_WIDTH, ALARM_BUTTON_HEIGHT);
    lv_obj_align(alarm_stop_button, LV_ALIGN_CENTER, 0, ALARM_BTN_Y);
    lv_obj_set_style_radius(alarm_stop_button, 15, 0);  // Rounded corners
    lv_obj_set_style_bg_color(alarm_stop_button, lv_color_hex(COLOR_ERROR), 0);
    lv_obj_set_style_bg_color(alarm_stop_button, lv_color_hex(COLOR_RED_DARK), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(alarm_stop_button, 0, 0);
    lv_obj_set_style_border_width(alarm_stop_button, 0, 0);
    lv_obj_add_event_cb(alarm_stop_button, alarm_stop_button_clicked, LV_EVENT_CLICKED, NULL);

    // Label with black text
    lv_obj_t *label = lv_label_create(alarm_stop_button);
    lv_label_set_text(label, "STOP");
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_BLACK), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);

    // Start hidden
    lv_obj_add_flag(alarm_stop_button, LV_OBJ_FLAG_HIDDEN);
}

static void create_touch_area(void)
{
    // Create invisible touch area covering the whole screen for long-press detection
    touch_area = lv_obj_create(lv_scr_act());
    lv_obj_set_size(touch_area, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(touch_area, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(touch_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(touch_area, 0, 0);
    lv_obj_clear_flag(touch_area, LV_OBJ_FLAG_SCROLLABLE);

    // Add touch events for long-press detection
    lv_obj_add_event_cb(touch_area, touch_event_handler, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(touch_area, touch_event_handler, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(touch_area, touch_event_handler, LV_EVENT_PRESS_LOST, NULL);

    // Start hidden
    lv_obj_add_flag(touch_area, LV_OBJ_FLAG_HIDDEN);
}

static void clock_timer_callback(void* arg)
{
    const char* weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);

    char time_str[16];
    strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);

    char date_str[64];
    snprintf(date_str, sizeof(date_str), "%02d.%02d.%04d %s",
             timeinfo.tm_mday,
             timeinfo.tm_mon + 1,
             timeinfo.tm_year + 1900,
             weekdays[timeinfo.tm_wday]);

    bsp_display_lock(0);
    if (time_label) {
        lv_label_set_text(time_label, time_str);
    }
    if (date_label) {
        lv_label_set_text(date_label, date_str);
    }
    bsp_display_unlock();
}

void homescreen_ui_init(void)
{
    // Create long-press timer
    const esp_timer_create_args_t longpress_timer_args = {
        .callback = &longpress_timer_callback,
        .name = "longpress_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&longpress_timer_args, &longpress_timer));

    bsp_display_lock(0);

    time_label = lv_label_create(lv_scr_act());
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, CLOCK_Y_NORMAL);
    lv_obj_set_style_text_color(time_label, lv_color_hex(COLOR_WHITE), 0);
    lv_obj_set_style_text_font(time_label, &orbitron_192, 0);
    lv_obj_add_flag(time_label, LV_OBJ_FLAG_HIDDEN);

    date_label = lv_label_create(lv_scr_act());
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, DATE_Y_NORMAL);
    lv_obj_set_style_text_color(date_label, lv_color_hex(COLOR_WHITE), 0);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_48, 0);
    lv_obj_add_flag(date_label, LV_OBJ_FLAG_HIDDEN);

    create_alarm_stop_button();
    create_touch_area();

    bsp_display_unlock();

    ESP_LOGI(TAG, "Homescreen UI initialized (5s long-press for settings)");
}

void homescreen_ui_show(void)
{
    bsp_display_lock(0);

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(COLOR_BLACK), 0);

    if (time_label) {
        lv_obj_clear_flag(time_label, LV_OBJ_FLAG_HIDDEN);
    }
    if (date_label) {
        lv_obj_clear_flag(date_label, LV_OBJ_FLAG_HIDDEN);
    }
    if (touch_area) {
        lv_obj_clear_flag(touch_area, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(touch_area);  // Ensure touch area is on top
    }

    bsp_display_unlock();

    clock_timer_callback(NULL);

    if (!clock_timer) {
        const esp_timer_create_args_t timer_args = {
            .callback = &clock_timer_callback,
            .name = "clock_update"
        };
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &clock_timer));
    }

    // Always restart the timer (it may have been stopped in hide())
    esp_timer_stop(clock_timer);  // Stop first in case it's running
    ESP_ERROR_CHECK(esp_timer_start_periodic(clock_timer, 5000000));
}

void homescreen_ui_hide(void)
{
    // Cancel any pending long-press
    if (longpress_timer && longpress_active) {
        esp_timer_stop(longpress_timer);
        longpress_active = false;
    }

    bsp_display_lock(0);

    if (time_label) {
        lv_obj_add_flag(time_label, LV_OBJ_FLAG_HIDDEN);
    }
    if (date_label) {
        lv_obj_add_flag(date_label, LV_OBJ_FLAG_HIDDEN);
    }
    if (alarm_stop_button) {
        lv_obj_add_flag(alarm_stop_button, LV_OBJ_FLAG_HIDDEN);
    }
    if (touch_area) {
        lv_obj_add_flag(touch_area, LV_OBJ_FLAG_HIDDEN);
    }

    bsp_display_unlock();

    if (clock_timer) {
        esp_timer_stop(clock_timer);
    }
}

void homescreen_ui_show_alarm_button(void)
{
    bsp_display_lock(0);
    if (alarm_stop_button) {
        lv_obj_clear_flag(alarm_stop_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(alarm_stop_button);
    }
    // Disable long-press while alarm is active
    if (touch_area) {
        lv_obj_add_flag(touch_area, LV_OBJ_FLAG_HIDDEN);
    }
    bsp_display_unlock();
}

void homescreen_ui_hide_alarm_button(void)
{
    bsp_display_lock(0);
    if (alarm_stop_button) {
        lv_obj_add_flag(alarm_stop_button, LV_OBJ_FLAG_HIDDEN);
    }
    // Re-enable long-press
    if (touch_area) {
        lv_obj_clear_flag(touch_area, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(touch_area);
    }
    bsp_display_unlock();
}

void homescreen_ui_set_compact(bool compact)
{
    if (compact_mode == compact) {
        return;  // No change
    }
    compact_mode = compact;

    bsp_display_lock(0);

    if (compact) {
        if (time_label) {
            lv_obj_align(time_label, LV_ALIGN_CENTER, 0, CLOCK_Y_COMPACT);
        }
        if (date_label) {
            lv_obj_add_flag(date_label, LV_OBJ_FLAG_HIDDEN);
        }
        if (touch_area) {
            lv_obj_add_flag(touch_area, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        if (time_label) {
            int y = alarm_shift ? CLOCK_Y_COMPACT : CLOCK_Y_NORMAL;
            lv_obj_align(time_label, LV_ALIGN_CENTER, 0, y);
        }
        if (date_label) {
            int y = alarm_shift ? DATE_Y_COMPACT : DATE_Y_NORMAL;
            lv_obj_clear_flag(date_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(date_label, LV_ALIGN_CENTER, 0, y);
        }
        if (touch_area) {
            lv_obj_clear_flag(touch_area, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(touch_area);
        }
    }

    bsp_display_unlock();

    ESP_LOGD(TAG, "Compact mode: %s", compact ? "ON" : "OFF");
}

void homescreen_ui_set_alarm_shift(bool shift)
{
    if (alarm_shift == shift) {
        return;  // No change
    }
    alarm_shift = shift;

    // Don't shift if already in compact mode
    if (compact_mode) {
        return;
    }

    bsp_display_lock(0);

    if (shift) {
        if (time_label) {
            lv_obj_align(time_label, LV_ALIGN_CENTER, 0, CLOCK_Y_COMPACT);
        }
        if (date_label) {
            lv_obj_align(date_label, LV_ALIGN_CENTER, 0, DATE_Y_COMPACT);
        }
    } else {
        if (time_label) {
            lv_obj_align(time_label, LV_ALIGN_CENTER, 0, CLOCK_Y_NORMAL);
        }
        if (date_label) {
            lv_obj_align(date_label, LV_ALIGN_CENTER, 0, DATE_Y_NORMAL);
        }
    }

    bsp_display_unlock();

    ESP_LOGD(TAG, "Alarm shift: %s", shift ? "ON" : "OFF");
}
