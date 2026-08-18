# Compatibility promises

pico-idf aims for **source-level familiarity**, not ABI or silicon
compatibility with ESP32.

## Compatible on purpose

- `esp_err_t` / `ESP_OK` / `ESP_ERR_*` and `ESP_ERROR_CHECK`
- `ESP_LOGI` / `W` / `E` / `D` / `V`
- `void app_main(void)` started as a FreeRTOS task
- `idf_component_register(...)` and `sdkconfig` `CONFIG_*` names
- `pidf.py` subcommands that match `idf.py`
- GPIO, UART, I2C, SPI, ADC, PWM call shapes
- `esp_wifi_init` / `set_mode` / `set_config` / `start` / `connect`
  (planned, over CYW43439)
- `nvs_*` key-value API (planned)
- `esp_event_loop_create_default` / `esp_event_handler_register`
- `esp_http_client`, `httpd`, MQTT, mDNS, SNTP (planned, over lwIP)
- ESP-Claw-shaped `claw_*` subset: caps, events, RAM memory, local agent,
  USB REPL / device MCP (`components/claw`; see [CLAW.md](CLAW.md))

## Deliberately different

| ESP-IDF | pico-idf |
|---|---|
| `idf.py set-target esp32s3` | `pidf.py set-target pico_w` / `pico2_w` |
| Partition CSV + esptool | UF2 + picotool; reserved flash tail for NVS/OTA |
| Wi-Fi MAC in ROM/blob | CYW43439 firmware via Pico SDK |
| Bluedroid / NimBLE | BTstack (Bluedroid-shaped wrapper later) |
| RMT / PCNT silicon | PIO programs |
| ULP coprocessor | PIO + second core |
| eFuse | RP2350 OTP only; none on RP2040 |
| ESP-NOW / ESP-MESH / ESP-LR | Not implementable; use UDP/MQTT/softAP |
| Thread / Zigbee | No 802.15.4 radio |
| `esp_wifi_set_protocol(WIFI_PROTOCOL_LR)` | 802.11n only |
| ESP-Claw Lua + Telegram + 8 MB PSRAM boards | Pico subset: USB REPL, keyword agent, RAM notes |

## Will not compile unchanged

ESP-IDF examples that include `soc/rtc.h`, `esp_mac.h` chip-id helpers,
Xtensa/ESP-RISC-V intrinsics, or `sdkconfig` options such as
`CONFIG_ESP32_WIFI_IRAM_OPT` need a Pico pass. The vibe-coding loop is
the intended way to port them one example at a time.
