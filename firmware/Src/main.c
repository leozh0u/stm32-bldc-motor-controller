/**
 * main.c — TIM1 PWM on PA8 (TIM1_CH1), direction pins PA9/PA10
 * Drives TB6612FNG motor A. STBY must be tied HIGH externally or via GPIO.
 */

#include "stm32f401xe.h"

#define PWM_FREQ_HZ   20000   // 20kHz, above audible range
#define TIM1_CLK_HZ   16000000

static void gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER &= ~(0x3 << (8 * 2));
    GPIOA->MODER |=  (0x2 << (8 * 2));
    GPIOA->AFR[1] &= ~(0xF << ((8 - 8) * 4));
    GPIOA->AFR[1] |=  (0x1 << ((8 - 8) * 4));

    GPIOA->MODER &= ~(0x3 << (9 * 2));
    GPIOA->MODER |=  (0x1 << (9 * 2));
    GPIOA->MODER &= ~(0x3 << (10 * 2));
    GPIOA->MODER |=  (0x1 << (10 * 2));
}

static void tim1_pwm_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    uint16_t period = (TIM1_CLK_HZ / PWM_FREQ_HZ) - 1;

    TIM1->PSC = 0;
    TIM1->ARR = period;

    TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM1->CCMR1 |= (0x6 << 4);
    TIM1->CCMR1 |= TIM_CCMR1_OC1PE;

    TIM1->CCER |= TIM_CCER_CC1E;

    TIM1->BDTR |= TIM_BDTR_MOE;

    TIM1->CR1 |= TIM_CR1_ARPE;
    TIM1->CR1 |= TIM_CR1_CEN;

    TIM1->CCR1 = 0;
}

static void motor_set_duty(uint16_t duty_percent)
{
    uint32_t ccr = ((uint32_t)(TIM1->ARR + 1) * duty_percent) / 100;
    TIM1->CCR1 = ccr;
}

static void motor_forward(void)
{
    GPIOA->ODR |= (1 << 9);
    GPIOA->ODR &= ~(1 << 10);
}

int main(void)
{
    gpio_init();
    tim1_pwm_init();
    motor_forward();
    motor_set_duty(50);

    for (;;);
}
