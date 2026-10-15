#pragma once

#include <stddef.h>
#include <stdint.h>

void claw_lock_init(void);
void claw_lock(void);
void claw_unlock(void);
uint32_t claw_uptime_ms(void);
size_t claw_free_heap(void);
