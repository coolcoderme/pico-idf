#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "claw/claw.h"

static const char *TAG = "edge_agent";

void app_main(void)
{
    ESP_ERROR_CHECK(claw_runtime_init());
    ESP_ERROR_CHECK(claw_event_add_rule(
        "led.on", "gpio", "{\"pin\":32,\"level\":1}"));
    ESP_ERROR_CHECK(claw_event_add_rule(
        "led.off", "gpio", "{\"pin\":32,\"level\":0}"));
    ESP_ERROR_CHECK(claw_sched_every_ms("heartbeat", 10000, "heartbeat"));
    ESP_ERROR_CHECK(claw_repl_start());

    ESP_LOGI(TAG, "pico-idf claw edge agent; USB-CDC prompt is claw>");
    ESP_LOGI(TAG, "try: led on | agent remember picnic | cap list");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
