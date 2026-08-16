# pico-idf

Minimal C/C++ firmware project for the **Raspberry Pi Pico** family
(RP2040 / RP2350), built with the official
[Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk).

The sample app (`src/main.c`) blinks the on-board LED and prints a heartbeat
counter over the USB serial (CDC) console — the classic embedded "hello world".

## Prerequisites

- CMake ≥ 3.13
- ARM cross toolchain: `gcc-arm-none-eabi`, `libnewlib-arm-none-eabi`, `libstdc++-arm-none-eabi-newlib`
- Python 3, `build-essential`
- The Raspberry Pi Pico SDK

On Debian/Ubuntu these are installed automatically by
[`.cursor/install.sh`](.cursor/install.sh). To do it manually:

```bash
sudo apt-get install -y cmake build-essential python3 gcc-arm-none-eabi \
    libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
git clone --branch 2.3.0 https://github.com/raspberrypi/pico-sdk.git /opt/pico-sdk
git -C /opt/pico-sdk submodule update --init
```

## Build

```bash
# Uses PICO_SDK_PATH, or falls back to /opt/pico-sdk (see CMakeLists.txt)
cmake -S . -B build
cmake --build build
```

Build for a different board (e.g. Pico W or Pico 2) by passing `PICO_BOARD`:

```bash
cmake -S . -B build -DPICO_BOARD=pico_w   # or: pico2, pico2_w
```

Artifacts are written to `build/`:

- `blink.elf` — the linked ELF (debug / SWD)
- `blink.uf2` — drag-and-drop image for the Pico's USB bootloader (`BOOTSEL`)
- `blink.bin`, `blink.hex`, `blink.elf.map`

## Flash

Hold `BOOTSEL` while plugging in the Pico, then copy `build/blink.uf2` onto the
`RPI-RP2` mass-storage device that appears. The board reboots and starts
blinking; open the USB serial port (115200 baud) to see the heartbeat log.

## Cloud Agent environment

[`.cursor/environment.json`](.cursor/environment.json) points `install` at
`.cursor/install.sh`, which installs the toolchain, pins the Pico SDK, and
pre-configures the CMake build tree so `cmake --build build` works immediately.
