/**
 * systick.c — SysTick 1ms tick. Uses CMSIS SysTick_Config (core header,
 * permitted). Counter wraps at 2^32 ms (~49.7 days); all comparisons use
 * subtraction so wrap is harmless.
 */
#include "stm32f401xe.h"
#include "systick.h"

#define CORE_CLK_HZ 16000000u

static volatile uint32_t s_ticks;

void SysTick_Handler(void)
{
    s_ticks++;
}

void systick_init(void)
{
    SysTick_Config(CORE_CLK_HZ / 1000u);
}

uint32_t millis(void)
{
    return s_ticks;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = s_ticks;
    while ((uint32_t)(s_ticks - start) < ms) {
        __NOP();
    }
}
