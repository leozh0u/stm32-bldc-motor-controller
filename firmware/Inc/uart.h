/**
 * uart.h — USART2 TX on PA2 (AF7), 115200 8N1, routed through ST-LINK VCP.
 * Blocking TX only; one 14-byte telemetry frame takes ~1.2ms on the wire.
 */
#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stddef.h>

void uart_init(void);
void uart_write(const uint8_t *data, size_t len);   /* blocking */

#endif
