#include "json_mini.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char *skip_ws(const char *s)
{
    if (s == NULL) {
        return NULL;
    }
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
        s++;
    }
    return s;
}

static const char *find_value(const char *json, const char *key)
{
    char pat[40];
    const char *p;
    size_t klen;

    if (json == NULL || key == NULL) {
        return NULL;
    }
    klen = strlen(key);
    if (klen + 3 >= sizeof(pat)) {
        return NULL;
    }
    pat[0] = '"';
    memcpy(pat + 1, key, klen);
    pat[1 + klen] = '"';
    pat[2 + klen] = '\0';

    p = json;
    while ((p = strstr(p, pat)) != NULL) {
        const char *after = skip_ws(p + strlen(pat));
        if (*after == ':') {
            return skip_ws(after + 1);
        }
        p++;
    }
    return NULL;
}

static size_t value_span(const char *v)
{
    if (v == NULL || *v == '\0') {
        return 0;
    }
    if (*v == '"') {
        size_t i = 1;
        while (v[i] != '\0') {
            if (v[i] == '\\' && v[i + 1] != '\0') {
                i += 2;
                continue;
            }
            if (v[i] == '"') {
                return i + 1;
            }
            i++;
        }
        return i;
    }
    if (*v == '{' || *v == '[') {
        char open = *v;
        char close = (open == '{') ? '}' : ']';
        int depth = 0;
        size_t i = 0;
        int in_str = 0;
        while (v[i] != '\0') {
            if (in_str) {
                if (v[i] == '\\' && v[i + 1] != '\0') {
                    i += 2;
                    continue;
                }
                if (v[i] == '"') {
                    in_str = 0;
                }
                i++;
                continue;
            }
            if (v[i] == '"') {
                in_str = 1;
            } else if (v[i] == open) {
                depth++;
            } else if (v[i] == close) {
                depth--;
                if (depth == 0) {
                    return i + 1;
                }
            }
            i++;
        }
        return i;
    }
    size_t i = 0;
    while (v[i] != '\0' && v[i] != ',' && v[i] != '}' && v[i] != ']' &&
           v[i] != ' ' && v[i] != '\n' && v[i] != '\r' && v[i] != '\t') {
        i++;
    }
    return i;
}

bool claw_json_has_key(const char *json, const char *key)
{
    return find_value(json, key) != NULL;
}

int claw_json_copy_value(const char *json, const char *key, char *out, size_t n)
{
    const char *v = find_value(json, key);
    size_t span;
    if (v == NULL || out == NULL || n == 0) {
        return -1;
    }
    span = value_span(v);
    if (span + 1 > n) {
        span = n - 1;
    }
    memcpy(out, v, span);
    out[span] = '\0';
    return (int)span;
}

bool claw_json_get_str(const char *json, const char *key, char *out, size_t n)
{
    const char *v = find_value(json, key);
    size_t o = 0;
    if (v == NULL || out == NULL || n == 0) {
        return false;
    }
    out[0] = '\0';
    if (*v != '"') {
        return false;
    }
    v++;
    while (*v != '\0' && *v != '"' && o + 1 < n) {
        if (*v == '\\' && v[1] != '\0') {
            v++;
            out[o++] = *v++;
            continue;
        }
        out[o++] = *v++;
    }
    out[o] = '\0';
    return true;
}

int claw_json_get_int(const char *json, const char *key, int def)
{
    const char *v = find_value(json, key);
    char tmp[32];
    if (v == NULL) {
        return def;
    }
    if (*v == '"') {
        if (!claw_json_get_str(json, key, tmp, sizeof(tmp))) {
            return def;
        }
        return atoi(tmp);
    }
    return atoi(v);
}

int claw_json_get_bool(const char *json, const char *key, int def)
{
    const char *v = find_value(json, key);
    if (v == NULL) {
        return def;
    }
    if (strncmp(v, "true", 4) == 0) {
        return 1;
    }
    if (strncmp(v, "false", 5) == 0) {
        return 0;
    }
    if (isdigit((unsigned char)*v) || *v == '-') {
        return atoi(v) != 0;
    }
    return def;
}
