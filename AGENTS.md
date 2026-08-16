# Agent guide for pico-idf

This repository is an **ESP-IDF-shaped** firmware framework for
Raspberry Pi Pico W and Pico 2 W. It is not ESP-IDF. Read
`docs/ARCHITECTURE.md` and `docs/VIBECODING.md` before changing code.

Vibe-coding is the **MCP server** at `tools/pidf_mcp.py` (also
`pidf.py mcp`). Prefer those tools over ad-hoc shell when they exist.

## Hard rules

- Do not vendor or copy ESP-IDF `.c` / proprietary headers into this tree.
- Do not mark a `tools/features.json` row `done` unless an example builds.
- Do not implement ESP-NOW, ESP-WIFI-MESH, SmartConfig, Thread/Zigbee,
  capacitive touch, TWAI, or eFuse as if the Pico had that silicon.
  Those rows are `impossible`; offer the documented substitute instead.
- Application entry is `void app_main(void)` on FreeRTOS. Keep it that way.
- Public APIs use ESP-IDF names when a row exists (`esp_err_t`, `ESP_LOGI`,
  `gpio_set_level`, future `esp_wifi_connect`). Pico-only APIs use a
  `pidf_` prefix (`pidf_onboard_led_set`, `PIDF_GPIO_WL_LED`).

## How to add a feature

1. MCP `vibe_next` (or `./tools/pidf.py vibe next`) — implement that id.
2. Create `components/<name>/` with `CMakeLists.txt` calling
   `idf_component_register`.
3. Add `examples/<group>/<name>/` that only includes public headers.
4. Host-test portable logic under `tests/`.
5. Cross-compile: `./tools/pidf.py set-target pico_w && ./tools/pidf.py build`
   and the same for `pico2_w`.
6. Update the feature row and this file if conventions change.

## Build environment

```bash
export PIDF_PATH=/workspace
export PICO_SDK_PATH=/opt/pico-sdk
export FREERTOS_KERNEL_PATH=/opt/FreeRTOS-Kernel
./tools/pidf.py set-target pico_w
./tools/pidf.py build
```

`.cursor/install.sh` is the idempotent Cloud Agent bootstrap.

## Style

- C11, 4-space indent, braces on the next line for functions (K&R / Pico SDK).
- `esp_err_t` returns. Log with `ESP_LOGx`.
- No secrets in the tree. Wi-Fi SSID/password come from `sdkconfig`.
