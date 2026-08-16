/*
 * pico-idf: minimal Raspberry Pi Pico (RP2040/RP2350) firmware.
 *
 * Blinks the on-board LED and prints a heartbeat counter over the USB
 * serial (CDC) console. This is the "hello world" used to prove the
 * Cloud Agent development environment can cross-compile firmware end to end.
 */
#include <stdio.h>
#include "pico/stdlib.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

int main(void) {
    stdio_init_all();

#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif

    uint32_t heartbeat = 0;
    while (true) {
#ifdef PICO_DEFAULT_LED_PIN
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
#endif
        printf("pico-idf: heartbeat #%lu (LED on)\n", (unsigned long)heartbeat);
        sleep_ms(LED_DELAY_MS);

#ifdef PICO_DEFAULT_LED_PIN
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
#endif
        sleep_ms(LED_DELAY_MS);

        heartbeat++;
    }
}
