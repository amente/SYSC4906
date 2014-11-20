#include <stdio.h>
#include <stdbool.h>

#include "CU_TM4C123.h"
#include "uart.h"

void uart_int_off(UART0_Type *uart)
{
    uart->IM &= ~(UART_IM_RXIM | UART_IM_RTIM);
}

void uart_int_on(UART0_Type *uart)
{
    uart->IM |= UART_IM_RXIM | UART_IM_RTIM;
}

/*
    Configure UART to baud, 8-N-1, interrupt on Rx
    Returns -1 if no init code is available  otherwise 0 for success
    Don't forget to implement the RX ISRs
*/
int uart_init(UART0_Type *uart, uint32_t baud)
{
    if (uart == UART0)
    {
        // UART0_RX = PA0
        // UART0_TX = PA1
        
        // enable clock to Port A
        SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R0;
        // enable clock to UART0
        SYSCTL->RCGCUART |= SYSCTL_RCGCUART_R0;
        
        // select the alternate function for PA0 & PA1
        GPIOA->AFSEL |= (1<<0) | (1<<1);
        // configure the pins to be U0RX & U0TX
        GPIOA->PCTL |= GPIO_PCTL_PA0_U0RX | GPIO_PCTL_PA1_U0TX;
        // enable the digital pad for PA0 & PA1
        GPIOA->DEN |= (1<<0) | (1<<1);
        
        // enable the interrupt from the NVIC
        NVIC_EnableIRQ(UART0_IRQn);
    }
    else if (uart == UART5)
    {
        // UART5_RX = PE4
        // UART5_TX = PE5 
        
        // enable clock to Port E
        SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R4;
        // enable clock to UART5
        SYSCTL->RCGCUART |= SYSCTL_RCGCUART_R5;
        
        // select the alternate function for PE4 & PE5
        GPIOE->AFSEL |= (1<<4) | (1<<5);
        // configure the pins to be U5RX & U5TX
        GPIOE->PCTL |= GPIO_PCTL_PE4_U5RX | GPIO_PCTL_PE5_U5TX;
        // enable the digital pad for PE4 & PE5
        GPIOE->DEN |= (1<<4) | (1<<5);
        
        // enable the interrupt from the NVIC
        NVIC_EnableIRQ(UART5_IRQn);
    }
    else
    {
        // No init for specifed module
        return -1;
    }
        
    // calculate the int part of the baud rate divider
    uart->IBRD = SystemCoreClock / 16 / baud;
    // calculate the fractional part of the baud rate divider
    uart->FBRD = (((((SystemCoreClock << 3) / baud)) + 1) >> 1) & UART_FBRD_DIVFRAC_M;
    
    // configure the word length to be 8 bits
    uart->LCRH |= UART_LCRH_WLEN_8 | UART_LCRH_FEN;
    // set the RX FIFO Trigger level
    uart->IFLS |= FIFO_TRIG_LVL;
    // DO NOT DO THIS BY DEFAULT: enable RX interrupts
    // uart_int_on(uart);
    // enable TX and RX
    uart->CTL |= UART_CTL_UARTEN;
    
    return 0;
}

uint8_t uart_sendbyte(UART0_Type *uart, uint8_t b)
{
    uart->DR = b;
    while(uart->FR & UART_FR_TXFF);    // busy wait until the 16 byte TX FIFO is no longer full
    return b;
}

void uart_sendbytes(UART0_Type *uart, void *buf, uint16_t len)
{
    while(len--)
    {
        uart_sendbyte(uart, *(char *)buf);
        buf = (char*)buf + 1;
    }
}

void uart_sendstr(UART0_Type *uart, void *buf)
{
    while(*(char*)buf)
    {
        uart_sendbyte(uart, *(char*)buf);
        buf = (char*)buf + 1;
    }
}

int uart_getbyte(UART0_Type *uart)
{
    if (uart->FR & UART_FR_RXFE)
    {
        return EOF;
    }
    else
    {
        return uart->DR;
    }
}

uint16_t uart_getbytes(UART0_Type *uart, void *buf, uint16_t len)
{
    int c;
    uint8_t *bufptr = (uint8_t*) buf;
    while(len-- && ((c = uart_getbyte(uart)) != EOF))
    {
        *bufptr = c;
        bufptr++;
    }
    return (uint32_t)bufptr-(uint32_t)buf;
}

#ifdef __RINGBUF_H
int uart_getbytes_ringbuf(UART0_Type *uart, RING_BUF_t *buf)
{
    int c, rc;
#pragma diag_suppress   1293
    while( ((c = uart_getbyte(uart)) != EOF) && !(rc = (ringbuf_put(buf, c))) )
        ;
#pragma diag_warning    1293
    return rc;
}
#endif
