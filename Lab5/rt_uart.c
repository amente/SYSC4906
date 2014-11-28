#include <stdio.h>
#include "uart.h"

#pragma import(__use_no_semihosting_swi)

struct __FILE { UART0_Type *uart; };
FILE __stdout = { UART0 };
FILE __stdin = { UART0 };

int fputc(int ch, FILE *f) {
  return (uart_sendbyte(f->uart, ch));
}

int ferror(FILE *f) {
  /* Your implementation of ferror */
  return EOF;
}

void _sys_exit(int return_code) {
  while (1);    /* endless loop */
}

void _ttywrch(int ch)
{
    uart_init(UART0, 9600);
    fputc(ch, &__stdout);
}
