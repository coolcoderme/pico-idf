#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Telegram / Feishu / similar. Needs Wi-Fi + TLS — not on this RAM budget yet. */
esp_err_t claw_im_send(const char *channel, const char *text);

#ifdef __cplusplus
}
#endif
