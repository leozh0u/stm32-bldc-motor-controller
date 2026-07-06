/**
 * encoder.c — TIM2 encoder mode 3 (x4 decode, counts both edges of both
 * channels). PA0 = TIM2_CH1, PA1 = TIM2_CH2, both AF1, internal pull-ups
 * (hall encoder outputs are often open-collector). Register-level per RM0368.
 */
#include "stm32f401xe.h"
#include "encoder.h"

void encoder_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* PA0, PA1: AF mode, AF1 = TIM2_CH1/CH2, pull-up */
    GPIOA->MODER &= ~((0x3u << (0 * 2)) | (0x3u << (1 * 2)));
    GPIOA->MODER |=  ((0x2u << (0 * 2)) | (0x2u << (1 * 2)));
    GPIOA->AFR[0] &= ~((0xFu << (0 * 4)) | (0xFu << (1 * 4)));
    GPIOA->AFR[0] |=  ((0x1u << (0 * 4)) | (0x1u << (1 * 4)));
    GPIOA->PUPDR &= ~((0x3u << (0 * 2)) | (0x3u << (1 * 2)));
    GPIOA->PUPDR |=  ((0x1u << (0 * 2)) | (0x1u << (1 * 2)));

    /* CH1/CH2 as inputs mapped to TI1/TI2 (CCxS=01), input filter 0b0011 */
    TIM2->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_CC2S |
                     TIM_CCMR1_IC1F | TIM_CCMR1_IC2F);
    TIM2->CCMR1 |= TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0;
    TIM2->CCMR1 |= (0x3u << 4) | (0x3u << 12);

    /* Rising-edge polarity, non-inverted (CCxP=0, CCxNP=0) — reset default,
     * cleared explicitly for self-documentation. Inputs are not enabled for
     * capture (CCxE stays 0); encoder mode only needs the TI mapping. */
    TIM2->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP |
                    TIM_CCER_CC2P | TIM_CCER_CC2NP);

    /* Encoder mode 3: count both edges of TI1 and TI2 (SMS=011) */
    TIM2->SMCR &= ~TIM_SMCR_SMS;
    TIM2->SMCR |= (0x3u << 0);

    TIM2->ARR = 0xFFFFFFFFu;      /* full 32-bit range */
    TIM2->CNT = 0;
    TIM2->CR1 |= TIM_CR1_CEN;
}

uint32_t encoder_count(void)
{
    return TIM2->CNT;
}

int16_t encoder_rpm(uint32_t period_ms)
{
    static uint32_t last_cnt;

    uint32_t now = TIM2->CNT;
    int32_t delta = (int32_t)(now - last_cnt);   /* wrap- and sign-safe */
    last_cnt = now;

    if (period_ms == 0) {
        return 0;
    }
    /* rpm = delta counts / CPR revs, per period_ms, scaled to per-minute.
     * Worst case delta at 10ms/500rpm/1320cpr is ~110; delta*60000 cannot
     * overflow int32 for any physically plausible speed. */
    int32_t rpm = (delta * 60000) / ((int32_t)ENCODER_CPR * (int32_t)period_ms);

    if (rpm > INT16_MAX) rpm = INT16_MAX;
    if (rpm < INT16_MIN) rpm = INT16_MIN;
    return (int16_t)rpm;
}
