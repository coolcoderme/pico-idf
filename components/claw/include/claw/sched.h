#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t claw_sched_every_ms(const char *id, uint32_t period_ms, const char *event_name);
esp_err_t claw_sched_cancel(const char *id);
int claw_sched_count(void);
esp_err_t claw_sched_list_json(char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif
