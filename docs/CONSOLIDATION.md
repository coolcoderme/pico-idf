# How this tree was chosen (PRs #1–#3)

`main` was an empty repository (LICENSE only). Three overlapping draft
PRs landed in sequence and then diverged. This branch is the single
ready-for-review successor. None of the drafts had review comments.

## Timeline and intent

| PR | Date | Direction |
|---|---|---|
| [#1](https://github.com/coolcoderme/pico-idf/pull/1) Set up Raspberry Pi Pico SDK dev environment | 2026-08-16 | Parallel from empty `main`. Root `blink` firmware + Cloud Agent toolchain/SDK/picotool bootstrap. |
| [#2](https://github.com/coolcoderme/pico-idf/pull/2) ESP-IDF-shaped plan (RTOS + vibe-coding) | 2026-08-16, claw add-on 2026-08-18 | Independent of #1. Pico SDK + FreeRTOS SMP + ESP-IDF-shaped APIs + MCP vibe-coding, later plus ESP-Claw. Default example: blink. |
| [#3](https://github.com/coolcoderme/pico-idf/pull/3) Keep ESP-Claw features only | 2026-08-19 | **Built on #2** (same first four commits). Drops the ESP-IDF feature matrix. Default example: `examples/claw/edge_agent`. |

#3 is the latest product intent. #1 is the original Cloud Agent
environment, already absorbed and extended by #2/#3's `install.sh`
(toolchain, Pico SDK 2.3.0, picotool, GNU host compiler, plus
FreeRTOS-Kernel). #2 remains useful as the RTOS + MCP foundation, but
its ESP-IDF backlog (`nvs`, `wifi`, `mqtt`, GPIO-as-IDF, ESP-NOW, …)
was explicitly rejected by #3.

## Kept

- **Cloud Agent environment** from #1, as evolved by #2/#3:
  `.cursor/environment.json`, `.cursor/install.sh` (ARM toolchain,
  pinned Pico SDK 2.3.0, system picotool, GNU `cc`/`c++`, FreeRTOS-Kernel).
  Install now configures `examples/claw/edge_agent`, not a root blink.
- **Bring-up blink** from #1, relocated to `examples/get-started/blink`
  (already in #2/#3). Not a product feature.
- **#2 foundation:** `pidf.py`, stdio MCP server, FreeRTOS SMP `app_main`,
  component CMake (`idf_component_register`), thin `esp_err` / log /
  GPIO helpers as **private runtime infrastructure**.
- **#3 product contract:** claw-only `tools/features.json`
  (`claw-runtime`, `claw-sched`, `claw-mcp-device`, `claw-lua`,
  `claw-im`, `claw-llm`, `claw-skill`). MCP prompt `port_esp_idf_app`
  is gone. Default project is the claw edge agent.

## Dropped (and why)

- **#1 root `CMakeLists.txt` + `src/main.c` + `pico_sdk_import.cmake`.**
  A bare Pico SDK blink at the repo root conflicts with the
  ESP-IDF-shaped project template. The same hello-world lives under
  `examples/get-started/blink`.
- **#2 ESP-IDF feature matrix** (50+ rows: `nvs`, `wifi`, `mqtt`,
  `esp-now`, `gpio` as IDF, …). #3 made the vibe-coding contract
  claw-only. Pico SDK / FreeRTOS stay as HAL and RTOS, not as an
  ESP-IDF port backlog.
- **#2 MCP prompt `port_esp_idf_app`.** Agents should extend
  `components/claw`, not port ESP-IDF apps.
- **#2 default blink bootstrap.** #3 switched Cloud Agent install to
  the claw example. Blink remains available as a secondary target.
- **Treating `esp_err` / `ESP_LOGI` / `driver/gpio` as product APIs.**
  They remain in-tree so claw can boot, log, and drive the CYW43439
  LED. They are not tracked features.

## Superseded drafts

This pull request fully supersedes #1, #2, and #3. Close all three
after merge.
