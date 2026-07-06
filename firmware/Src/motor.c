/**
 * motor.c — TIM1 CH1 PWM @ 20kHz on PA8 (AF1), direction PA9/PA10.
 * Register-level per RM0368. Remember: TIM1 is an advanced-control timer,
 * BDTR.MOE must be set or the pin stays dead.
 */
#include "stm32f401xe.h"
#include "motor.h"

#define PWM_FREQ_HZ   20000u
#define TIM1_CLK_HZ   16000000u

static void dir_forward(void)
{
    GPIOA->BSRR = (1u << 9) | (1u << (10 + 16));   /* AIN1=1, AIN2=0 */
}

static void dir_reverse(void)
{
    GPIOA->BSRR = (1u << (9 + 16)) | (1u << 10);   /* AIN1=0, AIN2=1 */
}

static void dir_coast(void)
{
    GPIOA->BSRR = (1u << (9 + 16)) | (1u << (10 + 16)); /* both low */
}

void motor_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* PA8: AF mode, AF1 = TIM1_CH1 */
    GPIOA->MODER &= ~(0x3u << (8 * 2));
    GPIOA->MODER |=  (0x2u << (8 * 2));
    GPIOA->AFR[1] &= ~(0xFu << ((8 - 8) * 4));
    GPIOA->AFR[1] |=  (0x1u << ((8 - 8) * 4));

    /* PA9, PA10: general-purpose outputs (AIN1, AIN2) */
    GPIOA->MODER &= ~(0x3u << (9 * 2));
    GPIOA->MODER |=  (0x1u << (9 * 2));
    GPIOA->MODER &= ~(0x3u << (10 * 2));
    GPIOA->MODER |=  (0x1u << (10 * 2));

    dir_coast();

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    TIM1->PSC = 0;
    TIM1->ARR = (uint16_t)((TIM1_CLK_HZ / PWM_FREQ_HZ) - 1u);

    /* CH1: PWM mode 1 (OC1M=110), preload enabled */
    TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM1->CCMR1 |= (0x6u << 4);
    TIM1->CCMR1 |= TIM_CCMR1_OC1PE;

    TIM1->CCER |= TIM_CCER_CC1E;
    TIM1->BDTR |= TIM_BDTR_MOE;          /* advanced timer: no MOE, no output */

    TIM1->CR1 |= TIM_CR1_ARPE;
    TIM1->CR1 |= TIM_CR1_CEN;

    TIM1->CCR1 = 0;
}

void motor_set(int16_t duty_percent)
{
    if (duty_percent > 100)  duty_percent = 100;
    if (duty_percent < -100) duty_percent = -100;

    uint16_t mag = (uint16_t)(duty_percent < 0 ? -duty_percent : duty_percent);

    if (duty_percent > 0)      dir_forward();
    else if (duty_percent < 0) dir_reverse();
    else                       dir_coast();

    TIM1->CCR1 = (uint16_t)(((uint32_t)(TIM1->ARR + 1u) * mag) / 100u);
}

void motor_brake(void)
{
    TIM1->CCR1 = 0;
    GPIOA->BSRR = (1u << 9) | (1u << 10);          /* AIN1=AIN2=1 */
}
