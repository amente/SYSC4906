#include "CU_TM4C123.h"
#include "uart.h"
#include "ir.h"
#include <stdio.h>

void UART0_Handler()
{
    if (UART0->MIS & (UART_MIS_RXMIS | UART_MIS_RTMIS))
    {
        NVIC_DisableIRQ(UART0_IRQn);  // not using it
        //uart_sendbyte(UART5, uart_getbyte(UART0));
    }
}

void UART5_Handler()
{
    if (UART5->MIS & (UART_MIS_RXMIS | UART_MIS_RTMIS))
    {
        NVIC_DisableIRQ(UART5_IRQn);  // not using it
        //uart_sendbyte(UART0, uart_getbyte(UART5));
    }
}

void carrier_on()
{
    // generate a 38kHz carrier wave on pin PB5
    SYSCTL->RCGCGPIO |= 1<<1;
    SYSCTL->RCGCPWM |= 1<<0;
    SYSCTL->RCGCGPIO;

    GPIOB->DIR |= 1<<5; // PB5 = OUTPUT
    GPIOB->AFSEL |= 1<<5;
    GPIOB->PCTL |= GPIO_PCTL_PB5_M0PWM3;
    GPIOB->DEN |= 1<<5;
    
    PWM0->_1_CTL = 0;
    PWM0->_1_GENB = 0x80C;
    PWM0->_1_LOAD = SystemCoreClock / 38000;
    PWM0->_1_CMPB = PWM0->_1_LOAD >> 1;  // 50% duty
    PWM0->_1_CTL |= 1<<0;
    PWM0->ENABLE |= 1<<3;
}


uint8_t cmds[][4] =   {{0x05, 0x04, 0x06, 0x07},
                       {0x09, 0x08, 0x0A, 0x0B},
                       {0x0D, 0x0C, 0x0E, 0x0F},
                       {0x15, 0x14, 0x16, 0x17},
                       {0x19, 0x18, 0x1A, 0x1B},
                       {0x11, 0x10, 0x12, 0x13}};

#define UART_DEMO                       
                       
int main()
{
    // PB5 configured as M0PWM3 (AFSEL = 4) @ 38kHz carrier 
    // UART5_RX = PE4
    // UART5_TX = PE5
    // Potential conflict on PB5
    
    int c;
    uint8_t nline = 0, input[2];
    uint8_t *input_p = input;
    uart_init(UART0, 600);
    carrier_on();
    
#ifdef UART_DEMO
    uart_init(UART5, 600);
    while(1)
    {
        if ((c = uart_getbyte(UART0)) != EOF)
        {
            uart_sendbyte(UART5, c);
        }
        if ((c = uart_getbyte(UART5)) != EOF)
        {
            uart_sendbyte(UART0, c);
        }
    }
#else    
    ir_init();
    while(1)
    {
        if ((c = uart_getbyte(UART0)) != EOF)
        {
           uart_sendbyte(UART0, c);
           *input_p = c - '0' - 1;
            input_p++;
            if ((input_p - input) == 2)
            {
                if (input[0] < 4 && input[1] < 6) 
                    ir_sendcmd(IR_ADDR, cmds[input[1]][input[0]]);
                input_p = input;
                uart_sendstr(UART0, "\x1B[E");
                if (nline < 23)
                    nline++;
                else
                    uart_sendstr(UART0, "\x1B[S");
            }
        }
    }
#endif
}
