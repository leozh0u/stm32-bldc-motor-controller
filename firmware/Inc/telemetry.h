/**
 * telemetry.h — 14-byte framed telemetry, wire format defined in
 * telemetry/protocol.py. Keep the two packers in sync manually.
 */
#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>

#define TELEM_FRAME_LEN 14

/* Packs one frame into buf (must hold TELEM_FRAME_LEN bytes). */
void telemetry_pack(uint8_t buf[TELEM_FRAME_LEN],
                    uint32_t timestamp_ms,
                    int16_t rpm,
                    int16_t pwm_duty,
                    int16_t setpoint_rpm);

/* Packs and sends one frame over USART2 (uart_init() must have run). */
void telemetry_send(uint32_t timestamp_ms,
                    int16_t rpm,
                    int16_t pwm_duty,
                    int16_t setpoint_rpm);

#endif
