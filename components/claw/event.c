#include "claw/event.h"
#include "claw/cap.h"
#include "claw/claw.h"
#include "claw_priv.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    char name[32];
    claw_event_cb_t cb;
    void *ctx;
} claw_sub_t;

typedef struct {
    char event[32];
    char cap_id[CLAW_ID_LEN];
    char args[96];
} claw_rule_t;

static claw_sub_t s_subs[CLAW_MAX_SUBS];
static int s_nsub;
static claw_rule_t s_rules[CLAW_MAX_RULES];
static int s_nrule;
static int s_depth;

esp_err_t claw_event_subscribe(const char *name, claw_event_cb_t cb, void *ctx)
{
    if (cb == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    claw_lock();
    if (s_nsub >= CLAW_MAX_SUBS) {
        claw_unlock();
        return ESP_ERR_NO_MEM;
    }
    if (name == NULL) {
        s_subs[s_nsub].name[0] = '\0';
    } else {
        snprintf(s_subs[s_nsub].name, sizeof(s_subs[s_nsub].name), "%s", name);
    }
    s_subs[s_nsub].cb = cb;
    s_subs[s_nsub].ctx = ctx;
    s_nsub++;
    claw_unlock();
    return ESP_OK;
}

esp_err_t claw_event_add_rule(const char *event, const char *cap_id, const char *args_json)
{
    if (event == NULL || cap_id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    claw_lock();
    if (s_nrule >= CLAW_MAX_RULES) {
        claw_unlock();
        return ESP_ERR_NO_MEM;
    }
    snprintf(s_rules[s_nrule].event, sizeof(s_rules[s_nrule].event), "%s", event);
    snprintf(s_rules[s_nrule].cap_id, sizeof(s_rules[s_nrule].cap_id), "%s", cap_id);
    snprintf(s_rules[s_nrule].args, sizeof(s_rules[s_nrule].args), "%s",
             args_json ? args_json : "{}");
    s_nrule++;
    claw_unlock();
    return ESP_OK;
}

esp_err_t claw_event_post(const char *name, const char *payload_json)
{
    claw_sub_t subs[CLAW_MAX_SUBS];
    claw_rule_t rules[CLAW_MAX_RULES];
    int nsub;
    int nrule;
    int i;
    char cap_out[160];

    if (name == NULL || name[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (payload_json == NULL) {
        payload_json = "{}";
    }

    claw_lock();
    if (s_depth >= 3) {
        claw_unlock();
        return ESP_ERR_INVALID_STATE;
    }
    s_depth++;
    nsub = s_nsub;
    nrule = s_nrule;
    memcpy(subs, s_subs, sizeof(subs));
    memcpy(rules, s_rules, sizeof(rules));
    claw_unlock();

    for (i = 0; i < nsub; i++) {
        if (subs[i].name[0] == '\0' || strcmp(subs[i].name, name) == 0) {
            subs[i].cb(name, payload_json, subs[i].ctx);
        }
    }
    for (i = 0; i < nrule; i++) {
        if (strcmp(rules[i].event, name) == 0) {
            (void)claw_cap_call(rules[i].cap_id, rules[i].args, cap_out, sizeof(cap_out));
        }
    }

    claw_lock();
    s_depth--;
    claw_unlock();
    return ESP_OK;
}

esp_err_t claw_event_rules_json(char *out, size_t out_sz)
{
    size_t used;
    int i;
    if (out == NULL || out_sz < 3) {
        return ESP_ERR_INVALID_ARG;
    }
    used = (size_t)snprintf(out, out_sz, "{\"rules\":[");
    for (i = 0; i < s_nrule && used + 8 < out_sz; i++) {
        int n = snprintf(out + used, out_sz - used,
                         "%s{\"event\":\"%s\",\"cap\":\"%s\"}",
                         i ? "," : "", s_rules[i].event, s_rules[i].cap_id);
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
