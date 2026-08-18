#include "claw/im.h"
#include "claw/lua.h"

#include <stdio.h>
#include <string.h>

esp_err_t claw_im_send(const char *channel, const char *text)
{
    (void)channel;
    (void)text;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t claw_lua_eval(const char *source, char *out, size_t out_sz)
{
    (void)source;
    if (out != NULL && out_sz > 0) {
        snprintf(out, out_sz,
                 "{\"ok\":false,\"error\":\"planned\",\"need\":\"lua-vm\"}");
    }
    return ESP_ERR_NOT_SUPPORTED;
}
