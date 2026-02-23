#ifndef RING_OVERLAY_H
#define RING_OVERLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "ui_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RING_COLOR_BLUE    COLOR_BLUE
#define RING_COLOR_GREEN   COLOR_GREEN
#define RING_COLOR_RED     COLOR_RED

void ring_overlay_show(uint32_t color);

void ring_overlay_hide(void);
bool ring_overlay_is_visible(void);
void ring_overlay_show_safe(uint32_t color);
void ring_overlay_hide_safe(void);

#ifdef __cplusplus
}
#endif

#endif
