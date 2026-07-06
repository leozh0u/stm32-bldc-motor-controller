/**
 * systick.h — 1ms system tick, wraparound-safe delays.
 */
#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_init(void);          /* 1ms tick from 16MHz HSI core clock */
uint32_t millis(void);            /* ms since systick_init(), wraps at 2^32 */
void delay_ms(uint32_t ms);       /* blocking, wraparound-safe */

#endif
