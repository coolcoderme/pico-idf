#include "claw/cap.h"
#include "claw/claw.h"
#include "claw_priv.h"

#include <stdio.h>
#include <string.h>

static claw_cap_desc_t s_caps[CLAW_MAX_CAPS];
static int s_n;

esp_err_t claw_cap_register(const claw_cap_desc_t *desc)
{
    int i;
    if (desc == NULL || desc->id == NULL || desc->invoke == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    claw_lock();
    for (i = 0; i < s_n; i++) {
        if (strcmp(s_caps[i].id, desc->id) == 0) {
            s_caps[i] = *desc;
            claw_unlock();
            return ESP_OK;
        }
    }
    if (s_n >= CLAW_MAX_CAPS) {
        claw_unlock();
        return ESP_ERR_NO_MEM;
    }
    s_caps[s_n++] = *desc;
    claw_unlock();
    return ESP_OK;
}

esp_err_t claw_cap_call(const char *id, const char *args_json, char *out, size_t out_sz)
{
    claw_cap_fn_t fn = NULL;
    int i;
    if (id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (args_json == NULL) {
        args_json = "{}";
    }
    claw_lock();
    for (i = 0; i < s_n; i++) {
        if (strcmp(s_caps[i].id, id) == 0) {
            fn = s_caps[i].invoke;
            break;
        }
    }
    claw_unlock();
    if (fn == NULL) {
        if (out != NULL && out_sz > 0) {
            snprintf(out, out_sz, "{\"ok\":false,\"error\":\"not_found\",\"id\":\"%s\"}", id);
        }
        return ESP_ERR_NOT_FOUND;
    }
    return fn(args_json, out, out_sz);
}

int claw_cap_count(void)
{
    return s_n;
}

esp_err_t claw_cap_info(int index, char *id, size_t id_n, char *summary, size_t summary_n)
{
    if (index < 0 || index >= s_n) {
        return ESP_ERR_INVALID_ARG;
    }
    if (id != NULL && id_n > 0) {
        snprintf(id, id_n, "%s", s_caps[index].id);
    }
    if (summary != NULL && summary_n > 0) {
        snprintf(summary, summary_n, "%s", s_caps[index].summary ? s_caps[index].summary : "");
    }
    return ESP_OK;
}

esp_err_t claw_cap_list_json(char *out, size_t out_sz)
{
    size_t used;
    int i;
    if (out == NULL || out_sz < 3) {
        return ESP_ERR_INVALID_ARG;
    }
    used = (size_t)snprintf(out, out_sz, "{\"caps\":[");
    for (i = 0; i < s_n && used + 8 < out_sz; i++) {
        int n = snprintf(out + used, out_sz - used, "%s{\"id\":\"%s\",\"summary\":\"%s\"}",
                         i ? "," : "", s_caps[i].id,
                         s_caps[i].summary ? s_caps[i].summary : "");
        if (n < 0) {
            return ESP_ERR_INVALID_SIZE;
        }
        used += (size_t)n;
    }
    if (used + 2 >= out_sz) {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(out + used, "]}", 3);
    return ESP_OK;
}
