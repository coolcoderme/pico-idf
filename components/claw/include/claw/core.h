#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Local agent: keyword NLU + capability dispatch. Cloud LLM is planned. */
esp_err_t claw_core_submit(const char *user_text, char *reply, size_t reply_n);

#ifdef __cplusplus
}
#endif
