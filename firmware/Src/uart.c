/**
 * uart.c — USART2 register-level init per RM0368. APB1 = 16MHz (HSI, no
 * prescale). 115200 baud, OVER8=0: USARTDIV = 16e6/(16*115200) = 8.6805
 * -> mantissa 8, fraction round(0.6805*16) = 11 -> BRR = 0x08B
 * -> actual 115108 baud, -0.08% error.
 *
 * PA3 (RX) is configured too so the pin is claimed, but only TX is enabled
 * for now.
 */
#include "stm32f401xe.h"
#include "uart.h"

void uart_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2, PA3: AF mode, AF7 = USART2_TX/RX */
    GPIOA->MODER &= ~((0x3u << (2 * 2)) | (0x3u << (3 * 2)));
    GPIOA->MODER |=  ((0x2u << (2 * 2)) | (0x2u << (3 * 2)));
    GPIOA->AFR[0] &= ~((0xFu << (2 * 4)) | (0xFu << (3 * 4)));
    GPIOA->AFR[0] |=  ((0x7u << (2 * 4)) | (0x7u << (3 * 4)));

    USART2->BRR = (8u << 4) | 11u;                 /* 115200 @ 16MHz */
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;     /* 8N1, TX enable */
}

void uart_write(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        while (!(USART2->SR & USART_SR_TXE)) { }
        USART2->DR = data[i];
    }
    while (!(USART2->SR & USART_SR_TC)) { }        /* drain last byte */
}
