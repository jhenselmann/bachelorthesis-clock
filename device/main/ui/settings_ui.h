// Settings overlay (long-press on homescreen)

#ifndef SETTINGS_UI_H
#define SETTINGS_UI_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void settings_ui_init(void);
void settings_ui_show(void);
void settings_ui_hide(void);
bool settings_ui_is_visible(void);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_UI_H
