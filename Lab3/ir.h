#include "CU_TM4C123.h"
#include <stdint.h>

#ifndef __IR_H
#define __IR_H

#define DIV_562_5   1785
#define IR_ADDR     0x00

void ir_init(void);
void ir_sendcmd(uint8_t addr, uint8_t cmd);
    
#endif
