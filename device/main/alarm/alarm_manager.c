#include "alarm_manager.h"
#include "alarm_playback.h"

#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "ui/homescreen_ui.h"
#include "ui/ui_manager.h"

#define NVS_NAMESPACE "alarms"
#define NVS_KEY_ALARMS "alarm_data"

static alarm_entry_t alarms[ALARM_MAX_COUNT] = {0};
static alarm_trigger_callback_t trigger_callback = NULL;
static esp_timer_handle_t alarm_timer = NULL;
static int active_alarm_idx = -1;

static bool last_set_pending = false;
static uint8_t last_set_hour = 0;
static uint8_t last_set_minute = 0;
static bool show_alarm_list = false;

static esp_err_t load_from_nvs(void);
static esp_err_t save_to_nvs(void);

esp_err_t alarm_manager_init(void)
{
    memset(alarms, 0, sizeof(alarms));

    esp_err_t err = load_from_nvs();
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    return err;
}

int alarm_manager_add(uint8_t hour, uint8_t minute)
{
    if (hour > 23 || minute > 59) {
        return -1;
    }

    // Find first free slot
    for (int i = 0; i < ALARM_MAX_COUNT; i++) {
        if (!alarms[i].enabled) {
            alarms[i].hour = hour;
            alarms[i].minute = minute;
            alarms[i].enabled = true;
            alarms[i].triggered = false;
            save_to_nvs();
            return i;
        }
    }
    return -1;
}

esp_err_t alarm_manager_remove(int index)
{
    if (index < 0 || index >= ALARM_MAX_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    alarms[index].enabled = false;
    alarms[index].triggered = false;
    return save_to_nvs();
}

esp_err_t alarm_manager_remove_all(void)
{
    for (int i = 0; i < ALARM_MAX_COUNT; i++) {
        alarms[i].enabled = false;
        alarms[i].triggered = false;
    }
    return save_to_nvs();
}

const alarm_entry_t* alarm_manager_get_all(int *count)
{
    if (count) {
        *count = ALARM_MAX_COUNT;
    }
    return alarms;
}

void alarm_manager_register_callback(alarm_trigger_callback_t callback)
{
    trigger_callback = callback;
}

void alarm_manager_check(void)
{
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    for (int i = 0; i < ALARM_MAX_COUNT; i++) {
        if (!alarms[i].enabled || alarms[i].triggered) {
            continue;
        }
        if (alarms[i].hour == timeinfo.tm_hour && alarms[i].minute == timeinfo.tm_min) {
            alarms[i].triggered = true;
            if (trigger_callback) {
                trigger_callback(i);
            }
        }
    }
}

void alarm_manager_dismiss(int index)
{
    if (index >= 0 && index < ALARM_MAX_COUNT) {
        alarms[index].triggered = false;
    }
}

// ============ NVS Storage ============

static esp_err_t load_from_nvs(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }

    size_t size = sizeof(alarms);
    err = nvs_get_blob(handle, NVS_KEY_ALARMS, alarms, &size);
    nvs_close(handle);

    // Reset triggered state on load
    for (int i = 0; i < ALARM_MAX_COUNT; i++) {
        alarms[i].triggered = false;
    }
    return err;
}

static esp_err_t save_to_nvs(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_blob(handle, NVS_KEY_ALARMS, alarms, sizeof(alarms));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

// ============ Next Alarms (sorted) ============

static int calc_minutes_until(int alarm_hour, int alarm_minute, int now_hour, int now_minute)
{
    int alarm_mins = alarm_hour * 60 + alarm_minute;
    int now_mins = now_hour * 60 + now_minute;
    if (alarm_mins <= now_mins) {
        alarm_mins += 24 * 60;  // Tomorrow
    }
    return alarm_mins - now_mins;
}

int alarm_manager_get_next_alarms(alarm_entry_t *out_alarms, int max_count)
{
    if (!out_alarms || max_count <= 0) {
        return 0;
    }

    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);

    // Collect enabled alarms with sort key
    typedef struct {
        alarm_entry_t alarm;
        int mins_until;
    } alarm_sort_t;

    alarm_sort_t temp[ALARM_MAX_COUNT];
    int count = 0;

    for (int i = 0; i < ALARM_MAX_COUNT; i++) {
        if (alarms[i].enabled) {
            temp[count].alarm = alarms[i];
            temp[count].mins_until = calc_minutes_until(
                alarms[i].hour, alarms[i].minute, t.tm_hour, t.tm_min);
            count++;
        }
    }

    // Bubble sort by mins_until
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (temp[j].mins_until > temp[j + 1].mins_until) {
                alarm_sort_t swap = temp[j];
                temp[j] = temp[j + 1];
                temp[j + 1] = swap;
            }
        }
    }

    int result = (count < max_count) ? count : max_count;
    for (int i = 0; i < result; i++) {
        out_alarms[i] = temp[i].alarm;
    }
    return result;
}

// ============ Timer & Callbacks ============

static void alarm_check_timer_callback(void *arg)
{
    alarm_manager_check();
}

static void on_alarm_stopped(void)
{
    homescreen_ui_hide_alarm_button();

    if (active_alarm_idx >= 0) {
        alarm_manager_remove(active_alarm_idx);
        active_alarm_idx = -1;
    }
}

static void on_alarm_triggered(int alarm_index)
{
    int count;
    const alarm_entry_t *alarms_list = alarm_manager_get_all(&count);
    if (alarm_index < 0 || alarm_index >= count) {
        return;
    }

    active_alarm_idx = alarm_index;
    ui_show_homescreen();

    if (alarm_playback_start() == ESP_OK) {
        homescreen_ui_show_alarm_button();
    } else {
        active_alarm_idx = -1;
    }

    alarm_manager_dismiss(alarm_index);
}

esp_err_t alarm_manager_start(void)
{
    alarm_manager_register_callback(on_alarm_triggered);
    alarm_playback_register_stopped_callback(on_alarm_stopped);

    const esp_timer_create_args_t timer_args = {
        .callback = alarm_check_timer_callback,
        .name = "alarm_check"
    };
    esp_err_t err = esp_timer_create(&timer_args, &alarm_timer);
    if (err != ESP_OK) {
        return err;
    }

    return esp_timer_start_periodic(alarm_timer, 60 * 1000 * 1000);
}

// ============ UI State ============

void alarm_manager_set_last_set(uint8_t hour, uint8_t minute)
{
    last_set_hour = hour;
    last_set_minute = minute;
    last_set_pending = true;
}

bool alarm_manager_get_last_set(uint8_t *hour, uint8_t *minute)
{
    if (!last_set_pending) {
        return false;
    }
    if (hour) *hour = last_set_hour;
    if (minute) *minute = last_set_minute;
    last_set_pending = false;
    return true;
}

void alarm_manager_set_show_list(bool show)
{
    show_alarm_list = show;
}

bool alarm_manager_get_show_list(void)
{
    if (!show_alarm_list) {
        return false;
    }
    show_alarm_list = false;
    return true;
}
