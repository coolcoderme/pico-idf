#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef const char *esp_event_base_t;
typedef void *esp_event_loop_handle_t;
typedef void (*esp_event_handler_t)(void *arg, esp_event_base_t base,
                                    int32_t event_id, void *event_data);

#define ESP_EVENT_ANY_BASE  ((esp_event_base_t)0)
#define ESP_EVENT_ANY_ID    (-1)

extern const char WIFI_EVENT_NAME[];
extern const char IP_EVENT_NAME[];
#define WIFI_EVENT ((esp_event_base_t)WIFI_EVENT_NAME)
#define IP_EVENT   ((esp_event_base_t)IP_EVENT_NAME)

enum {
    WIFI_EVENT_WIFI_READY = 0,
    WIFI_EVENT_SCAN_DONE,
    WIFI_EVENT_STA_START,
    WIFI_EVENT_STA_STOP,
    WIFI_EVENT_STA_CONNECTED,
    WIFI_EVENT_STA_DISCONNECTED,
    WIFI_EVENT_AP_START,
    WIFI_EVENT_AP_STOP,
};

enum {
    IP_EVENT_STA_GOT_IP = 0,
    IP_EVENT_STA_LOST_IP,
};

esp_err_t esp_event_loop_create_default(void);
esp_err_t esp_event_loop_delete_default(void);
esp_err_t esp_event_handler_register(esp_event_base_t base, int32_t event_id,
                                     esp_event_handler_t handler, void *arg);
esp_err_t esp_event_handler_unregister(esp_event_base_t base, int32_t event_id,
                                       esp_event_handler_t handler);
esp_err_t esp_event_post(esp_event_base_t base, int32_t event_id,
                         const void *data, size_t data_size, uint32_t ticks);

#ifdef __cplusplus
}
#endif
