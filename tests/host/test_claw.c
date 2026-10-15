#include <stdio.h>
#include <string.h>

#include "claw/claw.h"
#include "esp_err.h"

extern int claw_host_last_pin;
extern int claw_host_last_level;

static int g_events;
static char g_last_event[32];

static void on_event(const char *name, const char *payload, void *ctx)
{
    (void)payload;
    (void)ctx;
    g_events++;
    snprintf(g_last_event, sizeof(g_last_event), "%s", name);
}

static int expect(int cond, int code)
{
    if (!cond) {
        return code;
    }
    return 0;
}

int main(void)
{
    char out[768];
    char reply[160];
    char role[12];
    char text[96];
    int rc;

    rc = expect(claw_runtime_init() == ESP_OK, 1);
    if (rc) {
        return rc;
    }
    rc = expect(claw_cap_count() >= 7, 2);
    if (rc) {
        return rc;
    }

    claw_host_last_pin = -1;
    if (claw_cap_call("gpio", "{\"pin\":32,\"level\":1}", out, sizeof(out)) != ESP_OK) {
        return 3;
    }
    if (claw_host_last_pin != 32 || claw_host_last_level != 1) {
        return 4;
    }
    if (strstr(out, "\"ok\":true") == NULL) {
        return 5;
    }

    if (claw_cap_call("nope", "{}", out, sizeof(out)) != ESP_ERR_NOT_FOUND) {
        return 6;
    }

    if (claw_event_subscribe("ping", on_event, NULL) != ESP_OK) {
        return 7;
    }
    if (claw_event_post("ping", "{\"n\":1}") != ESP_OK || g_events != 1) {
        return 8;
    }
    if (strcmp(g_last_event, "ping") != 0) {
        return 9;
    }

    claw_host_last_level = -1;
    if (claw_event_add_rule("led.on", "gpio", "{\"pin\":32,\"level\":1}") != ESP_OK) {
        return 10;
    }
    if (claw_event_post("led.on", "{}") != ESP_OK) {
        return 11;
    }
    if (claw_host_last_pin != 32 || claw_host_last_level != 1) {
        return 12;
    }

    if (claw_memory_append("user", "hello") != ESP_OK || claw_memory_count() < 1) {
        return 13;
    }
    if (claw_memory_get(0, role, sizeof(role), text, sizeof(text)) != ESP_OK) {
        return 14;
    }
    if (strcmp(role, "user") != 0 || strcmp(text, "hello") != 0) {
        return 15;
    }

    claw_host_last_level = 0;
    if (claw_core_submit("led on", reply, sizeof(reply)) != ESP_OK) {
        return 16;
    }
    if (strstr(reply, "LED on") == NULL || claw_host_last_level != 1) {
        return 17;
    }
    if (claw_core_submit("remember picnic", reply, sizeof(reply)) != ESP_OK) {
        return 18;
    }
    if (strcmp(reply, "remembered") != 0) {
        return 19;
    }

    if (claw_sched_every_ms("hb", 1000, "heartbeat") != ESP_OK || claw_sched_count() != 1) {
        return 20;
    }
    if (claw_lua_eval("print(1)", out, sizeof(out)) != ESP_ERR_NOT_SUPPORTED) {
        return 21;
    }
    if (claw_im_send("telegram", "hi") != ESP_ERR_NOT_SUPPORTED) {
        return 22;
    }

    if (claw_mcp_handle_line(
            "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}",
            out, sizeof(out)) != ESP_OK) {
        return 23;
    }
    if (strstr(out, "\"name\":\"gpio\"") == NULL) {
        return 24;
    }
    claw_host_last_level = 0;
    if (claw_mcp_handle_line(
            "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/call\","
            "\"params\":{\"name\":\"gpio\",\"arguments\":{\"pin\":32,\"level\":0}}}",
            out, sizeof(out)) != ESP_OK) {
        return 25;
    }
    if (claw_host_last_level != 0 || strstr(out, "\"ok\":true") == NULL) {
        return 26;
    }

    if (claw_repl_eval("led on", out, sizeof(out)) != ESP_OK) {
        return 27;
    }
    if (claw_host_last_level != 1) {
        return 28;
    }
    if (claw_repl_eval("cap list", out, sizeof(out)) != ESP_OK ||
        strstr(out, "gpio") == NULL) {
        return 29;
    }
    if (claw_repl_eval("agent status", out, sizeof(out)) != ESP_OK ||
        strstr(out, "local agent") == NULL) {
        return 30;
    }

    if (claw_cap_call("system", "{}", out, sizeof(out)) != ESP_OK ||
        strstr(out, "pico-idf") == NULL) {
        return 31;
    }

    printf("ok\n");
    return 0;
}
