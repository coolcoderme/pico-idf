#include "esp_log.h"

#include <stdarg.h>
#include <string.h>

static esp_log_level_t s_level = (esp_log_level_t)CONFIG_LOG_DEFAULT_LEVEL;

#if defined(PICO_BOARD) || defined(PICO_RP2040) || defined(PICO_RP2350)
#include "pico/time.h"
uint32_t esp_log_timestamp(void)
{
    return (uint32_t)(to_ms_since_boot(get_absolute_time()));
}
#else
#include <time.h>
uint32_t esp_log_timestamp(void)
{
    return 0;
}
#endif

void esp_log_level_set(const char *tag, esp_log_level_t level)
{
    (void)tag;
    s_level = level;
}

esp_log_level_t esp_log_level_get(const char *tag)
{
    (void)tag;
    return s_level;
}

static const char *level_char(esp_log_level_t level)
{
    switch (level) {
    case ESP_LOG_ERROR: return "E";
    case ESP_LOG_WARN: return "W";
    case ESP_LOG_INFO: return "I";
    case ESP_LOG_DEBUG: return "D";
    case ESP_LOG_VERBOSE: return "V";
    default: return "?";
    }
}

void esp_log_write(esp_log_level_t level, const char *tag, const char *fmt, ...)
{
    if (level > s_level || level == ESP_LOG_NONE) {
        return;
    }
    printf("%s (%u) %s: ", level_char(level), (unsigned)esp_log_timestamp(),
           tag ? tag : "?");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}
