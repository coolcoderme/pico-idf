# pico-idf

ESP-Claw-shaped edge agent for the **Raspberry Pi Pico W** and
**Pico 2 W**: capabilities, events, on-device memory, a local agent,
a USB `claw>` REPL, and a vibe-coding MCP server.

This is **not** ESP-IDF and **not** a port of Espressif's
[ESP-Claw](https://github.com/espressif/esp-claw) sources. The feature
matrix is claw-only.

```c
void app_main(void)
{
    ESP_ERROR_CHECK(claw_runtime_init());
    ESP_ERROR_CHECK(claw_repl_start());
}
```

On Pico W, GPIO 32 is the CYW43439 onboard LED (`PIDF_GPIO_WL_LED`).

## Plan (short)

| Layer | What we use |
|---|---|
| HAL / radio | Raspberry Pi Pico SDK (CYW43439, lwIP, TinyUSB, mbedTLS) |
| RTOS | FreeRTOS SMP (RP2040, RP2350 ARM, RP2350 RISC-V) |
| Product | ESP-Claw-shaped `components/claw` — see [docs/CLAW.md](docs/CLAW.md) |
| CLI | `pidf.py` — `set-target`, `build`, `flash`, `monitor`, `vibe`, `mcp` |
| MCP | `tools/pidf_mcp.py` — Cursor vibe-coding tools / resources / prompts |
| Backlog | `tools/features.json` — remaining **ESP-Claw** rows only |

Full write-up: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).
Feature inventory: [docs/FEATURES.md](docs/FEATURES.md).
Vibe-coding loop: [docs/VIBECODING.md](docs/VIBECODING.md).
ESP-Claw mapping: [docs/CLAW.md](docs/CLAW.md).
What will never be 1:1: [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md).

## Quick start

```bash
export PIDF_PATH=$PWD
export PICO_SDK_PATH=/opt/pico-sdk
export FREERTOS_KERNEL_PATH=/opt/FreeRTOS-Kernel
export PIDF_PROJECT=$PIDF_PATH/examples/claw/edge_agent

./tools/pidf.py set-target pico_w    # or pico2_w, pico2_w_riscv
./tools/pidf.py build
./tools/pidf.py flash                 # picotool, or copy the .uf2
./tools/pidf.py monitor
```

On Debian/Ubuntu, `.cursor/install.sh` installs the ARM toolchain, Pico
SDK 2.3.0, FreeRTOS-Kernel, and picotool.

USB-CDC prompt:

```
claw> help
claw> led on
claw> agent remember picnic
claw> cap list
```

## Targets

- `pico_w` — RP2040 + CYW43439
- `pico2_w` — RP2350 ARM + CYW43439
- `pico2_w_riscv` — same board, Hazard3 RISC-V cores
- `pico` / `pico2` — no radio, bring-up only

## Vibe-coding (MCP server)

Cursor loads [`.cursor/mcp.json`](.cursor/mcp.json) and talks to
`tools/pidf_mcp.py` over stdio. The host calls `vibe_next`,
`scaffold_feature`, `set_target`, and `build` for the next **claw**
row (`claw-lua`, `claw-im`, …).

```bash
./tools/pidf.py mcp            # stdio MCP server
./tools/pidf.py vibe status    # same matrix, CLI form
```

See [docs/VIBECODING.md](docs/VIBECODING.md).

## Status of this tree

Ships today:

- `pidf.py` and the component CMake project
- FreeRTOS SMP boot into `app_main`
- ESP-Claw-shaped stack: `components/claw` +
  `examples/claw/edge_agent` (caps, events, RAM memory, local agent,
  USB REPL / device MCP)

Still `planned` on the claw matrix: Lua, IM, cloud LLM, skills UI.
ESP-IDF APIs are out of scope.
