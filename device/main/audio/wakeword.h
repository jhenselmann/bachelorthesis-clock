#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void wakeword_init(void);
void wakeword_start(void);
void wakeword_pause(void);
void wakeword_resume(void);
void wakeword_trigger_manual(void);
