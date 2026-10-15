#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "hal/gpio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t pin_bit_mask;
    gpio_mode_t mode;
    gpio_pullup_t pull_up_en;
    gpio_pulldown_t pull_down_en;
    gpio_int_type_t intr_type;
} gpio_config_t;

esp_err_t gpio_config(const gpio_config_t *cfg);
esp_err_t gpio_reset_pin(gpio_num_t gpio_num);
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);
int gpio_get_level(gpio_num_t gpio_num);
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode);
esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull);
esp_err_t gpio_pullup_en(gpio_num_t gpio_num);
esp_err_t gpio_pullup_dis(gpio_num_t gpio_num);
esp_err_t gpio_pulldown_en(gpio_num_t gpio_num);
esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num);

#ifdef __cplusplus
}
#endif
