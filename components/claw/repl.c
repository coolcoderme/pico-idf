#include "claw/repl.h"
#include "claw/claw.h"

#include "esp_log.h"

#include <stdio.h>
#include <string.h>

#ifndef PIDF_HOST_TEST
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pico/stdio.h"
#ifdef LIB_PICO_STDIO_USB
#include "pico/stdio_usb.h"
#endif
#endif

static const char *skip_ws(const char *s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    return s;
}

static int starts(const char *s, const char *pfx)
{
    size_t n = strlen(pfx);
    return strncmp(s, pfx, n) == 0;
}

esp_err_t claw_repl_eval(const char *line, char *out, size_t out_sz)
{
    const char *s;
    if (line == NULL || out == NULL || out_sz == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    s = skip_ws(line);
    if (*s == '\0') {
        out[0] = '\0';
        return ESP_OK;
    }
    if (*s == '{') {
        return claw_mcp_handle_line(s, out, out_sz);
    }
    if (strcmp(s, "help") == 0) {
        snprintf(out, out_sz,
                 "cap list | cap call <id> <json> | event post <name> [json] | "
                 "event rule <name> <cap> [json] | memory list | agent <text> | "
                 "sched list | led on|off | gpio <pin> <0|1>");
        return ESP_OK;
    }
    if (strcmp(s, "cap list") == 0) {
        return claw_cap_list_json(out, out_sz);
    }
    if (starts(s, "cap call ")) {
        char id[CLAW_ID_LEN];
        const char *rest = skip_ws(s + 9);
        const char *sp = strchr(rest, ' ');
        size_t idlen;
        const char *json;
        if (sp == NULL) {
            snprintf(id, sizeof(id), "%s", rest);
            json = "{}";
        } else {
            idlen = (size_t)(sp - rest);
            if (idlen >= sizeof(id)) {
                idlen = sizeof(id) - 1;
            }
            memcpy(id, rest, idlen);
            id[idlen] = '\0';
            json = skip_ws(sp + 1);
        }
        return claw_cap_call(id, json, out, out_sz);
    }
    if (starts(s, "event post ")) {
        char name[32];
        const char *rest = skip_ws(s + 11);
        const char *sp = strchr(rest, ' ');
        const char *payload = "{}";
        if (sp == NULL) {
            snprintf(name, sizeof(name), "%s", rest);
        } else {
            size_t n = (size_t)(sp - rest);
            if (n >= sizeof(name)) {
                n = sizeof(name) - 1;
            }
            memcpy(name, rest, n);
            name[n] = '\0';
            payload = skip_ws(sp + 1);
        }
        claw_event_post(name, payload);
        snprintf(out, out_sz, "{\"ok\":true}");
        return ESP_OK;
    }
    if (starts(s, "event rule ")) {
        char event[32];
        char cap_id[CLAW_ID_LEN];
        const char *rest = skip_ws(s + 11);
        const char *sp = strchr(rest, ' ');
        const char *sp2;
        const char *json = "{}";
        size_t n;
        if (sp == NULL) {
            snprintf(out, out_sz, "{\"ok\":false,\"error\":\"usage\"}");
            return ESP_ERR_INVALID_ARG;
        }
        n = (size_t)(sp - rest);
        if (n >= sizeof(event)) {
            n = sizeof(event) - 1;
        }
        memcpy(event, rest, n);
        event[n] = '\0';
        rest = skip_ws(sp + 1);
        sp2 = strchr(rest, ' ');
        if (sp2 == NULL) {
            snprintf(cap_id, sizeof(cap_id), "%s", rest);
        } else {
            n = (size_t)(sp2 - rest);
            if (n >= sizeof(cap_id)) {
                n = sizeof(cap_id) - 1;
            }
            memcpy(cap_id, rest, n);
            cap_id[n] = '\0';
            json = skip_ws(sp2 + 1);
        }
        if (claw_event_add_rule(event, cap_id, json) != ESP_OK) {
            snprintf(out, out_sz, "{\"ok\":false}");
            return ESP_FAIL;
        }
        snprintf(out, out_sz, "{\"ok\":true}");
        return ESP_OK;
    }
    if (strcmp(s, "event rules") == 0) {
        return claw_event_rules_json(out, out_sz);
    }
    if (strcmp(s, "memory list") == 0) {
        return claw_memory_dump(out, out_sz);
    }
    if (starts(s, "agent ")) {
        char reply[160];
        claw_core_submit(skip_ws(s + 6), reply, sizeof(reply));
        snprintf(out, out_sz, "%s", reply);
        return ESP_OK;
    }
    if (strcmp(s, "sched list") == 0) {
        return claw_sched_list_json(out, out_sz);
    }
    if (strcmp(s, "led on") == 0) {
        return claw_cap_call("gpio", "{\"pin\":32,\"level\":1}", out, out_sz);
    }
    if (strcmp(s, "led off") == 0) {
        return claw_cap_call("gpio", "{\"pin\":32,\"level\":0}", out, out_sz);
    }
    if (starts(s, "gpio ")) {
        int pin = 0;
        int level = 0;
        char json[48];
        if (sscanf(s + 5, "%d %d", &pin, &level) != 2) {
            snprintf(out, out_sz, "{\"ok\":false,\"error\":\"usage\"}");
            return ESP_ERR_INVALID_ARG;
        }
        snprintf(json, sizeof(json), "{\"pin\":%d,\"level\":%d}", pin, level);
        return claw_cap_call("gpio", json, out, out_sz);
    }
    snprintf(out, out_sz, "unknown command; type help");
    return ESP_OK;
}

#ifndef PIDF_HOST_TEST
static const char *TAG = "claw.repl";

static void repl_task(void *arg)
{
    char line[160];
    size_t n = 0;
    (void)arg;
#ifdef LIB_PICO_STDIO_USB
    while (!stdio_usb_connected()) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
#endif
    printf("\nclaw> ");
    fflush(stdout);
    for (;;) {
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (c == '\r' || c == '\n') {
            char out[768];
            if (n == 0) {
                printf("claw> ");
                fflush(stdout);
                continue;
            }
            line[n] = '\0';
            n = 0;
            printf("\n");
            if (claw_repl_eval(line, out, sizeof(out)) == ESP_OK || out[0] != '\0') {
                printf("%s\n", out);
            }
            printf("claw> ");
            fflush(stdout);
        } else if (c == 8 || c == 127) {
            if (n > 0) {
                n--;
            }
        } else if (n + 1 < sizeof(line) && c >= 32 && c < 127) {
            line[n++] = (char)c;
        }
    }
}

esp_err_t claw_repl_start(void)
{
    BaseType_t ok = xTaskCreate(repl_task, "claw_repl", 2048, NULL, 2, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "repl task");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
#else
esp_err_t claw_repl_start(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
#endif
