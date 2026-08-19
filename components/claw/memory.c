#include "claw/memory.h"
#include "claw/claw.h"
#include "claw_priv.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    char role[12];
    char text[CLAW_NOTE_CHARS];
} claw_note_t;

static claw_note_t s_notes[CLAW_MAX_NOTES];
static int s_n;
static int s_head;

esp_err_t claw_memory_append(const char *role, const char *text)
{
    int idx;
    if (role == NULL || text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    claw_lock();
    idx = (s_head + s_n) % CLAW_MAX_NOTES;
    if (s_n < CLAW_MAX_NOTES) {
        s_n++;
    } else {
        s_head = (s_head + 1) % CLAW_MAX_NOTES;
        idx = (s_head + s_n - 1) % CLAW_MAX_NOTES;
    }
    snprintf(s_notes[idx].role, sizeof(s_notes[idx].role), "%s", role);
    snprintf(s_notes[idx].text, sizeof(s_notes[idx].text), "%s", text);
    claw_unlock();
    return ESP_OK;
}

int claw_memory_count(void)
{
    return s_n;
}

esp_err_t claw_memory_get(int index, char *role, size_t role_n, char *text, size_t text_n)
{
    int idx;
    if (index < 0 || index >= s_n) {
        return ESP_ERR_INVALID_ARG;
    }
    idx = (s_head + index) % CLAW_MAX_NOTES;
    if (role != NULL && role_n > 0) {
        snprintf(role, role_n, "%s", s_notes[idx].role);
    }
    if (text != NULL && text_n > 0) {
        snprintf(text, text_n, "%s", s_notes[idx].text);
    }
    return ESP_OK;
}

esp_err_t claw_memory_clear(void)
{
    claw_lock();
    s_n = 0;
    s_head = 0;
    memset(s_notes, 0, sizeof(s_notes));
    claw_unlock();
    return ESP_OK;
}

esp_err_t claw_memory_dump(char *out, size_t out_sz)
{
    size_t used;
    int i;
    if (out == NULL || out_sz < 3) {
        return ESP_ERR_INVALID_ARG;
    }
    used = (size_t)snprintf(out, out_sz, "{\"notes\":[");
    for (i = 0; i < s_n && used + 8 < out_sz; i++) {
        int idx = (s_head + i) % CLAW_MAX_NOTES;
        int n = snprintf(out + used, out_sz - used,
                         "%s{\"role\":\"%s\",\"text\":\"%s\"}",
                         i ? "," : "", s_notes[idx].role, s_notes[idx].text);
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
