// High-level UI control (homescreen, voice assistant states)

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_manager_init(void);
void ui_show_homescreen(void);
void ui_show_listening(void);
void ui_show_processing(void);
void ui_show_transcript(const char *text);
void ui_show_response(void);
void ui_show_error(const char *message);

#ifdef __cplusplus
}
#endif

#endif // UI_MANAGER_H
