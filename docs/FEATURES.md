# Feature matrix

This tree tracks **ESP-Claw features only**. There is no ESP-IDF
feature backlog (no `nvs`, `wifi`, `mqtt`, GPIO-as-IDF, …).

Machine-readable source of truth: [`tools/features.json`](../tools/features.json).

Print the live table:

```bash
./tools/pidf.py vibe status
```

Cursor should use the MCP server (`pidf.py mcp` / `tools/pidf_mcp.py`)
and call `list_features` / `vibe_next` instead of parsing this file.

Status values:

| Status | Meaning |
|---|---|
| `done` | Public `claw_*` API works; `examples/claw/edge_agent` builds |
| `partial` | Common subset works |
| `planned` | Pico-sized analogue is straightforward; not wrapped yet |
| `impossible` | No silicon or RAM for a full ESP-Claw module. Use the note's substitute |

## ESP-Claw rows

- **done:** `claw-runtime`, `claw-sched` — caps, events, RAM memory, local agent, USB REPL
- **partial:** `claw-mcp-device` — JSON-RPC lines over USB-CDC
- **planned:** `claw-lua`, `claw-im`, `claw-llm`, `claw-skill`

Suggested vibe-coding order:

1. `claw-lua` — honest Pico-sized Lua or keep event-rule substitute
2. `claw-mcp-device` — finish the USB JSON-RPC tool surface
3. `claw-im` — IM over Pico SDK Wi-Fi + TLS
4. `claw-llm` — HTTP LLM after a TLS client exists
5. `claw-skill` — skills / board manager / settings UI on LittleFS

Pico SDK, FreeRTOS SMP, and thin `esp_err` / `ESP_LOGI` / GPIO helpers
are **runtime infrastructure** for `components/claw`. They are not
features to port from ESP-IDF.

See [docs/CLAW.md](CLAW.md).
