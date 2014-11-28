#include <stdint.h>
#include "CU_TM4C123.h"

#ifndef __UART_H
#define __UART_H

#define FIFO_TRIG_LVL    UART_IFLS_RX4_8     // default RX 1/2 full

int uart_init(UART0_Type *, uint32_t);
void uart_uninit(UART0_Type *uart);
uint8_t uart_sendbyte(UART0_Type *, uint8_t);
void uart_sendbytes(UART0_Type *, void *, uint16_t);
void uart_sendstr(UART0_Type *, void *);
int uart_getbyte(UART0_Type *);
uint16_t uart_getbytes(UART0_Type *, void *, uint16_t);
void uart_int_off(UART0_Type *uart);
void uart_int_on(UART0_Type *uart);

#ifdef __RINGBUF_H
int uart_getbytes_ringbuf(UART0_Type *, RING_BUF_t *);
#endif

#endif
