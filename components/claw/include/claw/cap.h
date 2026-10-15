#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef esp_err_t (*claw_cap_fn_t)(const char *args_json, char *out, size_t out_sz);

typedef struct {
    const char *id;
    const char *summary;
    claw_cap_fn_t invoke;
} claw_cap_desc_t;

esp_err_t claw_cap_register(const claw_cap_desc_t *desc);
esp_err_t claw_cap_call(const char *id, const char *args_json, char *out, size_t out_sz);
int claw_cap_count(void);
esp_err_t claw_cap_info(int index, char *id, size_t id_n, char *summary, size_t summary_n);
esp_err_t claw_cap_list_json(char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif
