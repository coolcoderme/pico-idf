# Vibe-coding pico-idf

Vibe-coding here means: describe the next ESP-IDF-shaped feature in
plain language, let an agent implement it against the matrix, then
rebuild. The framework is designed so that loop is safe.

## Loop

```text
pidf.py vibe status          # see what is done / planned / impossible
pidf.py vibe next            # copy the prompt for the next planned row
# agent implements the component + example + host test
pidf.py build                # cross-compile for the current target
pidf.py vibe status          # row should now read "done" or "partial"
```

Rules the agent must follow (also in `AGENTS.md`):

1. Read `tools/features.json` before writing code. Do not invent a
   parallel API if an ESP-IDF name already exists.
2. Implement the Pico backend. Do not copy ESP-IDF `.c` files.
3. Add or extend an example under `examples/` that uses only public
   headers.
4. Add a host test when the logic is portable (parsers, NVS encoding,
   HTTP framing). Skip host tests that need the Pico SDK.
5. Cross-compile at least `examples/get-started/blink` plus the new
   example for `pico_w` and `pico2_w`.
6. Flip the feature row to `done` or `partial`. Never mark
   `impossible` as `done`.
7. Keep `app_main` as the application entry. Do not replace FreeRTOS
   with a bare `while (1)` poll loop unless the feature is explicitly
   a no-RTOS bring-up sample.

## Prompt shape that works

Good:

> Implement `nvs_flash` for pico-idf. Match `nvs_flash_init`,
> `nvs_open`, `nvs_set_u32`, `nvs_get_u32`, `nvs_commit`, `nvs_close`.
> Store records in the last 16 KB of on-board flash. Add
> `examples/storage/nvs_rw` and a host test for the record codec.
> Update `tools/features.json`.

Bad:

> Add storage. (too vague)
> Port ESP-IDF nvs_flash.c. (do not vendor Espressif sources)
> Make Wi-Fi work like ESP-NOW. (marked impossible)

## Project a human (or agent) types

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "blink";

void app_main(void)
{
    gpio_reset_pin(CONFIG_BLINK_GPIO);
    gpio_set_direction(CONFIG_BLINK_GPIO, GPIO_MODE_OUTPUT);
    while (1) {
        gpio_set_level(CONFIG_BLINK_GPIO, 1);
        ESP_LOGI(TAG, "LED on");
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_set_level(CONFIG_BLINK_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
```

That is valid pico-idf. On Pico W, `CONFIG_BLINK_GPIO` defaults to the
CYW43439 LED virtual pin (`PIDF_GPIO_WL_LED`).

## What to tell the agent about hardware

- Pico W LED is **not** a CPU GPIO. It sits on the CYW43439. The GPIO
  shim maps `PIDF_GPIO_WL_LED` (32) onto `cyw43_arch_gpio_put`.
- Pico 2 W is the same radio, more SRAM, M33 (or Hazard3).
- Never busy-wait in `app_main` when FreeRTOS is running; use
  `vTaskDelay`.
- Wi-Fi and the onboard LED on W boards both need `cyw43_arch_init`
  (the GPIO shim does this lazily).

## Cursor / Cloud Agent setup

`.cursor/install.sh` installs the ARM toolchain, Pico SDK, FreeRTOS
Kernel, and picotool, then configures a blink build. After install:

```bash
export PIDF_PATH=/workspace
export PICO_SDK_PATH=/opt/pico-sdk
export FREERTOS_KERNEL_PATH=/opt/FreeRTOS-Kernel
./tools/pidf.py set-target pico_w
./tools/pidf.py build
```
