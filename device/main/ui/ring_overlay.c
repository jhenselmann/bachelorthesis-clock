#include "ring_overlay.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "esp_log.h"
#include "config/app_config.h"

static const char *TAG = "RING_OVERLAY";

// Ring configuration
#define DISPLAY_SIZE    800       // Round display diameter
#define RING_WIDTH      20
#define RING_OPA_MIN    LV_OPA_30
#define RING_OPA_MAX    LV_OPA_COVER
#define RING_ANIM_TIME  800       // ms per pulse cycle

static lv_obj_t *ring_obj = NULL;
static lv_anim_t ring_anim;

static void ring_anim_cb(void *var, int32_t value)
{
    if (ring_obj) {
        lv_obj_set_style_arc_opa((lv_obj_t *)var, value, LV_PART_MAIN);
    }
}

void ring_overlay_show(uint32_t color)
{
    if (ring_obj) {
        // Already showing - just update color
        lv_obj_set_style_arc_color(ring_obj, lv_color_hex(color), LV_PART_MAIN);
        return;
    }

    // Create full circle arc (360°)
    ring_obj = lv_arc_create(lv_scr_act());
    lv_obj_set_size(ring_obj, DISPLAY_SIZE, DISPLAY_SIZE);
    lv_obj_center(ring_obj);

    // Full circle: 0° to 360°
    lv_arc_set_angles(ring_obj, 0, 360);
    lv_arc_set_bg_angles(ring_obj, 0, 360);

    // Disable rotation and interaction
    lv_arc_set_rotation(ring_obj, 0);
    lv_obj_remove_style(ring_obj, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(ring_obj, LV_OBJ_FLAG_CLICKABLE);

    // Style the arc (main part = our visible ring)
    lv_obj_set_style_arc_color(ring_obj, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_arc_width(ring_obj, RING_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(ring_obj, RING_OPA_MIN, LV_PART_MAIN);

    // Hide the indicator part
    lv_obj_set_style_arc_opa(ring_obj, LV_OPA_TRANSP, LV_PART_INDICATOR);

    // Setup pulsing animation
    lv_anim_init(&ring_anim);
    lv_anim_set_var(&ring_anim, ring_obj);
    lv_anim_set_values(&ring_anim, RING_OPA_MIN, RING_OPA_MAX);
    lv_anim_set_time(&ring_anim, RING_ANIM_TIME);
    lv_anim_set_playback_time(&ring_anim, RING_ANIM_TIME);
    lv_anim_set_repeat_count(&ring_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&ring_anim, ring_anim_cb);
    lv_anim_start(&ring_anim);

    ESP_LOGD(TAG, "Ring shown (color: 0x%06lx)", color);
}

void ring_overlay_hide(void)
{
    if (!ring_obj) {
        return;
    }

    lv_anim_del(ring_obj, ring_anim_cb);
    lv_obj_del(ring_obj);
    ring_obj = NULL;

    ESP_LOGD(TAG, "Ring hidden");
}

bool ring_overlay_is_visible(void)
{
    return ring_obj != NULL;
}

void ring_overlay_show_safe(uint32_t color)
{
    bsp_display_lock(0);
    ring_overlay_show(color);
    bsp_display_unlock();
}

void ring_overlay_hide_safe(void)
{
    bsp_display_lock(0);
    ring_overlay_hide();
    bsp_display_unlock();
}
