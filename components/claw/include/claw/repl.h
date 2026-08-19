#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t claw_repl_eval(const char *line, char *out, size_t out_sz);
esp_err_t claw_repl_start(void);

#ifdef __cplusplus
}
#endif
