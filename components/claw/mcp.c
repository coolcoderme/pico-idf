#include "claw/mcp.h"
#include "claw/cap.h"
#include "claw/claw.h"
#include "json_mini.h"

#include <stdio.h>
#include <string.h>

static void rpc_error(char *out, size_t n, const char *id_raw, int code, const char *msg)
{
    if (id_raw == NULL || id_raw[0] == '\0') {
        id_raw = "null";
    }
    snprintf(out, n,
             "{\"jsonrpc\":\"2.0\",\"id\":%s,\"error\":{\"code\":%d,\"message\":\"%s\"}}",
             id_raw, code, msg);
}

static void rpc_result(char *out, size_t n, const char *id_raw, const char *result)
{
    if (id_raw == NULL || id_raw[0] == '\0') {
        id_raw = "null";
    }
    snprintf(out, n, "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":%s}", id_raw, result);
}

esp_err_t claw_mcp_handle_line(const char *json_line, char *out, size_t out_sz)
{
    char method[48];
    char id_raw[32];
    char name[32];
    char args[192];
    char result[768];
    char tmp[256];
    int i;
    int n;
    size_t used;

    if (json_line == NULL || out == NULL || out_sz == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    while (*json_line == ' ' || *json_line == '\t') {
        json_line++;
    }
    if (json_line[0] != '{') {
        return ESP_ERR_INVALID_ARG;
    }

    if (claw_json_copy_value(json_line, "id", id_raw, sizeof(id_raw)) < 0) {
        snprintf(id_raw, sizeof(id_raw), "null");
    }
    if (!claw_json_get_str(json_line, "method", method, sizeof(method))) {
        rpc_error(out, out_sz, id_raw, -32600, "missing method");
        return ESP_ERR_INVALID_ARG;
    }

    if (strcmp(method, "initialize") == 0 || strcmp(method, "notifications/initialized") == 0) {
        rpc_result(out, out_sz, id_raw,
                   "{\"protocolVersion\":\"2025-03-26\","
                   "\"capabilities\":{\"tools\":{}},"
                   "\"serverInfo\":{\"name\":\"pico-idf-claw\",\"version\":\"0.2.0\"}}");
        return ESP_OK;
    }
    if (strcmp(method, "ping") == 0) {
        rpc_result(out, out_sz, id_raw, "{}");
        return ESP_OK;
    }
    if (strcmp(method, "tools/list") == 0) {
        used = (size_t)snprintf(result, sizeof(result), "{\"tools\":[");
        n = claw_cap_count();
        for (i = 0; i < n && used + 16 < sizeof(result); i++) {
            char id[CLAW_ID_LEN];
            char summary[80];
            int w;
            if (claw_cap_info(i, id, sizeof(id), summary, sizeof(summary)) != ESP_OK) {
                continue;
            }
            w = snprintf(result + used, sizeof(result) - used,
                         "%s{\"name\":\"%s\",\"description\":\"%s\"}",
                         i ? "," : "", id, summary);
            if (w < 0) {
                break;
            }
            used += (size_t)w;
        }
        if (used + 3 < sizeof(result)) {
            memcpy(result + used, "]}", 3);
        }
        rpc_result(out, out_sz, id_raw, result);
        return ESP_OK;
    }
    if (strcmp(method, "tools/call") == 0) {
        tmp[0] = '\0';
        if (!claw_json_get_str(json_line, "name", name, sizeof(name))) {
            rpc_error(out, out_sz, id_raw, -32602, "missing name");
            return ESP_ERR_INVALID_ARG;
        }
        if (claw_json_copy_value(json_line, "arguments", args, sizeof(args)) < 0) {
            snprintf(args, sizeof(args), "{}");
        }
        (void)claw_cap_call(name, args, tmp, sizeof(tmp));
        if (tmp[0] == '\0') {
            snprintf(tmp, sizeof(tmp), "{\"ok\":false}");
        }
        rpc_result(out, out_sz, id_raw, tmp);
        return ESP_OK;
    }

    rpc_error(out, out_sz, id_raw, -32601, "unknown method");
    return ESP_ERR_NOT_FOUND;
}
