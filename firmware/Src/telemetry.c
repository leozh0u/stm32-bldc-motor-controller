/**
 * telemetry.c — mirror of telemetry/protocol.py pack_frame():
 * [0] START 0xAA, [1] LEN 10, [2:6] u32 timestamp_ms, [6:8] i16 rpm,
 * [8:10] i16 pwm_duty, [10:12] i16 setpoint_rpm, [12] XOR checksum of
 * payload, [13] END 0x55. Little-endian, same as Cortex-M native order;
 * bytes are still written explicitly so this never depends on struct layout.
 */
#include "telemetry.h"
#include "uart.h"

#define TELEM_START 0xAAu
#define TELEM_END   0x55u
#define PAYLOAD_LEN 10u

static void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)(v >> 8);
}

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

void telemetry_pack(uint8_t buf[TELEM_FRAME_LEN],
                    uint32_t timestamp_ms,
                    int16_t rpm,
                    int16_t pwm_duty,
                    int16_t setpoint_rpm)
{
    buf[0] = TELEM_START;
    buf[1] = PAYLOAD_LEN;
    put_u32(&buf[2], timestamp_ms);
    put_u16(&buf[6], (uint16_t)rpm);
    put_u16(&buf[8], (uint16_t)pwm_duty);
    put_u16(&buf[10], (uint16_t)setpoint_rpm);

    uint8_t cs = 0;
    for (int i = 2; i < 2 + (int)PAYLOAD_LEN; i++) {
        cs ^= buf[i];
    }
    buf[12] = cs;
    buf[13] = TELEM_END;
}

void telemetry_send(uint32_t timestamp_ms,
                    int16_t rpm,
                    int16_t pwm_duty,
                    int16_t setpoint_rpm)
{
    uint8_t buf[TELEM_FRAME_LEN];
    telemetry_pack(buf, timestamp_ms, rpm, pwm_duty, setpoint_rpm);
    uart_write(buf, TELEM_FRAME_LEN);
}
