#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"
#include "esp_log.h"

#ifndef CONFIG_PIDF_APP_MAIN_STACK
#define CONFIG_PIDF_APP_MAIN_STACK 2048
#endif

#ifndef CONFIG_PIDF_APP_MAIN_PRIORITY
#define CONFIG_PIDF_APP_MAIN_PRIORITY 10
#endif

void app_main(void);

static const char *TAG = "pidf";

static void app_main_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "starting app_main on core %d", (int)portGET_CORE_ID());
    app_main();
    ESP_LOGW(TAG, "app_main returned; deleting task");
    vTaskDelete(NULL);
}

int main(void)
{
    stdio_init_all();
    BaseType_t ok = xTaskCreate(app_main_task, "app_main",
                                CONFIG_PIDF_APP_MAIN_STACK, NULL,
                                CONFIG_PIDF_APP_MAIN_PRIORITY, NULL);
    if (ok != pdPASS) {
        panic("failed to create app_main task");
    }
    vTaskStartScheduler();
    panic("scheduler returned");
}

void vApplicationMallocFailedHook(void)
{
    panic("FreeRTOS malloc failed");
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    panic("FreeRTOS stack overflow in %s", name ? name : "?");
}

static StaticTask_t idle_tcb[configNUMBER_OF_CORES];
static StackType_t idle_stack[configNUMBER_OF_CORES][configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **tcb, StackType_t **stack,
                                   uint32_t *size)
{
    *tcb = &idle_tcb[0];
    *stack = idle_stack[0];
    *size = configMINIMAL_STACK_SIZE;
}

#if (configNUMBER_OF_CORES > 1)
void vApplicationGetPassiveIdleTaskMemory(StaticTask_t **tcb, StackType_t **stack,
                                          uint32_t *size, BaseType_t index)
{
    /* index 0 is the first passive idle (core 1). */
    BaseType_t core = index + 1;
    if (core < 0 || core >= (BaseType_t)configNUMBER_OF_CORES) {
        core = 1;
    }
    *tcb = &idle_tcb[core];
    *stack = idle_stack[core];
    *size = configMINIMAL_STACK_SIZE;
}
#endif

static StaticTask_t timer_tcb;
static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **tcb, StackType_t **stack,
                                    uint32_t *size)
{
    *tcb = &timer_tcb;
    *stack = timer_stack;
    *size = configTIMER_TASK_STACK_DEPTH;
}
