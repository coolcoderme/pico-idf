#include "claw_priv.h"

#ifndef PIDF_HOST_TEST
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static SemaphoreHandle_t s_lock;
#endif

void claw_lock_init(void)
{
#ifndef PIDF_HOST_TEST
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
    }
#endif
}

void claw_lock(void)
{
#ifndef PIDF_HOST_TEST
    if (s_lock != NULL) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
    }
#endif
}

void claw_unlock(void)
{
#ifndef PIDF_HOST_TEST
    if (s_lock != NULL) {
        xSemaphoreGive(s_lock);
    }
#endif
}

uint32_t claw_uptime_ms(void)
{
#ifdef PIDF_HOST_TEST
    return 0;
#else
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
#endif
}

size_t claw_free_heap(void)
{
#ifdef PIDF_HOST_TEST
    return 65536;
#else
    return (size_t)xPortGetFreeHeapSize();
#endif
}
