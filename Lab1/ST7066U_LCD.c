#include "ST7066U_LCD.h"

void
delay(uint32_t ms){
    uint32_t i;
    while(ms>1){
        for(i=16000;i>1;i--){
        }
        ms--;
    }
}

uint8_t
LCD_read(uint8_t RS)
{
    volatile uint8_t buf;
    LCD_DATA_PORT->DIR = 0x00;
    LCD_RS(RS);
    LCD_RW(1);    
    LCD_EN(1);
    buf = LCD_DATA_PORT->DATA;
    LCD_EN(0);
    return buf;
}

void
__LCD_write(uint8_t RS,uint8_t data,uint8_t wait){
    LCD_DATA_PORT->DIR = 0xFF;
    LCD_RS(RS);
    LCD_RW(0);    
    LCD_DATA_PORT->DATA = data;
    LCD_EN(1);
    LCD_EN(0);
    LCD_DATA_PORT->DATA = 0;
    if(wait){
        while(LCD_read(0) & 0x80);
    }
}

void 
LCD_init(void){
      // External reset procedures
    delay(50);
    LCD_write_no_wait(RS_ADDR,0x30);
    delay(5);
    LCD_write_no_wait(RS_ADDR,0x30);
    delay(1);
    LCD_write(RS_ADDR,0x30);// We can start polling BF at this point
    
    LCD_write(RS_ADDR,0x38); // Functions set
    LCD_write(RS_ADDR,0x08); // Display off    
    LCD_write(RS_ADDR,0x01); // Display clear
    LCD_write(RS_ADDR,0x06); // Entry mode set
    LCD_write(RS_ADDR,0x0C); // Display on                  
} 

void 
LCD_write_str(char *str,uint8_t pos){
    LCD_write(RS_ADDR,pos);
    while(*str != '\0'){
        LCD_write(RS_DATA,*(str++));    
    }
}

void 
LCD_write_nstr(char *str, uint8_t n, uint8_t pos){
    LCD_write(RS_ADDR,pos);
    for(; n>0; n--)
    {
        LCD_write(RS_DATA,*(str++));    
    }
}
