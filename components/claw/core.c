#include "claw/core.h"
#include "claw/cap.h"
#include "claw/event.h"
#include "claw/memory.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void lower_copy(char *dst, size_t n, const char *src)
{
    size_t i = 0;
    while (src[i] != '\0' && i + 1 < n) {
        dst[i] = (char)tolower((unsigned char)src[i]);
        i++;
    }
    dst[i] = '\0';
}

static int has_word(const char *hay, const char *word)
{
    size_t wlen = strlen(word);
    const char *p = hay;
    while ((p = strstr(p, word)) != NULL) {
        char before = (p == hay) ? ' ' : p[-1];
        char after = p[wlen];
        int b_ok = !isalnum((unsigned char)before);
        int a_ok = after == '\0' || !isalnum((unsigned char)after);
        if (b_ok && a_ok) {
            return 1;
        }
        p++;
    }
    return 0;
}

static const char *after_first_space(const char *s)
{
    const char *sp = strchr(s, ' ');
    if (sp == NULL) {
        return "";
    }
    while (*sp == ' ') {
        sp++;
    }
    return sp;
}

esp_err_t claw_core_submit(const char *user_text, char *reply, size_t reply_n)
{
    char low[128];
    char tmp[160];
    const char *led_on = "{\"pin\":32,\"level\":1}";
    const char *led_off = "{\"pin\":32,\"level\":0}";

    if (user_text == NULL || reply == NULL || reply_n == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    (void)claw_memory_append("user", user_text);
    snprintf(tmp, sizeof(tmp), "{\"text\":\"%s\"}", user_text);
    (void)claw_event_post("user.text", tmp);

    lower_copy(low, sizeof(low), user_text);

    if ((has_word(low, "led") || has_word(low, "light")) &&
        (has_word(low, "on") || has_word(low, "high"))) {
        (void)claw_cap_call("gpio", led_on, tmp, sizeof(tmp));
        snprintf(reply, reply_n, "LED on (GPIO 32)");
        (void)claw_memory_append("assistant", reply);
        return ESP_OK;
    }
    if ((has_word(low, "led") || has_word(low, "light")) &&
        (has_word(low, "off") || has_word(low, "low"))) {
        (void)claw_cap_call("gpio", led_off, tmp, sizeof(tmp));
        snprintf(reply, reply_n, "LED off (GPIO 32)");
        (void)claw_memory_append("assistant", reply);
        return ESP_OK;
    }
    if (strncmp(low, "remember ", 9) == 0 || strncmp(low, "note ", 5) == 0) {
        const char *note = after_first_space(user_text);
        (void)claw_memory_append("note", note);
        snprintf(reply, reply_n, "remembered");
        (void)claw_memory_append("assistant", reply);
        return ESP_OK;
    }
    if (has_word(low, "status") || has_word(low, "help")) {
        snprintf(reply, reply_n,
                 "local agent: led on/off, remember <text>, status. LLM/IM/Lua planned.");
        (void)claw_memory_append("assistant", reply);
        return ESP_OK;
    }
    snprintf(reply, reply_n, "unknown; try: led on | led off | remember <note> | status");
    (void)claw_memory_append("assistant", reply);
    return ESP_OK;
}
