// Interface for UI scenarios (A = minimal overlay, B = ring + transcript)

#ifndef UI_SCENARIO_H
#define UI_SCENARIO_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*init)(void);
    void (*show_idle)(void);
    void (*show_listening)(void);
    void (*show_processing)(void);
    void (*show_transcript)(const char *text);
    void (*show_response)(void);
    void (*show_error)(const char *message);
    void (*hide)(void);
} ui_scenario_funcs_t;

const ui_scenario_funcs_t* ui_scenario_a_get_funcs(void);
const ui_scenario_funcs_t* ui_scenario_b_get_funcs(void);

#ifdef __cplusplus
}
#endif

#endif
