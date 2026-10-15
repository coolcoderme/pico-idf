#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t claw_memory_append(const char *role, const char *text);
int claw_memory_count(void);
esp_err_t claw_memory_get(int index, char *role, size_t role_n, char *text, size_t text_n);
esp_err_t claw_memory_clear(void);
esp_err_t claw_memory_dump(char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif
