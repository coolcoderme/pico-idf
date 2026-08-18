# pico-idf architecture

**pico-idf** is an ESP-IDF-shaped firmware framework for the Raspberry Pi
Pico W (RP2040 + CYW43439) and Pico 2 W (RP2350 + CYW43439). It is **not**
ESP-IDF compiled for a Pico. Espressif's SDK is welded to Espressif silicon
(Wi-Fi MAC blobs, ROM helpers, Xtensa / ESP-RISC-V toolchains, eFuse, the
ESP partition table). That binary does not run on RP2040 or RP2350.

The workable plan is a **compatibility framework**:

1. Use the official Pico SDK as the HAL and wireless driver.
2. Use FreeRTOS SMP as the RTOS, the same programming model ESP-IDF uses.
3. Expose ESP-IDF-shaped APIs (`esp_err_t`, `ESP_LOGI`, `app_main`,
   `gpio_*`, `esp_wifi_*`, `nvs_*`, `esp_event_*`, `idf.py`-like CLI) so
   existing ESP-IDF knowledge and vibe-coding prompts transfer.
4. Keep an honest feature matrix: native, subset, planned, or impossible.
5. Expose the backlog through an MCP server (`tools/pidf_mcp.py`) so
   Cursor can vibe-code one feature at a time.

```
  app_main()  +  ESP-IDF-shaped components
                 (esp_wifi, nvs, mqtt, driver, …)
                         │
                 pico-idf shims + event loop
                         │
          ┌──────────────┼──────────────────┐
          │              │                  │
     FreeRTOS SMP    Pico SDK HAL     CYW43439 + lwIP
     (dual core)     GPIO/PIO/USB     BTstack / mbedTLS
          │              │                  │
     RP2040 (Pico W)  or  RP2350 ARM / RISC-V (Pico 2 W)
```

## Why this beats the alternatives

| Approach | Verdict |
|---|---|
| Compile ESP-IDF for RP2040/RP2350 | Impossible. Different CPU, memory map, radio, boot ROM, and closed Wi-Fi MAC. |
| Zephyr / NuttX on Pico | Real RTOSes, but not the ESP-IDF API surface people already know. |
| Arduino-Pico | High-level and complete, not FreeRTOS-first, not `idf.py`-shaped. |
| Bare Pico SDK | Required HAL, but no component model, NVS, OTA, or ESP-style Wi-Fi API. |
| **pico-idf (this repo)** | Pico SDK + FreeRTOS SMP + ESP-IDF-shaped APIs + vibe-coding loop. |

## Targets

| `pidf.py set-target` | Chip | Cores | Radio | Notes |
|---|---|---|---|---|
| `pico_w` | RP2040 | 2× M0+ | CYW43439 Wi-Fi + BT | Default wireless target |
| `pico2_w` | RP2350 | 2× M33 | CYW43439 Wi-Fi + BT | Default Pico 2 W (ARM) |
| `pico2_w_riscv` | RP2350 | 2× Hazard3 | CYW43439 | Same board, RISC-V cores |
| `pico` / `pico2` | RP2040 / RP2350 | 2 | none | Bring-up without wireless |

Pico 2 W RISC-V is a first-class target because ESP-IDF already trains people
on RISC-V (ESP32-C3/C6). The ISAs are **not** binary-compatible (Hazard3 vs
Espressif RV32); only the source-level ESP-IDF-shaped APIs carry over.

## Layers

### 1. CLI (`tools/pidf.py`)

Mirrors `idf.py` so muscle memory and LLM training data work:

| Command | ESP-IDF analogue | Purpose |
|---|---|---|
| `pidf.py set-target` | `idf.py set-target` | Select Pico W / Pico 2 W / RISC-V |
| `pidf.py build` | `idf.py build` | CMake + Ninja/Make |
| `pidf.py flash` | `idf.py flash` | `picotool` or UF2 copy |
| `pidf.py monitor` | `idf.py monitor` | USB-CDC serial |
| `pidf.py menuconfig` | `idf.py menuconfig` | Edit `sdkconfig` |
| `pidf.py create-project` | `idf.py create-project` | New app under `app_main` |
| `pidf.py create-component` | `idf.py create-component` | New component skeleton |
| `pidf.py vibe status` | — | Next unimplemented feature |
| `pidf.py vibe next` | — | Agent prompt for that feature |
| `pidf.py mcp` | — | MCP stdio server (Cursor vibe-coding) |

`PIDF_PATH` points at this repository, the same way `IDF_PATH` points at
ESP-IDF.

### 2. Build system

CMake, not Make. A project looks like an ESP-IDF project:

```cmake
cmake_minimum_required(VERSION 3.13)
include($ENV{PIDF_PATH}/tools/cmake/project.cmake)
project(blink)
```

`idf_component_register(SRCS … INCLUDE_DIRS … REQUIRES …)` registers
libraries. The project template adds every component under
`components/` plus the app's `main/`.

### 3. RTOS (FreeRTOS SMP)

ESP-IDF is FreeRTOS. pico-idf is FreeRTOS.

- Official RP2040 / RP2350 ARM / RP2350 RISC-V SMP ports from
  FreeRTOS-Kernel.
- `configNUMBER_OF_CORES=2`, core affinity, Pico sync/time interop so
  SDK mutexes and `sleep_ms` stay legal next to tasks.
- `main()` starts the scheduler and a pinned `app_main` task — the same
  boot contract as ESP-IDF.
- Wi-Fi on W boards uses `pico_cyw43_arch_lwip_sys_freertos` so the
  CYW43439 driver and lwIP run as FreeRTOS tasks, not a poll loop.

Pico-specific extras (PIO state machines, the second core as a hard
realtime island, Hazard3) stay available. They are not hidden; they are
documented as Pico extensions beside the ESP-IDF-shaped APIs.

### 4. Compatibility shims

Each ESP-IDF component becomes a pico-idf component with:

- Public headers that match the ESP-IDF names developers already type.
- A Pico backend (SDK, lwIP, BTstack, flash, PIO).
- A row in `tools/features.json` (`done` / `partial` / `planned` /
  `impossible`).
- An example and a vibe-coding recipe.

Do **not** vendor ESP-IDF or ESP-Claw sources. Clean-room headers plus Pico
implementations keep licensing simple (this repo is MIT; Pico SDK is
BSD-3; FreeRTOS is MIT).

### 5. Wireless

Both W boards share the Infineon CYW43439. Pico SDK already ships:

- `cyw43_driver` over PIO SPI
- lwIP (IPv4/IPv6, DHCP, sockets)
- BTstack (BLE + Classic)
- mbedTLS

pico-idf wraps those as `esp_wifi`, `esp_netif`, `esp_event`,
`esp_http_client`, `mqtt`, and a Bluedroid-shaped BLE subset. Protocols
that are Espressif-MAC-specific (ESP-NOW, ESP-WIFI-MESH, ESP-LR,
SmartConfig) cannot be bit-identical; the matrix marks them
`impossible` or offers a documented substitute (softAP + HTTP
provisioning instead of ESP-Touch).

### 6. Storage, OTA, security

| ESP-IDF | Pico plan |
|---|---|
| Partition table + NVS | Flash key-value store in a reserved last-N KB region |
| SPIFFS / FAT / VFS | LittleFS + FAT + VFS prefix mount |
| OTA `ota_0` / `ota_1` | Dual UF2 slots + boot scratch; `picotool` / USB for factory |
| mbedTLS / ESP-TLS | Pico SDK mbedTLS + `esp_tls` wrapper |
| eFuse | RP2350 OTP is a cousin, not a clone; RP2040 has no eFuse |

### 7. Peripherals

Map 1:1 where the silicon exists (GPIO, UART, I2C, SPI, ADC, PWM, I2S
via PIO). Emulate ESP-only blocks with PIO where it is honest (RMT,
PCNT). Mark true missing silicon `impossible` (capacitive touch, TWAI
hardware, 802.15.4, Ethernet MAC unless an external PHY is added).

Pico's unique block — **PIO** — is a first-class pico-idf component.
ESP-IDF has no PIO. Vibe-coding prompts should prefer PIO when an
ESP-IDF peripheral is `impossible`.

### 8. ESP-Claw-shaped edge agent (opt-in)

`components/claw` is a Pico-sized mapping of ESP-Claw ideas (capabilities,
events, memory, local agent, device MCP). It is **not** linked into
blink; apps `REQUIRES claw`. Lua, IM, and cloud LLM remain planned.
Details: [CLAW.md](CLAW.md).

## What "all ESP-IDF features" actually means

"All" is a product goal, not a claim that every ESP-IDF symbol links
today. The inventory in `tools/features.json` is the contract:

- **done** — API works on Pico W / Pico 2 W.
- **partial** — common subset works; extras are listed.
- **planned** — backend exists in Pico SDK or is straightforward; not
  wrapped yet. This is the vibe-coding backlog.
- **impossible** — no radio, no silicon, or Espressif-proprietary. The
  row says what to use instead.

Shipping the matrix, the CLI, the MCP server, FreeRTOS `app_main`, and
one real component (`driver/gpio`) is the foundation. Every later
feature is one MCP `vibe_next` turn.

## Non-goals

- Running ESP-IDF binaries or Espressif Wi-Fi firmware on a Pico.
- 100% source compatibility with every ESP-IDF example (ESP-NOW
  examples will never port).
- Replacing MicroPython for the user's existing Pico scripts. pico-idf
  is the C/RTOS path; MicroPython remains a separate stack.
- Hiding Pico-only hardware. PIO, USB, and RP2350 TrustZone/OTP stay
  documented Pico extensions.
