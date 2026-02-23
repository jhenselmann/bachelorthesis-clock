#include "settings_ui.h"
#include "ui_constants.h"
#include "voice_assistant_ui.h"
#include "ui_manager.h"
#include "config/app_config.h"

#include "audio/wakeword.h"
#include "alarm/alarm_manager.h"

#include "bsp/display.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "SETTINGS_UI";

static lv_obj_t *container = NULL;
static lv_obj_t *voice_btn = NULL;
static lv_obj_t *delete_alarms_btn = NULL;
static lv_obj_t *scenario_a_btn = NULL;
static lv_obj_t *scenario_b_btn = NULL;
static lv_obj_t *alarm_scenario_btn = NULL;
static lv_obj_t *close_btn = NULL;
static bool visible = false;

// Button styling
#define BTN_WIDTH 280
#define BTN_HEIGHT 70
#define BTN_SPACING 15
#define BTN_RADIUS 15

static void close_settings(void)
{
    settings_ui_hide();
    ui_show_homescreen();
}

static void voice_btn_clicked(lv_event_t *e)
{
    ESP_LOGI(TAG, "Voice assistant button pressed");
    close_settings();
    wakeword_trigger_manual();
}

static void delete_alarms_btn_clicked(lv_event_t *e)
{
    ESP_LOGI(TAG, "Delete all alarms button pressed");
    alarm_manager_remove_all();
    // Stay in settings
}

static void scenario_a_btn_clicked(lv_event_t *e)
{
    ESP_LOGI(TAG, "Scenario A selected");
    voice_assistant_ui_set_scenario(APP_UI_SCENARIO_A);
    close_settings();
}

static void scenario_b_btn_clicked(lv_event_t *e)
{
    ESP_LOGI(TAG, "Scenario B selected");
    voice_assistant_ui_set_scenario(APP_UI_SCENARIO_B);
    close_settings();
}

static void alarm_scenario_btn_clicked(lv_event_t *e)
{
    ESP_LOGI(TAG, "Alarm Scenario 3 selected - setting 3 alarms");

    // Remove all existing alarms
    alarm_manager_remove_all();

    // Add 3 predefined alarms: 5:40, 8:05, 9:20
    int idx1 = alarm_manager_add(5, 40);
    int idx2 = alarm_manager_add(8, 5);
    int idx3 = alarm_manager_add(9, 20);

    ESP_LOGI(TAG, "Created alarms at indices: %d, %d, %d", idx1, idx2, idx3);

    // Stay in settings (user can see result)
}

static void close_btn_clicked(lv_event_t *e)
{
    ESP_LOGI(TAG, "Close button pressed");
    close_settings();
}

static lv_obj_t* create_button(lv_obj_t *parent, const char *text,
                                lv_event_cb_t callback, bool is_active)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, BTN_WIDTH, BTN_HEIGHT);
    lv_obj_set_style_radius(btn, BTN_RADIUS, 0);

    uint32_t bg_color = is_active ? COLOR_BLUE : COLOR_BTN_DEFAULT;
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_BTN_PRESSED), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_WHITE), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);

    return btn;
}

static void update_scenario_buttons(void)
{
    uint8_t current = voice_assistant_ui_get_scenario();

    // Update button colors based on active scenario
    if (scenario_a_btn) {
        uint32_t color = (current == APP_UI_SCENARIO_A) ? COLOR_BLUE : COLOR_BTN_DEFAULT;
        lv_obj_set_style_bg_color(scenario_a_btn, lv_color_hex(color), 0);
    }
    if (scenario_b_btn) {
        uint32_t color = (current == APP_UI_SCENARIO_B) ? COLOR_BLUE : COLOR_BTN_DEFAULT;
        lv_obj_set_style_bg_color(scenario_b_btn, lv_color_hex(color), 0);
    }
}

void settings_ui_init(void)
{
    bsp_display_lock(0);

    // Create container that covers the whole screen
    container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(container, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(container, lv_color_hex(COLOR_BACKGROUND), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_radius(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t *title = lv_label_create(container);
    lv_label_set_text(title, "Einstellungen");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_text_color(title, lv_color_hex(COLOR_WHITE), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);

    // Calculate vertical positions for buttons
    // Screen height is 600, we have 6 buttons + title
    int start_y = 150;
    int btn_step = BTN_HEIGHT + BTN_SPACING;

    // Row 1: Voice Assistant + Delete Alarms (side by side)
    voice_btn = create_button(container, "Sprachassistent",
                                 voice_btn_clicked, false);
    lv_obj_align(voice_btn, LV_ALIGN_TOP_MID, -(BTN_WIDTH/2 + 10), start_y);

    delete_alarms_btn = create_button(container, "Wecker loeschen",
                                         delete_alarms_btn_clicked, false);
    lv_obj_align(delete_alarms_btn, LV_ALIGN_TOP_MID, (BTN_WIDTH/2 + 10), start_y);

    // Scenario section label
    lv_obj_t *scenario_label = lv_label_create(container);
    lv_label_set_text(scenario_label, "UI Szenario");
    lv_obj_align(scenario_label, LV_ALIGN_TOP_MID, 0, start_y + btn_step + 20);
    lv_obj_set_style_text_color(scenario_label, lv_color_hex(COLOR_GRAY), 0);
    lv_obj_set_style_text_font(scenario_label, &lv_font_montserrat_20, 0);

    // Row 2: Scenario A/B (side by side)
    int scenario_y = start_y + btn_step + 60;
    int scenario_btn_width = 240;
    int scenario_spacing = 20;

    uint8_t current_scenario = voice_assistant_ui_get_scenario();

    scenario_a_btn = lv_btn_create(container);
    lv_obj_set_size(scenario_a_btn, scenario_btn_width, BTN_HEIGHT);
    lv_obj_align(scenario_a_btn, LV_ALIGN_TOP_MID, -(scenario_btn_width/2 + scenario_spacing/2), scenario_y);
    lv_obj_set_style_radius(scenario_a_btn, BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(scenario_a_btn, lv_color_hex(
        current_scenario == APP_UI_SCENARIO_A ? COLOR_BLUE : COLOR_BTN_DEFAULT), 0);
    lv_obj_set_style_bg_color(scenario_a_btn, lv_color_hex(COLOR_BTN_PRESSED), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(scenario_a_btn, 0, 0);
    lv_obj_set_style_border_width(scenario_a_btn, 0, 0);
    lv_obj_add_event_cb(scenario_a_btn, scenario_a_btn_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label_a = lv_label_create(scenario_a_btn);
    lv_label_set_text(label_a, "Szenario A");
    lv_obj_center(label_a);
    lv_obj_set_style_text_color(label_a, lv_color_hex(COLOR_WHITE), 0);
    lv_obj_set_style_text_font(label_a, &lv_font_montserrat_20, 0);

    scenario_b_btn = lv_btn_create(container);
    lv_obj_set_size(scenario_b_btn, scenario_btn_width, BTN_HEIGHT);
    lv_obj_align(scenario_b_btn, LV_ALIGN_TOP_MID, (scenario_btn_width/2 + scenario_spacing/2), scenario_y);
    lv_obj_set_style_radius(scenario_b_btn, BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(scenario_b_btn, lv_color_hex(
        current_scenario == APP_UI_SCENARIO_B ? COLOR_BLUE : COLOR_BTN_DEFAULT), 0);
    lv_obj_set_style_bg_color(scenario_b_btn, lv_color_hex(COLOR_BTN_PRESSED), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(scenario_b_btn, 0, 0);
    lv_obj_set_style_border_width(scenario_b_btn, 0, 0);
    lv_obj_add_event_cb(scenario_b_btn, scenario_b_btn_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label_b = lv_label_create(scenario_b_btn);
    lv_label_set_text(label_b, "Szenario B");
    lv_obj_center(label_b);
    lv_obj_set_style_text_color(label_b, lv_color_hex(COLOR_WHITE), 0);
    lv_obj_set_style_text_font(label_b, &lv_font_montserrat_20, 0);

    // Alarm preset section label
    lv_obj_t *alarm_preset_label = lv_label_create(container);
    lv_label_set_text(alarm_preset_label, "Wecker Preset");
    lv_obj_align(alarm_preset_label, LV_ALIGN_TOP_MID, 0, scenario_y + btn_step + 20);
    lv_obj_set_style_text_color(alarm_preset_label, lv_color_hex(COLOR_GRAY), 0);
    lv_obj_set_style_text_font(alarm_preset_label, &lv_font_montserrat_20, 0);

    // Alarm preset button: Szenario 3 (5:40, 8:05, 9:20)
    int alarm_preset_y = scenario_y + btn_step + 60;
    alarm_scenario_btn = create_button(container, "Szenario 3 (5:40, 8:05, 9:20)",
                                          alarm_scenario_btn_clicked, false);
    lv_obj_set_width(alarm_scenario_btn, BTN_WIDTH + 100);  // Wider for longer text
    lv_obj_align(alarm_scenario_btn, LV_ALIGN_TOP_MID, 0, alarm_preset_y);

    // Close button at bottom
    close_btn = create_button(container, "Schliessen",
                                 close_btn_clicked, false);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -40);

    // Start hidden
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);

    bsp_display_unlock();

    ESP_LOGI(TAG, "Settings UI initialized");
}

void settings_ui_show(void)
{
    if (visible) {
        return;
    }

    bsp_display_lock(0);

    // Update scenario button colors before showing
    update_scenario_buttons();

    if (container) {
        lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(container);
    }

    visible = true;

    bsp_display_unlock();

    ESP_LOGI(TAG, "Settings UI shown");
}

void settings_ui_hide(void)
{
    if (!visible) {
        return;
    }

    bsp_display_lock(0);

    if (container) {
        lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
    }

    visible = false;

    bsp_display_unlock();

    ESP_LOGI(TAG, "Settings UI hidden");
}

bool settings_ui_is_visible(void)
{
    return visible;
}
