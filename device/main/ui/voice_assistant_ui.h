// Dispatches UI calls to active scenario (A or B)

#ifndef VOICE_ASSISTANT_UI_H
#define VOICE_ASSISTANT_UI_H

#include <stdint.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void voice_assistant_ui_init(void);
void voice_assistant_ui_show_idle(void);
void voice_assistant_ui_show_listening(void);
void voice_assistant_ui_show_processing(void);
void voice_assistant_ui_show_transcript(const char *text);
void voice_assistant_ui_show_response(void);
void voice_assistant_ui_show_error(const char *message);
void voice_assistant_ui_hide(void);
void voice_assistant_ui_set_scenario(uint8_t scenario);
uint8_t voice_assistant_ui_get_scenario(void);

#ifdef __cplusplus
}
#endif

#endif // VOICE_ASSISTANT_UI_H
