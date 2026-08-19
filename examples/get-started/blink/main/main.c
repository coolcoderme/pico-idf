#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_event.h"

static const char *TAG = "blink";

#ifndef CONFIG_BLINK_GPIO
#ifdef PICO_DEFAULT_LED_PIN
#define CONFIG_BLINK_GPIO PICO_DEFAULT_LED_PIN
#else
#define CONFIG_BLINK_GPIO PIDF_GPIO_WL_LED
#endif
#endif

void app_main(void)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(gpio_reset_pin(CONFIG_BLINK_GPIO));
    ESP_ERROR_CHECK(gpio_set_direction(CONFIG_BLINK_GPIO, GPIO_MODE_OUTPUT));

    ESP_LOGI(TAG, "pico-idf blink on GPIO %d", (int)CONFIG_BLINK_GPIO);

    int level = 0;
    while (1) {
        level = !level;
        ESP_ERROR_CHECK(gpio_set_level(CONFIG_BLINK_GPIO, (uint32_t)level));
        ESP_LOGI(TAG, "LED %s", level ? "on" : "off");
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
