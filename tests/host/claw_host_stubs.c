#include "driver/gpio.h"
#include "esp_log.h"

#include <stdarg.h>
#include <stdio.h>

int claw_host_last_pin = -1;
int claw_host_last_level = -1;

esp_err_t gpio_reset_pin(gpio_num_t gpio_num)
{
    (void)gpio_num;
    return ESP_OK;
}

esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
{
    (void)gpio_num;
    (void)mode;
    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    claw_host_last_pin = (int)gpio_num;
    claw_host_last_level = level ? 1 : 0;
    return ESP_OK;
}

int gpio_get_level(gpio_num_t gpio_num)
{
    if ((int)gpio_num == claw_host_last_pin) {
        return claw_host_last_level;
    }
    return 0;
}
