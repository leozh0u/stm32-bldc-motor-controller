/**
 * encoder.h — quadrature decode via TIM2 hardware encoder mode, PA0/PA1 (AF1).
 * TIM2 is 32-bit, so count deltas via unsigned subtraction are wraparound-safe
 * and sign-correct in two's complement.
 */
#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* Counts per revolution of the OUTPUT shaft, at x4 decode.
 * Computed from the purchased variant — JGB37-520, 12V / 178 rpm = gear
 * ratio 56 — per the encoder datasheet (11 PPR at the motor):
 *     11 PPR x 4 (x4 decode) x 56 (gear ratio) = 2464.
 * STILL confirm on hardware before trusting RPM: build MODE_CALIBRATE_CPR,
 * mark the output shaft, rotate exactly one turn by hand, read
 * encoder_count(), and reconcile against 2464. */
#define ENCODER_CPR 2464

void encoder_init(void);
uint32_t encoder_count(void);     /* raw hardware counter, free-running */

/* Signed RPM of the output shaft, computed from the count delta since the
 * previous call. Call at a fixed period and pass that period in ms. */
int16_t encoder_rpm(uint32_t period_ms);

#endif
