#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One JSON-RPC line (tools/list, tools/call) mapped onto claw capabilities. */
esp_err_t claw_mcp_handle_line(const char *json_line, char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif
