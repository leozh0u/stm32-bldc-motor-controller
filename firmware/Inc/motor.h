/**
 * motor.h — TB6612FNG motor A via TIM1_CH1 PWM (PA8) + AIN1/AIN2 (PA9/PA10).
 * STBY must be tied HIGH externally or the driver outputs nothing.
 */
#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

void motor_init(void);            /* GPIO + TIM1 20kHz PWM, starts at 0 duty */

/* Signed duty: -100..+100. Positive = forward (AIN1=1, AIN2=0),
 * negative = reverse, 0 = coast (both low, PWM 0). Clamps out of range. */
void motor_set(int16_t duty_percent);

void motor_brake(void);           /* AIN1=AIN2=1, short brake */

#endif
