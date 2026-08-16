# Feature matrix

Machine-readable source of truth: [`tools/features.json`](../tools/features.json).

Print the live table:

```bash
./tools/pidf.py vibe status
```

Status values:

| Status | Meaning |
|---|---|
| `done` | Public API works; an example builds |
| `partial` | Common subset works |
| `planned` | Pico backend exists or is straightforward; not wrapped yet |
| `impossible` | No silicon, no radio, or Espressif-proprietary. Use the note's substitute |

## How "all ESP-IDF features" get onto a Pico W / Pico 2 W

1. **RTOS first.** FreeRTOS SMP is already the kernel. Networking on W
   boards will use the FreeRTOS CYW43 arch, not a poll loop.
2. **Wrap what the Pico SDK already ships** (lwIP, BTstack, mbedTLS,
   TinyUSB, GPIO/UART/I2C/SPI/ADC/PWM) with ESP-IDF-shaped headers.
3. **Emulate ESP-only peripherals with PIO** (RMT, PCNT, I2S).
4. **Refuse the impossible honestly** (ESP-NOW, ESP-MESH, Thread,
   touch, TWAI, eFuse on RP2040) and document the substitute.
5. **Vibe-code one `planned` row per change** so the matrix stays true.

Suggested implementation order after this foundation:

1. `nvs` — flash key-value (unlocks Wi-Fi credentials)
2. `wifi` + `esp_netif` + `esp_event` WIFI/IP posts
3. `http-client` / `http-server` / `sntp` / `mdns`
4. `mqtt` + `esp-tls`
5. `ble` (BTstack wrapper)
6. `uart` `i2c` `spi` `adc` `ledc`
7. `ota` + `littlefs` + `vfs`
8. `provisioning` (softAP, not SmartConfig)
9. PIO extras: `rmt`, `i2s`, `pcnt`
