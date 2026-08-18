# ESP-Claw on pico-idf

[ESP-Claw](https://github.com/espressif/esp-claw) is Espressif's
chat-as-creation edge agent (C, ESP-IDF, Lua, IM, MCP, on-device
memory). pico-idf does **not** vendor that tree. This document maps
those product ideas onto Pico W / Pico 2 W with a clean-room
`components/claw` API.

ESP-Claw's documented boards want on the order of **8 MB flash + 8 MB
PSRAM**. Pico W is 264 KB SRAM and typically 2 MB flash; Pico 2 W is
about 520 KB SRAM and 4 MB flash. A full Lua VM + Telegram/Feishu +
FATFS + cloud LLM client will not fit. The matrix stays honest.

## What ships now

| ESP-Claw idea | pico-idf | Status |
|---|---|---|
| Capability registry / invoke | `claw_cap_register` / `claw_cap_call` | done |
| Event bus + router rules | `claw_event_post` / `claw_event_add_rule` | done |
| Structured memory | RAM ring of notes (`claw_memory_*`); NVS later | partial |
| Agent loop | Local keyword NLU → caps (`claw_core_submit`) | partial |
| Scheduler | FreeRTOS software timers posting events | done |
| MCP server | USB-CDC JSON-RPC lines (`claw_mcp_handle_line`) | partial |
| USB console | `claw>` REPL (`claw_repl_eval` / `claw_repl_start`) | done |
| Lua + drivers | `claw_lua_eval` returns `ESP_ERR_NOT_SUPPORTED` | planned |
| IM (Telegram, Feishu, …) | `claw_im_send` — needs Wi-Fi + TLS | planned |
| Cloud / local LLM HTTP | after `wifi` + `http-client` | planned |
| Skills / board manager / web UI | not started | planned |
| Camera / touch Lua drivers | no silicon / no camera bridge in-tree | planned / impossible |

Built-in caps: `gpio`, `system`, `time`, `memory`, `event`, `agent`,
`sched`, plus honest stubs `lua` and `im`.

GPIO 32 is the Pico W / Pico 2 W onboard LED (`PIDF_GPIO_WL_LED`), not
a CPU pin.

## Example

`examples/claw/edge_agent` is opt-in. Blink does **not** link `claw`.
`main` must `REQUIRES claw`.

```bash
export PIDF_PROJECT=$PIDF_PATH/examples/claw/edge_agent
./tools/pidf.py set-target pico_w
./tools/pidf.py build
```

USB-CDC prompt:

```
claw> help
claw> led on
claw> agent remember picnic
claw> cap list
claw> {"jsonrpc":"2.0","id":1,"method":"tools/list"}
```

Lines that start with `{` are device MCP JSON-RPC (`initialize`,
`tools/list`, `tools/call`, `ping`). That is a Pico-sized analogue of
ESP-Claw exposing capabilities as tools — not the host vibe-coding
server in `tools/pidf_mcp.py`.

## API sketch

```c
esp_err_t claw_runtime_init(void);
esp_err_t claw_cap_register(const claw_cap_desc_t *desc);
esp_err_t claw_cap_call(const char *id, const char *args_json, char *out, size_t n);
esp_err_t claw_event_post(const char *name, const char *payload_json);
esp_err_t claw_memory_append(const char *role, const char *text);
esp_err_t claw_core_submit(const char *user_text, char *reply, size_t n);
esp_err_t claw_repl_start(void);
```

Do not copy ESP-Claw `.c` files. Names are inspired by the public
docs, implementations are original and Pico-sized.

## Host MCP

Cursor's pico-idf server can `list_features` with `group=claw`, read
`pidf://docs/claw`, and use the `explain_claw` prompt. That is how
vibe-coding continues Lua/IM/LLM slices without pretending they work
today.
