#include "esp_event.h"

#include <string.h>

#define PIDF_EVENT_MAX 16

const char WIFI_EVENT_NAME[] = "WIFI_EVENT";
const char IP_EVENT_NAME[] = "IP_EVENT";

typedef struct {
    esp_event_base_t base;
    int32_t event_id;
    esp_event_handler_t handler;
    void *arg;
    int used;
} handler_slot_t;

static handler_slot_t s_handlers[PIDF_EVENT_MAX];
static int s_loop_ready;

esp_err_t esp_event_loop_create_default(void)
{
    memset(s_handlers, 0, sizeof(s_handlers));
    s_loop_ready = 1;
    return ESP_OK;
}

esp_err_t esp_event_loop_delete_default(void)
{
    s_loop_ready = 0;
    memset(s_handlers, 0, sizeof(s_handlers));
    return ESP_OK;
}

esp_err_t esp_event_handler_register(esp_event_base_t base, int32_t event_id,
                                     esp_event_handler_t handler, void *arg)
{
    if (!s_loop_ready) {
        esp_event_loop_create_default();
    }
    if (handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    for (int i = 0; i < PIDF_EVENT_MAX; i++) {
        if (!s_handlers[i].used) {
            s_handlers[i].base = base;
            s_handlers[i].event_id = event_id;
            s_handlers[i].handler = handler;
            s_handlers[i].arg = arg;
            s_handlers[i].used = 1;
            return ESP_OK;
        }
    }
    return ESP_ERR_NO_MEM;
}

esp_err_t esp_event_handler_unregister(esp_event_base_t base, int32_t event_id,
                                       esp_event_handler_t handler)
{
    for (int i = 0; i < PIDF_EVENT_MAX; i++) {
        if (s_handlers[i].used && s_handlers[i].handler == handler
            && s_handlers[i].base == base && s_handlers[i].event_id == event_id) {
            s_handlers[i].used = 0;
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t esp_event_post(esp_event_base_t base, int32_t event_id,
                         const void *data, size_t data_size, uint32_t ticks)
{
    (void)data_size;
    (void)ticks;
    if (!s_loop_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    for (int i = 0; i < PIDF_EVENT_MAX; i++) {
        if (!s_handlers[i].used) {
            continue;
        }
        int base_ok = (s_handlers[i].base == ESP_EVENT_ANY_BASE)
                      || (s_handlers[i].base == base);
        int id_ok = (s_handlers[i].event_id == ESP_EVENT_ANY_ID)
                    || (s_handlers[i].event_id == event_id);
        if (base_ok && id_ok) {
            s_handlers[i].handler(s_handlers[i].arg, base, event_id, (void *)data);
        }
    }
    return ESP_OK;
}
