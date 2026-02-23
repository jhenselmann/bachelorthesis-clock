#pragma once

#include "lvgl.h"

//Layout
#define UI_ICON_Y_OFFSET 120
#define UI_SPINNER_SIZE 100

//Base Colors
#define COLOR_BLACK      0x000000
#define COLOR_WHITE      0xFFFFFF
#define COLOR_BLUE       0x3498db
#define COLOR_GREEN      0x27ae60
#define COLOR_RED        0xe74c3c
#define COLOR_ORANGE     0xf39c12
#define COLOR_GRAY       0x888888
#define COLOR_LIGHT_GRAY 0xAAAAAA
#define COLOR_DARK_GRAY  0x333333
#define COLOR_DARKER_GRAY 0x111111
#define COLOR_RED_DARK   0xCC0000

//Voice Assistant State Colors
#define COLOR_READY      COLOR_GREEN
#define COLOR_LISTENING  COLOR_BLUE
#define COLOR_THINKING   COLOR_BLUE
#define COLOR_SPEAKING   COLOR_ORANGE
#define COLOR_ERROR      COLOR_RED

// UI Element Color
#define COLOR_BTN_DEFAULT  COLOR_DARK_GRAY
#define COLOR_BTN_PRESSED  0x555555
#define COLOR_BACKGROUND   COLOR_DARKER_GRAY

#define UI_RECOLOR_OPA LV_OPA_COVER
