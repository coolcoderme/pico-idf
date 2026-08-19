# Vibe-coding pico-idf (MCP server)

Vibe-coding here means a **Model Context Protocol server**, not a
chat-only workflow. Cursor (and any other MCP host) launches
`tools/pidf_mcp.py` over stdio and calls tools to drive the **ESP-Claw**
feature matrix onto Pico W / Pico 2 W.

## Start the server

```bash
export PIDF_PATH=/path/to/pico-idf
python3 $PIDF_PATH/tools/pidf_mcp.py
# or:
$PIDF_PATH/tools/pidf.py mcp
```

Cursor config is checked in as [`.cursor/mcp.json`](../.cursor/mcp.json):

```json
{
  "mcpServers": {
    "pico-idf": {
      "command": "python3",
      "args": ["tools/pidf_mcp.py"],
      "env": { "PIDF_PATH": "${workspaceFolder}" }
    }
  }
}
```

The server speaks newline-delimited JSON-RPC 2.0 on stdio. It has no
PyPI dependency. Logs go to stderr; stdout is protocol only.

## Tools

| Tool | What an agent does with it |
|---|---|
| `list_features` / `get_feature` | Read the ESP-Claw → Pico matrix |
| `vibe_next` | Get the next `planned` claw row and a ready prompt |
| `scaffold_feature` | Remind that work lives in `components/claw` + examples |
| `set_feature_status` | Mark the row `done` / `partial` after a green build |
| `set_target` / `get_target` | `pico_w`, `pico2_w`, `pico2_w_riscv` |
| `set_config` | `CONFIG_*` |
| `build` | Cross-compile the current project (default: edge_agent) |
| `create_project` / `create_component` | New `app_main` claw app or helper component |
| `get_rules` | AGENTS.md + compatibility constraints |
| `list_examples` | Example list |
| `list_claw_caps` | Builtin caps (done vs planned) |

`set_feature_status` refuses `impossible` → `done`. The matrix is
claw-only; do not invent ESP-IDF rows.

## Resources

- `pidf://features` — full `tools/features.json` (claw rows only)
- `pidf://features/{id}` — one row (`claw-lua`, `claw-im`, …)
- `pidf://target` — board + sdkconfig
- `pidf://docs/architecture`
- `pidf://docs/agents`
- `pidf://docs/compatibility`
- `pidf://docs/vibecoding`
- `pidf://docs/claw`
- `pidf://claw/caps`

## Prompts

- `implement_feature` — next (or named) planned claw slice
- `explain_claw` — Pico subset vs full ESP-Claw
- `explain_impossible` — why a named row cannot be a full ESP-Claw module

## Agent loop

1. `vibe_next` (or `implement_feature` prompt)
2. `get_rules` if the host has not loaded them
3. Implement inside `components/claw` (do not copy ESP-Claw or ESP-IDF `.c` files)
4. `build` for `pico_w`, then `set_target pico2_w` and `build` again
5. `set_feature_status` to `done` or `partial`

Keep `void app_main(void)` on FreeRTOS. Public names stay `claw_*`.

## Hardware reminders the MCP will also surface

- Pico W LED is not a CPU GPIO. Use `PIDF_GPIO_WL_LED` (32).
- Pico 2 W is the same radio, M33 (or Hazard3) and more SRAM.
- Use `vTaskDelay`, not a bare busy-wait, once the scheduler is running.
