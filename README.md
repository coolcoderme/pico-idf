# pico-idf

ESP-IDF-shaped firmware framework for the **Raspberry Pi Pico W** and
**Pico 2 W**, including **FreeRTOS SMP** and a **vibe-coding** loop for
the rest of the ESP-IDF feature surface.

ESP-IDF itself cannot run on RP2040 or RP2350. pico-idf is the
practical plan: Pico SDK as the HAL, FreeRTOS as the RTOS, ESP-IDF
names as the API, and an honest matrix for everything else.

```c
void app_main(void)
{
    gpio_reset_pin(CONFIG_BLINK_GPIO);
    gpio_set_direction(CONFIG_BLINK_GPIO, GPIO_MODE_OUTPUT);
    while (1) {
        gpio_set_level(CONFIG_BLINK_GPIO, 1);
        ESP_LOGI("blink", "LED on");
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_set_level(CONFIG_BLINK_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
```

That is the same shape as an ESP-IDF app. On Pico W, pin 32 is the
CYW43439 onboard LED.

## Plan (short)

| Layer | What we use |
|---|---|
| HAL / radio | Raspberry Pi Pico SDK (CYW43439, lwIP, BTstack, TinyUSB, mbedTLS) |
| RTOS | FreeRTOS SMP (RP2040, RP2350 ARM, RP2350 RISC-V) |
| API | ESP-IDF-shaped components (`esp_err`, `esp_log`, `gpio`, later `esp_wifi` / NVS / MQTT…) |
| CLI | `pidf.py` — `set-target`, `build`, `flash`, `monitor`, `vibe` |
| Backlog | `tools/features.json` — every remaining ESP-IDF feature is one agent slice |

Full write-up: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).
Feature inventory: [docs/FEATURES.md](docs/FEATURES.md).
Vibe-coding loop: [docs/VIBECODING.md](docs/VIBECODING.md).
What will never be 1:1: [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md).

## Quick start

```bash
export PIDF_PATH=$PWD
export PICO_SDK_PATH=/opt/pico-sdk
export FREERTOS_KERNEL_PATH=/opt/FreeRTOS-Kernel

./tools/pidf.py set-target pico_w    # or pico2_w, pico2_w_riscv
./tools/pidf.py build
./tools/pidf.py flash                 # picotool, or copy the .uf2
./tools/pidf.py monitor
```

On Debian/Ubuntu, `.cursor/install.sh` installs the ARM toolchain, Pico
SDK 2.3.0, FreeRTOS-Kernel, and picotool.

## Targets

- `pico_w` — RP2040 + CYW43439
- `pico2_w` — RP2350 ARM + CYW43439
- `pico2_w_riscv` — same board, Hazard3 RISC-V cores
- `pico` / `pico2` — no radio, bring-up only

## Vibe-coding the rest of ESP-IDF

```bash
./tools/pidf.py vibe status   # done / partial / planned / impossible
./tools/pidf.py vibe next     # prompt for the next planned feature
```

Agents implement one `planned` row at a time (component + example +
test + matrix update). See `AGENTS.md`.

## Status of this tree

Foundation is in:

- `pidf.py` and the component CMake project
- FreeRTOS SMP boot into `app_main`
- `esp_err`, `esp_log`, `esp_event` (subset), `driver/gpio`
- `examples/get-started/blink`

Wi-Fi, NVS, HTTP, MQTT, BLE, OTA, and the rest are `planned` rows —
that is intentional. The plan is to finish them through the vibe loop
instead of pretending a full ESP-IDF port exists today.
