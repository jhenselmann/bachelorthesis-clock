// Mutex helper macros
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define MUTEX_LOCK(mutex) \
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)

#define MUTEX_UNLOCK(mutex) \
    xSemaphoreGive(mutex)

#define MUTEX_TAKE(mutex) \
    xSemaphoreTake(mutex, portMAX_DELAY)

#define MUTEX_GIVE(mutex) \
    xSemaphoreGive(mutex)
