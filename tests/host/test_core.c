#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_event.h"

static int saw_event;

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    (void)data;
    if (id == WIFI_EVENT_STA_START) {
        saw_event = 1;
    }
}

int main(void)
{
    if (strcmp(esp_err_to_name(ESP_OK), "ESP_OK") != 0) {
        return 1;
    }
    if (strcmp(esp_err_to_name(ESP_ERR_INVALID_ARG), "ESP_ERR_INVALID_ARG") != 0) {
        return 2;
    }
    if (esp_event_loop_create_default() != ESP_OK) {
        return 3;
    }
    if (esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, on_event, NULL) != ESP_OK) {
        return 4;
    }
    if (esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_START, NULL, 0, 0) != ESP_OK) {
        return 5;
    }
    if (!saw_event) {
        return 6;
    }
    printf("ok\n");
    return 0;
}
