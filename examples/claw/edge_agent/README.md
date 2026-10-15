# ESP-Claw-shaped edge agent

Default pico-idf firmware. Clean-room Pico port of the **ideas** behind
[ESP-Claw](https://github.com/espressif/esp-claw) (chat-as-creation,
capabilities, events, memory, MCP). This is **not** ESP-Claw firmware
and does not vendor Espressif sources.

Pico W / Pico 2 W do not have the flash/PSRAM budget for Lua + IM +
FATFS + a cloud LLM. This example is the subset that fits:

- capability registry (`gpio`, `system`, `memory`, `event`, `agent`, …)
- event bus + router rules (`led.on` → GPIO 32)
- RAM session memory
- local keyword agent (`led on`, `remember …`)
- FreeRTOS-timer jobs
- USB-CDC `claw>` REPL (lines starting with `{` are device MCP JSON-RPC)

## Build

```bash
export PIDF_PATH=/path/to/pico-idf
export PICO_SDK_PATH=/opt/pico-sdk
export FREERTOS_KERNEL_PATH=/opt/FreeRTOS-Kernel
export PIDF_PROJECT=$PIDF_PATH/examples/claw/edge_agent

./tools/pidf.py set-target pico_w   # or pico2_w
./tools/pidf.py build
./tools/pidf.py flash
./tools/pidf.py monitor
```

Type `help` at the `claw>` prompt. `led on` drives the CYW43439 LED
(virtual GPIO 32).

See [docs/CLAW.md](../../../docs/CLAW.md).
