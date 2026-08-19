# pico-idf architecture

**pico-idf** is an **ESP-Claw-shaped** edge-agent framework for the
Raspberry Pi Pico W (RP2040 + CYW43439) and Pico 2 W (RP2350 + CYW43439).
It is not ESP-IDF, and it is not Espressif's [ESP-Claw](https://github.com/espressif/esp-claw)
compiled for a Pico.

ESP-Claw's documented boards want on the order of 8 MB flash + 8 MB
PSRAM, Lua, IM channels, and an ESP-IDF component tree. Pico W has
264 KB SRAM; Pico 2 W has about 520 KB. The workable plan is a
**clean-room Pico subset**:

1. Use the official Pico SDK as the HAL and wireless driver.
2. Use FreeRTOS SMP so the claw runtime can schedule events, REPL, and
   later IM/LLM work as tasks.
3. Expose `claw_*` APIs inspired by ESP-Claw public docs (caps, events,
   memory, agent, scheduler, device MCP, USB REPL).
4. Keep an honest claw-only feature matrix: native, subset, planned, or
   impossible on this silicon / RAM.
5. Expose that backlog through an MCP server (`tools/pidf_mcp.py`) so
   Cursor can vibe-code one `claw-*` row at a time.

```
  app_main()  +  components/claw
                 (caps, events, memory, agent, sched, REPL, MCP)
                         │
          ┌──────────────┼──────────────────┐
          │              │                  │
     FreeRTOS SMP    Pico SDK HAL     CYW43439 + lwIP
     (dual core)     GPIO/USB         mbedTLS (later IM/LLM)
          │              │                  │
     RP2040 (Pico W)  or  RP2350 ARM / RISC-V (Pico 2 W)
```

ESP-IDF-shaped public APIs (`esp_wifi`, `nvs`, `mqtt`, …) are **not**
part of this product. Thin `esp_err_t` / logging / GPIO helpers exist
only as private infrastructure for claw.

## Why this beats the alternatives

| Approach | Verdict |
|---|---|
| Compile ESP-Claw / ESP-IDF for RP2040/RP2350 | Impossible. Different CPU, memory map, radio, and RAM budget. |
| Full ESP-IDF-shaped Pico port | Out of scope. This repo's matrix is claw-only. |
| Bare Pico SDK | Required HAL, but no agent loop, caps, or USB claw REPL. |
| **pico-idf (this repo)** | Pico SDK + FreeRTOS SMP + ESP-Claw-shaped `components/claw`. |

## Targets

| `pidf.py set-target` | Chip | Cores | Radio | Notes |
|---|---|---|---|---|
| `pico_w` | RP2040 | 2× M0+ | CYW43439 Wi-Fi + BT | Default wireless target |
| `pico2_w` | RP2350 | 2× M33 | CYW43439 Wi-Fi + BT | Default Pico 2 W (ARM) |
| `pico2_w_riscv` | RP2350 | 2× Hazard3 | CYW43439 | Same board, RISC-V cores |
| `pico` / `pico2` | RP2040 / RP2350 | 2 | none | Bring-up without wireless |

## Layers

### 1. CLI (`tools/pidf.py`)

Build and vibe helpers for the claw example:

| Command | Purpose |
|---|---|
| `pidf.py set-target` | Select Pico W / Pico 2 W / RISC-V |
| `pidf.py build` | CMake + Ninja/Make |
| `pidf.py flash` | `picotool` or UF2 copy |
| `pidf.py monitor` | USB-CDC serial (`claw>` REPL) |
| `pidf.py menuconfig` | Edit `sdkconfig` |
| `pidf.py create-project` | New `app_main` app that `REQUIRES claw` |
| `pidf.py vibe status` | Claw feature matrix |
| `pidf.py vibe next` | Agent prompt for the next `claw-*` row |
| `pidf.py mcp` | MCP stdio server (Cursor vibe-coding) |

`PIDF_PATH` points at this repository. Default project is
`examples/claw/edge_agent`.

### 2. Build system

CMake. A project looks like:

```cmake
cmake_minimum_required(VERSION 3.13)
include($ENV{PIDF_PATH}/tools/cmake/project.cmake)
project(edge_agent)
```

`idf_component_register(SRCS … INCLUDE_DIRS … REQUIRES …)` registers
libraries. The template adds every component under `components/` plus
the app's `main/`. Claw apps must `REQUIRES claw`.

### 3. RTOS (FreeRTOS SMP)

- Official RP2040 / RP2350 ARM / RP2350 RISC-V SMP ports.
- `main()` starts the scheduler and a pinned `app_main` task.
- Claw scheduler jobs are FreeRTOS software timers that post events.

### 4. Claw runtime

`components/claw` is the product:

- Public headers under `include/claw/` (`claw.h`, `cap.h`, `event.h`, …)
- Built-in caps: `gpio`, `system`, `time`, `memory`, `event`, `agent`,
  `sched`, plus stubs `lua` and `im`
- USB REPL and JSON-RPC device MCP
- A row in `tools/features.json` for each ESP-Claw idea

Do **not** vendor ESP-Claw or ESP-IDF sources. Clean-room headers plus
Pico implementations keep licensing simple (this repo is MIT; Pico SDK
is BSD-3; FreeRTOS is MIT).

Details: [CLAW.md](CLAW.md).

### 5. Wireless (later claw rows)

IM and cloud LLM need CYW43439 + lwIP + mbedTLS from the Pico SDK.
Those are backends for `claw-im` / `claw-llm`, not ESP-IDF `esp_wifi`
or `esp_http_client` components.

## What the claw matrix means

The inventory in `tools/features.json` is the contract. Every row is
an ESP-Claw idea:

- **done** — `claw_*` API works on Pico W / Pico 2 W.
- **partial** — common subset works; extras are listed.
- **planned** — Pico analogue is straightforward; not wrapped yet.
- **impossible** — no RAM, no silicon, or a full ESP-Claw module cannot
  fit. The row says what to use instead.

## Non-goals

- Porting ESP-IDF APIs or examples onto Pico.
- Running ESP-Claw binaries or Espressif Wi-Fi firmware on a Pico.
- Hosting ESP-Claw's full Lua + IM + PSRAM board-manager stack on Pico W.
- Replacing MicroPython for existing Pico scripts.
