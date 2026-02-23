#include "ui_manager.h"
#include "app_config.h"
#include "homescreen_ui.h"
#include "voice_assistant_ui.h"

#include "esp_log.h"
#include "bsp/display.h"
#include "bsp/esp-bsp.h"

void ui_manager_init(void)
{
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = true,  // Enable double buffering to prevent DMA race conditions
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = false,
        }
    };

    bsp_display_start_with_config(&cfg);
    bsp_display_rotate(NULL, LV_DISP_ROTATION_180);
    bsp_display_backlight_on();
    bsp_display_brightness_set(100);

    homescreen_ui_init();
    voice_assistant_ui_init();
}

void ui_show_homescreen(void)
{
    homescreen_ui_show();
    voice_assistant_ui_show_idle();
}

void ui_show_listening(void)
{
    voice_assistant_ui_show_listening();
}

void ui_show_processing(void)
{
    voice_assistant_ui_show_processing();
}

void ui_show_transcript(const char *text)
{
    voice_assistant_ui_show_transcript(text);
}

void ui_show_response(void)
{
    voice_assistant_ui_show_response();
}

void ui_show_error(const char *message)
{
    voice_assistant_ui_show_error(message);
}
