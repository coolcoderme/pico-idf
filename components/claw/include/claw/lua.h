#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Full Lua VM is planned. Pico W SRAM cannot host ESP-Claw's Lua + drivers. */
esp_err_t claw_lua_eval(const char *source, char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif
