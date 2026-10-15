#include "claw/sched.h"
#include "claw/event.h"
#include "claw/claw.h"
#include "claw_priv.h"

#include <stdio.h>
#include <string.h>

#ifndef PIDF_HOST_TEST
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#endif

typedef struct {
    char id[CLAW_ID_LEN];
    uint32_t period_ms;
    char event[32];
    int used;
#ifndef PIDF_HOST_TEST
    TimerHandle_t timer;
#endif
} claw_job_t;

static claw_job_t s_jobs[CLAW_MAX_JOBS];

#ifndef PIDF_HOST_TEST
static void on_timer(TimerHandle_t timer)
{
    claw_job_t *job = (claw_job_t *)pvTimerGetTimerID(timer);
    char payload[48];
    if (job == NULL || !job->used) {
        return;
    }
    snprintf(payload, sizeof(payload), "{\"id\":\"%s\"}", job->id);
    (void)claw_event_post(job->event, payload);
}
#endif

static claw_job_t *find_job(const char *id)
{
    int i;
    for (i = 0; i < CLAW_MAX_JOBS; i++) {
        if (s_jobs[i].used && strcmp(s_jobs[i].id, id) == 0) {
            return &s_jobs[i];
        }
    }
    return NULL;
}

esp_err_t claw_sched_every_ms(const char *id, uint32_t period_ms, const char *event_name)
{
    claw_job_t *slot = NULL;
    int i;
    if (id == NULL || event_name == NULL || period_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    claw_lock();
    slot = find_job(id);
    if (slot == NULL) {
        for (i = 0; i < CLAW_MAX_JOBS; i++) {
            if (!s_jobs[i].used) {
                slot = &s_jobs[i];
                break;
            }
        }
    }
    if (slot == NULL) {
        claw_unlock();
        return ESP_ERR_NO_MEM;
    }
    snprintf(slot->id, sizeof(slot->id), "%s", id);
    snprintf(slot->event, sizeof(slot->event), "%s", event_name);
    slot->period_ms = period_ms;
    slot->used = 1;
#ifndef PIDF_HOST_TEST
    if (slot->timer == NULL) {
        slot->timer = xTimerCreate(slot->id, pdMS_TO_TICKS(period_ms), pdTRUE, slot, on_timer);
        if (slot->timer == NULL) {
            slot->used = 0;
            claw_unlock();
            return ESP_ERR_NO_MEM;
        }
    } else {
        xTimerChangePeriod(slot->timer, pdMS_TO_TICKS(period_ms), 0);
    }
    xTimerStart(slot->timer, 0);
#endif
    claw_unlock();
    return ESP_OK;
}

esp_err_t claw_sched_cancel(const char *id)
{
    claw_job_t *job;
    if (id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    claw_lock();
    job = find_job(id);
    if (job == NULL) {
        claw_unlock();
        return ESP_ERR_NOT_FOUND;
    }
#ifndef PIDF_HOST_TEST
    if (job->timer != NULL) {
        xTimerStop(job->timer, 0);
    }
#endif
    job->used = 0;
    claw_unlock();
    return ESP_OK;
}

int claw_sched_count(void)
{
    int i;
    int n = 0;
    for (i = 0; i < CLAW_MAX_JOBS; i++) {
        if (s_jobs[i].used) {
            n++;
        }
    }
    return n;
}

esp_err_t claw_sched_list_json(char *out, size_t out_sz)
{
    size_t used;
    int i;
    int first = 1;
    if (out == NULL || out_sz < 3) {
        return ESP_ERR_INVALID_ARG;
    }
    used = (size_t)snprintf(out, out_sz, "{\"jobs\":[");
    for (i = 0; i < CLAW_MAX_JOBS; i++) {
        int n;
        if (!s_jobs[i].used) {
            continue;
        }
        n = snprintf(out + used, out_sz - used,
                     "%s{\"id\":\"%s\",\"period_ms\":%u,\"event\":\"%s\"}",
                     first ? "" : ",", s_jobs[i].id, (unsigned)s_jobs[i].period_ms,
                     s_jobs[i].event);
        first = 0;
        if (n < 0) {
            return ESP_ERR_INVALID_SIZE;
        }
        used += (size_t)n;
        if (used + 2 >= out_sz) {
            return ESP_ERR_INVALID_SIZE;
        }
    }
    memcpy(out + used, "]}", 3);
    return ESP_OK;
}
