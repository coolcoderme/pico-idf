# Agent guide for pico-idf

This repository is an **ESP-Claw-shaped** edge agent for Raspberry Pi
Pico W and Pico 2 W. It is not ESP-IDF and not a vendor of ESP-Claw.
Read `docs/CLAW.md`, `docs/ARCHITECTURE.md`, and `docs/VIBECODING.md`
before changing code.

The feature matrix (`tools/features.json`) lists **claw rows only**.
Do not add ESP-IDF features (`nvs`, `wifi`, `mqtt`, `gpio` as IDF, …).

Vibe-coding is the **MCP server** at `tools/pidf_mcp.py` (also
`pidf.py mcp`). Prefer those tools over ad-hoc shell when they exist.

## Hard rules

- Do not vendor or copy ESP-Claw `.c` / proprietary headers into this tree.
  `components/claw` is a Pico-sized clean-room subset; see `docs/CLAW.md`.
- Do not vendor or copy ESP-IDF sources. Do not reintroduce ESP-IDF
  feature rows.
- Do not mark a `tools/features.json` row `done` unless an example builds.
- Application entry is `void app_main(void)` on FreeRTOS. Keep it that way.
- Public product APIs are `claw_*`. Pico-only helpers use a `pidf_`
  prefix (`pidf_onboard_led_set`, `PIDF_GPIO_WL_LED`).

## How to add a feature

1. MCP `vibe_next` (or `./tools/pidf.py vibe next`) — implement that
   `claw-*` id.
2. Extend `components/claw/` (do not create a parallel ESP-IDF component).
3. Keep `examples/claw/edge_agent` working; add a focused example only
   if the row needs one.
4. Host-test portable logic under `tests/`.
5. Cross-compile: `./tools/pidf.py set-target pico_w && ./tools/pidf.py build`
   and the same for `pico2_w`.
6. Update the feature row and this file if conventions change.

## Build environment

```bash
export PIDF_PATH=/workspace
export PICO_SDK_PATH=/opt/pico-sdk
export FREERTOS_KERNEL_PATH=/opt/FreeRTOS-Kernel
export PIDF_PROJECT=$PIDF_PATH/examples/claw/edge_agent
./tools/pidf.py set-target pico_w
./tools/pidf.py build
```

`.cursor/install.sh` is the idempotent Cloud Agent bootstrap.

## Style

- C11, 4-space indent, braces on the next line for functions (K&R / Pico SDK).
- `esp_err_t` returns. Log with `ESP_LOGx`.
- No secrets in the tree.
