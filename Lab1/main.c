#include "ST7066U_LCD.h"
#include <stdio.h>

#define BUTTON_PORT GPIOF   // SW1 Button on Launchpad 
#define BUTTON_PORT_RCGC (1UL<<5)
#define BUTTON_SW1_PIN (1UL<<4)

uint32_t buttoncnt = 0;
uint32_t rawbuttoncnt = 0;

uint8_t buf[16+1];
char text[] = "               Hello World!                ";
char* p = text;
char* q = text+5;

void TIMER0A_Handler()
{
    TIMER0->ICR |= 1<<0;
    LCD_write_nstr(p, 16, LCD_DDRAM_LINE1_ADDR);
    LCD_write_nstr(q, 16, LCD_DDRAM_LINE2_ADDR);  
    if (++p > text+27) p = text;
    if (++q > text+27) q = text;
}

void GPIOF_Handler()
{
    if ((GPIOF->MIS & (1<<4)))  // PF4
    {
        rawbuttoncnt++;
        if (SysTick->CTRL & (1<<16))
        {
            buttoncnt++;
            PWM0->_3_CMPA = (PWM0->_3_CMPA + 0x63) % 0x18E;
            
            SysTick->VAL = 1;   //W1C to reset SysTick
        }
        
        /*snprintf(buf, 16, "RawCount: %d", rawbuttoncnt);
        LCD_write_str(buf, LCD_DDRAM_LINE1_ADDR);
        snprintf(buf, 16, "Count: %d", buttoncnt);
        LCD_write_str(buf, LCD_DDRAM_LINE2_ADDR);*/
        
        GPIOF->ICR |= (1<<4);
    }
    else if ((GPIOF->MIS & (1<<0)))  // PF0
    {
        if (SysTick->CTRL & (1<<16))
        {
            TIMER0->TAILR -= SystemCoreClock / 4;  // 250ms increment
            if (TIMER0->TAILR < SystemCoreClock / 4)
                TIMER0->TAILR = SystemCoreClock;
            SysTick->VAL = 1;   //W1C to reset SysTick
        }
        GPIOF->ICR |= (1<<0);
    }
}

int 
main(void)
{
    
    // Enable lcd control and data GPIO ports
	SYSCTL->RCGCGPIO |= LCD_CONTROL_PORT_RCGC | LCD_DATA_PORT_RCGC | BUTTON_PORT_RCGC;
	// Dummy read
	SYSCTL->RCGCGPIO;
	
	SYSCTL->RCGCGPIO |= 1<<2;
	SYSCTL->RCGCPWM |= 1<<0;
	SYSCTL->RCGCTIMER |= 1<<0;
    
	GPIOC->AFSEL |= 1<<4;
	GPIOC->PCTL |= 4<<(4*4);
	GPIOC->DEN |= 1<<4;  
	  
	// SW1 & SW2
    GPIOF->LOCK = 0x4C4F434B;  // UNLOCK! 
    //lol v
    *(uint32_t*)&GPIOF->CR |= 1<<0;  // ENABLE COMMIT!
    GPIOF->DEN |= (1<<1) | (1<<4) | (1<<0);
    GPIOF->PUR |= (1<<4) | (1<<0);
    GPIOF->IM |= (1<<4) | (1<<0);
	*(uint32_t*)&GPIOF->CR &= ~(1<<0);  // DISABLE COMMIT!
    GPIOF->LOCK = 0x4C4F434B;  // LOCK!  
    
    
	//Set the direction and enable of all lcd control pins
	LCD_CONTROL_PORT->DIR |= LCD_CONTROL_RS_PIN | LCD_CONTROL_RW_PIN |  LCD_CONTROL_EN_PIN;  
	LCD_CONTROL_PORT->DEN |= LCD_CONTROL_RS_PIN | LCD_CONTROL_RW_PIN |  LCD_CONTROL_EN_PIN;		
	  
	//Enable all lcd data port pins
	LCD_DATA_PORT->DEN = 0xFF;  
	
	  
	PWM0->_3_CTL = 0;
	PWM0->_3_GENA = 0x8C;
	   
	PWM0->_3_LOAD = 0x18F;
	PWM0->_3_CMPA = 0;//0x18E;
	 
	PWM0->_3_CTL |= 1<<0;
	   
	PWM0->ENABLE |= 1<<6;
    
    TIMER0->CFG = 0;            // 32bit mode
    TIMER0->TAMR |= 0x02<<0;    // periodic mode 
    
    TIMER0->TAILR = SystemCoreClock;
    TIMER0->IMR |= 1<<0;
    TIMER0->TAV = 0;
    
    TIMER0->CTL |= 1<<0;    
    
	LCD_init();
    NVIC_EnableIRQ(GPIOF_IRQn);
    NVIC_EnableIRQ(TIMER0A_IRQn);
    
    SysTick->LOAD = SystemCoreClock / 1000 * 50;  // 50ms
    SysTick->CTRL |= 1<<0;
    
	while(1){
        // Nothing to do here!
	} 	
}
