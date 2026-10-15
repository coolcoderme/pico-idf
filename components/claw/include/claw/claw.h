#pragma once

#include "claw/cap.h"
#include "claw/event.h"
#include "claw/memory.h"
#include "claw/core.h"
#include "claw/sched.h"
#include "claw/im.h"
#include "claw/lua.h"
#include "claw/mcp.h"
#include "claw/repl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLAW_MAX_CAPS 16
#define CLAW_MAX_RULES 8
#define CLAW_MAX_SUBS 8
#define CLAW_MAX_NOTES 16
#define CLAW_NOTE_CHARS 96
#define CLAW_MAX_JOBS 8
#define CLAW_ID_LEN 24

esp_err_t claw_runtime_init(void);

#ifdef __cplusplus
}
#endif
