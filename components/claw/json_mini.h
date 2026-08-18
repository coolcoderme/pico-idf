#pragma once

#include <stdbool.h>
#include <stddef.h>

bool claw_json_has_key(const char *json, const char *key);
bool claw_json_get_str(const char *json, const char *key, char *out, size_t n);
int claw_json_get_int(const char *json, const char *key, int def);
int claw_json_get_bool(const char *json, const char *key, int def);
int claw_json_copy_value(const char *json, const char *key, char *out, size_t n);
