#include "CU_TM4C123.h"
#include <stdint.h>

#define LCD_DDRAM_LINE1_ADDR 0x80
#define LCD_DDRAM_LINE2_ADDR 0xC0

#define LCD_CONTROL_PORT       GPIOE
#define LCD_CONTROL_PORT_RCGC  (1UL<<4)
#define LCD_CONTROL_RS_PIN     (1UL<<1)
#define LCD_CONTROL_RW_PIN     (1UL<<2)
#define LCD_CONTROL_EN_PIN     (1UL<<3)

#define LCD_DATA_PORT          GPIOB
#define LCD_DATA_PORT_RCGC     (1UL<<1)

#define LCD_RW(x)   ((x) ?(LCD_CONTROL_PORT->DATA |= 0x04):(LCD_CONTROL_PORT->DATA &= ~0x04))
#define LCD_RS(x)   ((x) ?(LCD_CONTROL_PORT->DATA |= 0x02):(LCD_CONTROL_PORT->DATA &= ~0x02))
#define LCD_EN(x)   ((x) ?(LCD_CONTROL_PORT->DATA |= 0x08):(LCD_CONTROL_PORT->DATA &= ~0x08))

#define RS_ADDR	0
#define RS_DATA	1

#define LCD_write(RS,data)   				__LCD_write(RS,data,1)
#define LCD_write_no_wait(RS,data)	__LCD_write(RS,data,0)

void __LCD_write(uint8_t RS,uint8_t data,uint8_t wait);
uint8_t LCD_read(uint8_t RS);
void LCD_init(void);
void LCD_write_str(char *str,uint8_t pos);
