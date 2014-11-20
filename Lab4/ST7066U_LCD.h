#include "CU_TM4C123.h"
#include <stdint.h>

#define LCD_DDRAM_LINE1_ADDR 0x80
#define LCD_DDRAM_LINE2_ADDR 0xC0

#define LCD_CGRAM_ADDR       0x40

#define LCD_CONTROL_PORT       GPIOF
#define LCD_CONTROL_PORT_RCGC  (1UL<<5)
#define LCD_CONTROL_RS_PIN     (1UL<<2)
#define LCD_CONTROL_RW_PIN     (1UL<<3)
#define LCD_EN_CONTROL_PORT       GPIOB
#define LCD_EN_CONTROL_PORT_RCGC (1UL<<1)
#define LCD_CONTROL_EN_PIN     (1UL<<3)

#define LCD_DATA_PORT          GPIOC
#define LCD_DATA_PORT_RCGC     (1UL<<2)

#define LCD_RW(x)   ((x) ?(LCD_CONTROL_PORT->DATA |= LCD_CONTROL_RW_PIN):(LCD_CONTROL_PORT->DATA &= ~LCD_CONTROL_RW_PIN))
#define LCD_RS(x)   ((x) ?(LCD_CONTROL_PORT->DATA |= LCD_CONTROL_RS_PIN):(LCD_CONTROL_PORT->DATA &= ~LCD_CONTROL_RS_PIN))
#define LCD_EN(x)   ((x) ?(LCD_EN_CONTROL_PORT->DATA |= LCD_CONTROL_EN_PIN):(LCD_EN_CONTROL_PORT->DATA &= ~LCD_CONTROL_EN_PIN))

#define RS_ADDR	0
#define RS_DATA	1

#define LCD_write(RS,data)   		__LCD_write(RS,data,1)
#define LCD_write_no_wait(RS,data)	__LCD_write(RS,data,0)

#define LCD_FONT_HEART          "\x00\x00\x0a\x1f\x1f\x0e\x04\x00"
#define LCD_FONT_HEART_CODE     0
#define LCD_FONT_HEART_ADDR     (LCD_CGRAM_ADDR+8*LCD_FONT_HEART_CODE)
#define LCD_FONT_X              "\x00\x1b\x0e\x04\x0e\x1b\x00\x00"
#define LCD_FONT_X_CODE         1
#define LCD_FONT_X_ADDR         (LCD_CGRAM_ADDR+8*LCD_FONT_X_CODE)

void __LCD_write(uint8_t RS,uint8_t data,uint8_t wait);
uint8_t LCD_read(uint8_t RS);
void LCD_init(void);
void LCD_write_str(char *str,uint8_t pos);
void LCD_write_nstr(char *str, uint8_t n, uint8_t pos);
