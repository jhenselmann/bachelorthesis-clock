// Homescreen with clock (HH:MM) and date

#ifndef HOMESCREEN_UI_H
#define HOMESCREEN_UI_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void homescreen_ui_init(void);
void homescreen_ui_show(void);
void homescreen_ui_hide(void);
void homescreen_ui_show_alarm_button(void);
void homescreen_ui_hide_alarm_button(void);
void homescreen_ui_set_compact(bool compact);
void homescreen_ui_set_alarm_shift(bool shift);

#ifdef __cplusplus
}
#endif

#endif // HOMESCREEN_UI_H
