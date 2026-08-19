#include "driver/gpio.h"

#include "hardware/gpio.h"

#if defined(PIDF_CYW43) || defined(CYW43_WL_GPIO_LED_PIN)
#include "pico/cyw43_arch.h"
#define PIDF_HAS_CYW43 1
#endif

#ifdef PIDF_HAS_CYW43
static int s_cyw43_ready;

static esp_err_t ensure_cyw43(void)
{
    if (s_cyw43_ready) {
        return ESP_OK;
    }
    if (cyw43_arch_init() != 0) {
        return ESP_FAIL;
    }
    s_cyw43_ready = 1;
    return ESP_OK;
}
#endif

static int is_cpu_gpio(gpio_num_t n)
{
    return n >= 0 && n < NUM_BANK0_GPIOS;
}

static int is_wl_led(gpio_num_t n)
{
    return n == PIDF_GPIO_WL_LED;
}

esp_err_t gpio_reset_pin(gpio_num_t gpio_num)
{
#ifdef PIDF_HAS_CYW43
    if (is_wl_led(gpio_num)) {
        return ensure_cyw43();
    }
#endif
    if (!is_cpu_gpio(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_init((uint)gpio_num);
    gpio_set_dir((uint)gpio_num, GPIO_IN);
    gpio_disable_pulls((uint)gpio_num);
    return ESP_OK;
}

esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
{
#ifdef PIDF_HAS_CYW43
    if (is_wl_led(gpio_num)) {
        return (mode == GPIO_MODE_OUTPUT || mode == GPIO_MODE_INPUT_OUTPUT)
                   ? ensure_cyw43()
                   : ESP_ERR_NOT_SUPPORTED;
    }
#endif
    if (!is_cpu_gpio(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_init((uint)gpio_num);
    switch (mode) {
    case GPIO_MODE_INPUT:
        gpio_set_dir((uint)gpio_num, GPIO_IN);
        return ESP_OK;
    case GPIO_MODE_OUTPUT:
    case GPIO_MODE_OUTPUT_OD:
    case GPIO_MODE_INPUT_OUTPUT:
    case GPIO_MODE_INPUT_OUTPUT_OD:
        gpio_set_dir((uint)gpio_num, GPIO_OUT);
        return ESP_OK;
    case GPIO_MODE_DISABLE:
        return ESP_OK;
    default:
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
#ifdef PIDF_HAS_CYW43
    if (is_wl_led(gpio_num)) {
        esp_err_t err = ensure_cyw43();
        if (err != ESP_OK) {
            return err;
        }
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, level ? 1 : 0);
        return ESP_OK;
    }
#endif
    if (!is_cpu_gpio(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_put((uint)gpio_num, level ? 1 : 0);
    return ESP_OK;
}

int gpio_get_level(gpio_num_t gpio_num)
{
#ifdef PIDF_HAS_CYW43
    if (is_wl_led(gpio_num)) {
        if (ensure_cyw43() != ESP_OK) {
            return 0;
        }
        return cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN);
    }
#endif
    if (!is_cpu_gpio(gpio_num)) {
        return 0;
    }
    return gpio_get((uint)gpio_num);
}

esp_err_t gpio_config(const gpio_config_t *cfg)
{
    if (cfg == NULL || cfg->pin_bit_mask == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    for (int pin = 0; pin < 64; pin++) {
        if ((cfg->pin_bit_mask & (1ULL << pin)) == 0) {
            continue;
        }
        esp_err_t err = gpio_set_direction((gpio_num_t)pin, cfg->mode);
        if (err != ESP_OK) {
            return err;
        }
        if (cfg->pull_up_en) {
            gpio_pullup_en((gpio_num_t)pin);
        } else {
            gpio_pullup_dis((gpio_num_t)pin);
        }
        if (cfg->pull_down_en) {
            gpio_pulldown_en((gpio_num_t)pin);
        } else {
            gpio_pulldown_dis((gpio_num_t)pin);
        }
        (void)cfg->intr_type;
    }
    return ESP_OK;
}

esp_err_t gpio_pullup_en(gpio_num_t gpio_num)
{
    if (is_wl_led(gpio_num)) {
        return ESP_OK;
    }
    if (!is_cpu_gpio(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_pull_up((uint)gpio_num);
    return ESP_OK;
}

esp_err_t gpio_pullup_dis(gpio_num_t gpio_num)
{
    if (is_wl_led(gpio_num) || !is_cpu_gpio(gpio_num)) {
        return is_wl_led(gpio_num) ? ESP_OK : ESP_ERR_INVALID_ARG;
    }
    gpio_disable_pulls((uint)gpio_num);
    return ESP_OK;
}

esp_err_t gpio_pulldown_en(gpio_num_t gpio_num)
{
    if (is_wl_led(gpio_num)) {
        return ESP_OK;
    }
    if (!is_cpu_gpio(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_pull_down((uint)gpio_num);
    return ESP_OK;
}

esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num)
{
    if (is_wl_led(gpio_num) || !is_cpu_gpio(gpio_num)) {
        return is_wl_led(gpio_num) ? ESP_OK : ESP_ERR_INVALID_ARG;
    }
    gpio_disable_pulls((uint)gpio_num);
    return ESP_OK;
}

esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull)
{
    switch (pull) {
    case GPIO_PULLUP_ONLY:
        return gpio_pullup_en(gpio_num);
    case GPIO_PULLDOWN_ONLY:
        return gpio_pulldown_en(gpio_num);
    case GPIO_FLOATING:
        gpio_pullup_dis(gpio_num);
        return gpio_pulldown_dis(gpio_num);
    case GPIO_PULLUP_PULLDOWN:
        return ESP_ERR_NOT_SUPPORTED;
    default:
        return ESP_ERR_INVALID_ARG;
    }
}
