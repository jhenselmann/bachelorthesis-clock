#include "voice_assistant_ui.h"
#include "scenarios/ui_scenario.h"
#include "app_config.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "VOICE_ASSISTANT_UI";

#define NVS_NAMESPACE "clock3000"
#define NVS_KEY_SCENARIO "ui_scenario"

static uint8_t current_scenario = APP_UI_SCENARIO;
static const ui_scenario_funcs_t *scenario = NULL;
static const ui_scenario_funcs_t *scenario_a = NULL;
static const ui_scenario_funcs_t *scenario_b = NULL;

static void load_scenario_from_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);

    if (err == ESP_OK) {
        uint8_t stored_scenario = APP_UI_SCENARIO;
        err = nvs_get_u8(nvs_handle, NVS_KEY_SCENARIO, &stored_scenario);

        if (err == ESP_OK && stored_scenario <= APP_UI_SCENARIO_B) {
            current_scenario = stored_scenario;
            ESP_LOGI(TAG, "Loaded scenario %c from NVS", 'A' + current_scenario);
        } else {
            ESP_LOGI(TAG, "No stored scenario, using default %c", 'A' + current_scenario);
        }

        nvs_close(nvs_handle);
    } else {
        ESP_LOGW(TAG, "Failed to open NVS, using default scenario %c", 'A' + current_scenario);
    }
}

static void save_scenario_to_nvs(uint8_t scenario)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);

    if (err == ESP_OK) {
        err = nvs_set_u8(nvs_handle, NVS_KEY_SCENARIO, scenario);
        if (err == ESP_OK) {
            nvs_commit(nvs_handle);
            ESP_LOGI(TAG, "Saved scenario %c to NVS", 'A' + scenario);
        } else {
            ESP_LOGE(TAG, "Failed to save scenario: %s", esp_err_to_name(err));
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "Failed to open NVS for writing: %s", esp_err_to_name(err));
    }
}

static const ui_scenario_funcs_t* get_scenario_funcs_by_id(uint8_t scenario)
{
    switch (scenario) {
        case APP_UI_SCENARIO_A:
            if (!scenario_a) {
                scenario_a = ui_scenario_a_get_funcs();
            }
            return scenario_a;

        case APP_UI_SCENARIO_B:
            if (!scenario_b) {
                scenario_b = ui_scenario_b_get_funcs();
            }
            return scenario_b;

        default:
            ESP_LOGW(TAG, "Invalid scenario %d, using B", scenario);
            if (!scenario_b) {
                scenario_b = ui_scenario_b_get_funcs();
            }
            return scenario_b;
    }
}

void voice_assistant_ui_init(void)
{
    // Load stored scenario from NVS
    load_scenario_from_nvs();

    // Initialize all scenarios (they share LVGL objects, so all need init)
    scenario_a = ui_scenario_a_get_funcs();
    scenario_b = ui_scenario_b_get_funcs();

    if (scenario_a && scenario_a->init) {
        scenario_a->init();
    }
    if (scenario_b && scenario_b->init) {
        scenario_b->init();
    }

    // Set active scenario
    scenario = get_scenario_funcs_by_id(current_scenario);

    ESP_LOGI(TAG, "Voice assistant UI initialized (scenario %c)",
             'A' + current_scenario);
}

void voice_assistant_ui_set_scenario(uint8_t new_scenario)
{
    if (new_scenario > APP_UI_SCENARIO_B) {
        ESP_LOGW(TAG, "Invalid scenario %d, ignoring", new_scenario);
        return;
    }

    if (new_scenario == current_scenario) {
        ESP_LOGD(TAG, "Scenario %c already active", 'A' + new_scenario);
        return;
    }

    // Hide current scenario if visible
    if (scenario && scenario->hide) {
        scenario->hide();
    }

    // Switch to new scenario
    current_scenario = new_scenario;
    scenario = get_scenario_funcs_by_id(new_scenario);

    // Save to NVS
    save_scenario_to_nvs(new_scenario);

    ESP_LOGI(TAG, "Switched to scenario %c", 'A' + new_scenario);
}

uint8_t voice_assistant_ui_get_scenario(void)
{
    return current_scenario;
}

void voice_assistant_ui_show_idle(void)
{
    if (scenario && scenario->show_idle) {
        scenario->show_idle();
    }
}

void voice_assistant_ui_show_listening(void)
{
    if (scenario && scenario->show_listening) {
        scenario->show_listening();
    }
}

void voice_assistant_ui_show_processing(void)
{
    if (scenario && scenario->show_processing) {
        scenario->show_processing();
    }
}

void voice_assistant_ui_show_transcript(const char *text)
{
    if (scenario && scenario->show_transcript) {
        scenario->show_transcript(text);
    }
}

void voice_assistant_ui_show_response(void)
{
    if (scenario && scenario->show_response) {
        scenario->show_response();
    }
}

void voice_assistant_ui_show_error(const char *message)
{
    if (scenario && scenario->show_error) {
        scenario->show_error(message);
    }
}

void voice_assistant_ui_hide(void)
{
    if (scenario && scenario->hide) {
        scenario->hide();
    }
}
