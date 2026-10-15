#include "claw/cap.h"
#include "claw/claw.h"
#include "claw/event.h"
#include "claw/memory.h"
#include "claw/core.h"
#include "claw/sched.h"
#include "claw/im.h"
#include "claw/lua.h"
#include "claw_priv.h"
#include "json_mini.h"

#include "driver/gpio.h"

#include <stdio.h>
#include <string.h>

static void json_fail(char *out, size_t n, const char *err)
{
    if (out != NULL && n > 0) {
        snprintf(out, n, "{\"ok\":false,\"error\":\"%s\"}", err);
    }
}

static esp_err_t cap_gpio(const char *args, char *out, size_t n)
{
    int pin = claw_json_get_int(args, "pin", PIDF_GPIO_WL_LED);
    char mode[16];
    int has_mode = claw_json_get_str(args, "mode", mode, sizeof(mode));
    int has_level = claw_json_has_key(args, "level");
    int level = claw_json_get_int(args, "level", 0);
    esp_err_t err = ESP_OK;
    int got;

    if (has_mode) {
        gpio_mode_t m = GPIO_MODE_OUTPUT;
        if (strcmp(mode, "input") == 0) {
            m = GPIO_MODE_INPUT;
        }
        err = gpio_reset_pin((gpio_num_t)pin);
        if (err == ESP_OK) {
            err = gpio_set_direction((gpio_num_t)pin, m);
        }
    }
    if (err == ESP_OK && has_level) {
        err = gpio_reset_pin((gpio_num_t)pin);
        if (err == ESP_OK) {
            err = gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
        }
        if (err == ESP_OK) {
            err = gpio_set_level((gpio_num_t)pin, (uint32_t)level);
        }
    }
    if (err != ESP_OK) {
        json_fail(out, n, "gpio");
        return err;
    }
    got = gpio_get_level((gpio_num_t)pin);
    if (out != NULL && n > 0) {
        snprintf(out, n, "{\"ok\":true,\"pin\":%d,\"level\":%d}", pin, got);
    }
    return ESP_OK;
}

static esp_err_t cap_system(const char *args, char *out, size_t n)
{
    (void)args;
    if (out != NULL && n > 0) {
        snprintf(out, n,
                 "{\"ok\":true,\"chip\":\"pico-idf\",\"uptime_ms\":%u,\"heap\":%u,\"caps\":%d}",
                 (unsigned)claw_uptime_ms(), (unsigned)claw_free_heap(), claw_cap_count());
    }
    return ESP_OK;
}

static esp_err_t cap_time(const char *args, char *out, size_t n)
{
    (void)args;
    if (out != NULL && n > 0) {
        snprintf(out, n, "{\"ok\":true,\"uptime_ms\":%u}", (unsigned)claw_uptime_ms());
    }
    return ESP_OK;
}

static esp_err_t cap_memory(const char *args, char *out, size_t n)
{
    char op[16];
    if (!claw_json_get_str(args, "op", op, sizeof(op))) {
        snprintf(op, sizeof(op), "list");
    }
    if (strcmp(op, "clear") == 0) {
        claw_memory_clear();
        if (out != NULL && n > 0) {
            snprintf(out, n, "{\"ok\":true}");
        }
        return ESP_OK;
    }
    if (strcmp(op, "append") == 0) {
        char role[12];
        char text[CLAW_NOTE_CHARS];
        if (!claw_json_get_str(args, "role", role, sizeof(role))) {
            snprintf(role, sizeof(role), "note");
        }
        if (!claw_json_get_str(args, "text", text, sizeof(text))) {
            json_fail(out, n, "missing_text");
            return ESP_ERR_INVALID_ARG;
        }
        claw_memory_append(role, text);
        if (out != NULL && n > 0) {
            snprintf(out, n, "{\"ok\":true,\"count\":%d}", claw_memory_count());
        }
        return ESP_OK;
    }
    return claw_memory_dump(out, n);
}

static esp_err_t cap_event(const char *args, char *out, size_t n)
{
    char op[16];
    char name[32];
    if (!claw_json_get_str(args, "op", op, sizeof(op))) {
        snprintf(op, sizeof(op), "rules");
    }
    if (strcmp(op, "post") == 0) {
        char payload[96];
        if (!claw_json_get_str(args, "name", name, sizeof(name))) {
            json_fail(out, n, "missing_name");
            return ESP_ERR_INVALID_ARG;
        }
        if (claw_json_copy_value(args, "payload", payload, sizeof(payload)) < 0) {
            snprintf(payload, sizeof(payload), "{}");
        }
        claw_event_post(name, payload);
        if (out != NULL && n > 0) {
            snprintf(out, n, "{\"ok\":true}");
        }
        return ESP_OK;
    }
    if (strcmp(op, "rule") == 0) {
        char cap_id[CLAW_ID_LEN];
        char cap_args[96];
        if (!claw_json_get_str(args, "name", name, sizeof(name)) ||
            !claw_json_get_str(args, "cap", cap_id, sizeof(cap_id))) {
            json_fail(out, n, "missing_rule");
            return ESP_ERR_INVALID_ARG;
        }
        if (claw_json_copy_value(args, "args", cap_args, sizeof(cap_args)) < 0) {
            snprintf(cap_args, sizeof(cap_args), "{}");
        }
        if (claw_event_add_rule(name, cap_id, cap_args) != ESP_OK) {
            json_fail(out, n, "rule");
            return ESP_FAIL;
        }
        if (out != NULL && n > 0) {
            snprintf(out, n, "{\"ok\":true}");
        }
        return ESP_OK;
    }
    return claw_event_rules_json(out, n);
}

static esp_err_t cap_agent(const char *args, char *out, size_t n)
{
    char text[120];
    char reply[160];
    if (!claw_json_get_str(args, "text", text, sizeof(text))) {
        json_fail(out, n, "missing_text");
        return ESP_ERR_INVALID_ARG;
    }
    claw_core_submit(text, reply, sizeof(reply));
    if (out != NULL && n > 0) {
        snprintf(out, n, "{\"ok\":true,\"reply\":\"%s\"}", reply);
    }
    return ESP_OK;
}

static esp_err_t cap_sched(const char *args, char *out, size_t n)
{
    char op[16];
    if (!claw_json_get_str(args, "op", op, sizeof(op))) {
        snprintf(op, sizeof(op), "list");
    }
    if (strcmp(op, "every") == 0) {
        char id[CLAW_ID_LEN];
        char event[32];
        int period = claw_json_get_int(args, "period_ms", 0);
        if (!claw_json_get_str(args, "id", id, sizeof(id)) ||
            !claw_json_get_str(args, "event", event, sizeof(event))) {
            json_fail(out, n, "missing_job");
            return ESP_ERR_INVALID_ARG;
        }
        if (claw_sched_every_ms(id, (uint32_t)period, event) != ESP_OK) {
            json_fail(out, n, "sched");
            return ESP_FAIL;
        }
        if (out != NULL && n > 0) {
            snprintf(out, n, "{\"ok\":true}");
        }
        return ESP_OK;
    }
    if (strcmp(op, "cancel") == 0) {
        char id[CLAW_ID_LEN];
        if (!claw_json_get_str(args, "id", id, sizeof(id))) {
            json_fail(out, n, "missing_id");
            return ESP_ERR_INVALID_ARG;
        }
        return claw_sched_cancel(id);
    }
    return claw_sched_list_json(out, n);
}

static esp_err_t cap_lua(const char *args, char *out, size_t n)
{
    char src[8];
    (void)claw_json_get_str(args, "source", src, sizeof(src));
    return claw_lua_eval(src, out, n);
}

static esp_err_t cap_im(const char *args, char *out, size_t n)
{
    char channel[24];
    char text[64];
    if (!claw_json_get_str(args, "channel", channel, sizeof(channel))) {
        snprintf(channel, sizeof(channel), "local");
    }
    if (!claw_json_get_str(args, "text", text, sizeof(text))) {
        json_fail(out, n, "missing_text");
        return ESP_ERR_INVALID_ARG;
    }
    if (claw_im_send(channel, text) != ESP_OK) {
        json_fail(out, n, "planned_wifi_tls");
        return ESP_ERR_NOT_SUPPORTED;
    }
    return ESP_OK;
}

void claw_builtins_register(void)
{
    static const claw_cap_desc_t caps[] = {
        {"gpio", "Set or read a GPIO (pin 32 is the W-board LED)", cap_gpio},
        {"system", "Uptime, heap, capability count", cap_system},
        {"time", "Uptime in milliseconds", cap_time},
        {"memory", "RAM session notes (NVS persistence planned)", cap_memory},
        {"event", "Post events or add router rules", cap_event},
        {"agent", "Local keyword agent (cloud LLM planned)", cap_agent},
        {"sched", "Periodic FreeRTOS-timer jobs that post events", cap_sched},
        {"lua", "Lua VM (planned — RAM)", cap_lua},
        {"im", "Instant messaging (planned — Wi-Fi + TLS)", cap_im},
    };
    size_t i;
    for (i = 0; i < sizeof(caps) / sizeof(caps[0]); i++) {
        (void)claw_cap_register(&caps[i]);
    }
}
