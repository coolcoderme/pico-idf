#include "claw/claw.h"
#include "claw_priv.h"

#include "esp_log.h"

static const char *TAG = "claw";
static int s_ready;

void claw_builtins_register(void);

esp_err_t claw_runtime_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }
    claw_lock_init();
    claw_builtins_register();
    s_ready = 1;
    ESP_LOGI(TAG, "runtime ready, %d caps (ESP-Claw-shaped, Pico subset)", claw_cap_count());
    return ESP_OK;
}
