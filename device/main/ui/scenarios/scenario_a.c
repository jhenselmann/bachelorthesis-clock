#include "ui_scenario.h"
#include "ring_overlay.h"
#include "homescreen_ui.h"

#include "esp_log.h"
#include "bsp/display.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "SCENARIO_A";

static void scenario_a_init(void)
{
    ESP_LOGI(TAG, "Scenario A");
}

static void scenario_a_show_idle(void)
{
    homescreen_ui_set_alarm_shift(false);

    bsp_display_lock(0);
    ring_overlay_hide();
    bsp_display_unlock();
}

static void scenario_a_show_listening(void)
{
    bsp_display_lock(0);
    ring_overlay_show(RING_COLOR_BLUE);
    bsp_display_unlock();
    ESP_LOGD(TAG, "Listening - blue ring");
}

static void scenario_a_show_processing(void)
{
    bsp_display_lock(0);
    ring_overlay_show(RING_COLOR_BLUE);
    bsp_display_unlock();
    ESP_LOGD(TAG, "Processing - blue ring");
}

static void scenario_a_show_response(void)
{
    bsp_display_lock(0);
    ring_overlay_show(RING_COLOR_GREEN);
    bsp_display_unlock();
    ESP_LOGD(TAG, "Response - green ring");
}

static void scenario_a_show_error(const char *message)
{
    (void)message;
    bsp_display_lock(0);
    ring_overlay_show(RING_COLOR_RED);
    bsp_display_unlock();
    ESP_LOGD(TAG, "Error - red ring");
}

static void scenario_a_hide(void)
{
    bsp_display_lock(0);
    ring_overlay_hide();
    bsp_display_unlock();
}

static const ui_scenario_funcs_t s_scenario_a_funcs = {
    .init = scenario_a_init,
    .show_idle = scenario_a_show_idle,
    .show_listening = scenario_a_show_listening,
    .show_processing = scenario_a_show_processing,
    .show_transcript = NULL,
    .show_response = scenario_a_show_response,
    .show_error = scenario_a_show_error,
    .hide = scenario_a_hide,
};

const ui_scenario_funcs_t* ui_scenario_a_get_funcs(void)
{
    return &s_scenario_a_funcs;
}
