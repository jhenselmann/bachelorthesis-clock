#ifndef CONVERSATION_CONTROLLER_H
#define CONVERSATION_CONTROLLER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CONV_STATE_IDLE,
    CONV_STATE_CONNECTING,
    CONV_STATE_LISTENING,
    CONV_STATE_PROCESSING,
    CONV_STATE_RESPONSE,
    CONV_STATE_ERROR
} conversation_state_t;

esp_err_t conversation_controller_init(void);
esp_err_t conversation_controller_start(void);
void conversation_controller_stop_recording(void);
void conversation_controller_abort(const char *reason);
void conversation_controller_on_playback_complete(bool success);
conversation_state_t conversation_controller_get_state(void);
bool conversation_controller_is_active(void);

#ifdef __cplusplus
}
#endif

#endif // CONVERSATION_CONTROLLER_H
