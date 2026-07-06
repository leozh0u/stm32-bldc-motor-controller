/**
 * encoder.h — quadrature decode via TIM2 hardware encoder mode, PA0/PA1 (AF1).
 * TIM2 is 32-bit, so count deltas via unsigned subtraction are wraparound-safe
 * and sign-correct in two's complement.
 */
#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* Counts per revolution of the OUTPUT shaft, at x4 decode.
 * PLACEHOLDER — MUST be measured on hardware before RPM values mean anything:
 * mark the shaft, rotate exactly one turn by hand, read encoder_count().
 * (JGB37-520 is typically 11 PPR at the motor x4 decode x gear ratio;
 * gear ratio varies by variant, so measure, don't trust a datasheet.) */
#define ENCODER_CPR 1320

void encoder_init(void);
uint32_t encoder_count(void);     /* raw hardware counter, free-running */

/* Signed RPM of the output shaft, computed from the count delta since the
 * previous call. Call at a fixed period and pass that period in ms. */
int16_t encoder_rpm(uint32_t period_ms);

#endif
