#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*claw_event_cb_t)(const char *name, const char *payload_json, void *ctx);

esp_err_t claw_event_subscribe(const char *name, claw_event_cb_t cb, void *ctx);
esp_err_t claw_event_post(const char *name, const char *payload_json);
esp_err_t claw_event_add_rule(const char *event, const char *cap_id, const char *args_json);
esp_err_t claw_event_rules_json(char *out, size_t out_sz);

#ifdef __cplusplus
}
#endif
