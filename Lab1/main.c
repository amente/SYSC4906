#include "ST7066U_LCD.h"


#define BUTTON_PORT GPIOF   // SW1 Button on Launchpad 
#define BUTTON_PORT_RCGC (1UL<<5)
#define BUTTON_SW1_PIN (1UL<<4)



int 
main(void){
	  uint8_t count=0;
    // Enable lcd control and data GPIO ports
		SYSCTL->RCGCGPIO |= LCD_CONTROL_PORT_RCGC | LCD_DATA_PORT_RCGC | BUTTON_PORT_RCGC;
	  // Dummy read
		SYSCTL->RCGCGPIO;
	
	  SYSCTL->RCGCGPIO |= 1<<2;
    SYSCTL->RCGCPWM |= 1<<0;
	
	  GPIOC->AFSEL |= 1<<4;
    GPIOC->PCTL |= 4<<(4*4);
    GPIOC->DEN |= 1<<4;  
	
	
		//SYSCTL->RCC |= (0x10<<20); // Use system clock PWMDIV and set to /2*/
	
	  //Enable clock to Time module (Timer 2
	  //SYSCTL->RCGCTIMER |= (1UL<<2);
	  
	  
	  	   
		 //Set the direction and enable of all lcd control pins
	  LCD_CONTROL_PORT->DIR |= LCD_CONTROL_RS_PIN | LCD_CONTROL_RW_PIN |  LCD_CONTROL_EN_PIN;  
    LCD_CONTROL_PORT->DEN |= LCD_CONTROL_RS_PIN | LCD_CONTROL_RW_PIN |  LCD_CONTROL_EN_PIN;		
	  
	  //Enable all lcd data port pins
    LCD_DATA_PORT->DEN = 0xFF;  
	
	  
	  //Enable SW1 button pin, 
	  //BUTTON_PORT->DIR &= ~BUTTON_SW1_PIN;
	 // BUTTON_PORT->DEN |= BUTTON_SW1_PIN;
		//BUTTON_PORT->AFSEL |= BUTTON_SW1_PIN;
		//BUTTON_PORT ->PCTL = (BUTTON_PORT -> PCTL & ~(0xF<<28))|(0x7<<28); // Select T2CCP0
		
		/*
    //Configure timer 2
		TIMER2->CTL &= ~(1UL<<0); // Disable timer
    TIMER2->CFG = 0x4UL; // Choose 16bit timer
		TIMER2->TAMR |= 0x3UL; // Set timer to capture mode 
		// TIMER2-CTL |= xx ,Default edge count is positive 0
		//TIMER2->TAMATCHR . Default up count mode
		TIMER2->CTL|= (1UL<<0); // Enable timer
				
		
	  //BUTTON_PORT->PUR |= BUTTON_SW1_PIN;
		
		*/
	  
    PWM0->_3_CTL = 0;
    PWM0->_3_GENA = 0x8C;
       
    PWM0->_3_LOAD = 0x18F;
    PWM0->_3_CMPA = 0x12B;
     
    PWM0->_3_CTL |= 1<<0;
       
    PWM0->ENABLE |= 1<<6;
	  

	  LCD_init();
	  LCD_write_str("Hello World!", LCD_DDRAM_LINE1_ADDR);		
		while(1){		
			//	while(TIMER2->RIS 
		} 	
}
