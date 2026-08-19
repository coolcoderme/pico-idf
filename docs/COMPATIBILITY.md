# Compatibility promises

pico-idf now aims for an **ESP-Claw-shaped** edge agent on Pico W /
Pico 2 W, not an ESP-IDF API port.

## Compatible on purpose (ESP-Claw ideas)

- Capability registry / invoke (`claw_cap_register` / `claw_cap_call`)
- Event bus + router rules (`claw_event_post` / `claw_event_add_rule`)
- RAM structured memory (`claw_memory_*`)
- Local keyword agent (`claw_core_submit`)
- Periodic scheduler (`claw_sched_every_ms`)
- USB `claw>` REPL and JSON-RPC device MCP
- `void app_main(void)` on FreeRTOS (boot contract for the example)

## Deliberately different from full ESP-Claw

| ESP-Claw (Espressif boards) | pico-idf |
|---|---|
| ESP-IDF + 8 MB flash / 8 MB PSRAM class boards | Pico SDK + FreeRTOS on 264–520 KB SRAM |
| `cap_lua` + Lua drivers | Planned; Pico W cannot host the full VM + driver set |
| Telegram / Feishu / QQ IM | Planned over CYW43439 + TLS; USB REPL is the stand-in |
| Cloud LLM HTTP agent | Planned; local keyword agent works offline |
| FATFS SYSTEM/DATA + board-manager web UI | LittleFS later (`claw-skill`) |
| On-device MCP SDK | USB-CDC JSON-RPC subset (`initialize`, `tools/list`, `tools/call`, `ping`) |

## Not in this tree

ESP-IDF feature rows (`esp_wifi`, `nvs`, `mqtt`, `driver/uart`, ESP-NOW,
ESP-MESH, SmartConfig, Thread, touch, TWAI, eFuse, …) were removed from
`tools/features.json`. Do not re-add them. If claw later needs Wi-Fi or
flash, implement a Pico SDK backend inside `components/claw` (or a
private helper), not an ESP-IDF-shaped public API.

ESP-Claw examples that assume Espressif partitions, Board Manager, or
PSRAM will not compile unchanged. Implement Pico-sized analogues one
`claw-*` row at a time.
