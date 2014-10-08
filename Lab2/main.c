#include "CU_TM4C123.h"

#define MIN_AIN0_VAL         0x400
#define MAX_AIN0_VAL         0xB00
#define AIN0_HYS             10

#define MIN_AIN1_VAL         0x560
#define MAX_AIN1_VAL         0xB00
#define MIN_DUTY             1
#define MAX_DUTY             50
#define DUTY_SPREAD          (MAX_DUTY - MIN_DUTY)
#define AIN1_SPREAD          (MAX_AIN1_VAL - MIN_AIN1_VAL)
#define MAP_AIN1_TO_DUTY(x)  (MAX_DUTY - (x - MIN_AIN1_VAL) * (DUTY_SPREAD) / (AIN1_SPREAD))

#define MAP_A_B(x, min_a, max_a, min_b, max_b) (max_b - (x - min_a) * (max_b-min_b) / (max_a-min_a))

#define LOAD_TABLE_SIZE      60
uint16_t load_table[LOAD_TABLE_SIZE];
uint8_t oct_base = 0;
uint8_t is_hbd = 0;
uint8_t pitch_bend = 0;

void ADC0SS0_Handler()
{
    uint16_t pitch_a = ADC0->SSFIFO0;
    uint16_t duty_a = ADC0->SSFIFO0;
    static uint8_t dutycycle = 50;
    static uint8_t hys = 0;   
    
    ADC0->ISC |= 1<<0;
    
    if ((duty_a < MAX_AIN1_VAL))
    {
        dutycycle = 51 - MAP_AIN1_TO_DUTY(duty_a);
    }
    else
    {
        dutycycle = 50;
    }
    if (!is_hbd)
    {
        if ((pitch_a < (hys ? MAX_AIN0_VAL+AIN0_HYS : MAX_AIN0_VAL-AIN0_HYS)) && (pitch_a > MIN_AIN0_VAL))
        {
            PWM0->_3_LOAD = load_table[MAP_A_B(pitch_a, MIN_AIN0_VAL, MAX_AIN0_VAL, oct_base, oct_base+8)];
            hys = 1;
            GPIOC->DEN |= 1<<4;
        }
        else
        {
            hys = 0;
            GPIOC->DEN &= ~(1<<4);
        }
    }
    else
    {
        if ((pitch_a < (hys ? MAX_AIN0_VAL+AIN0_HYS : MAX_AIN0_VAL-AIN0_HYS)) && (pitch_a > MIN_AIN0_VAL))
        {
            pitch_bend = MAP_A_B(pitch_a, MIN_AIN0_VAL, MAX_AIN0_VAL, 0, 10);
            hys = 1;
        }
        else
        {
            pitch_bend = 0;
            hys = 0;
        }
    }
    PWM0->_3_CMPA = PWM0->_3_LOAD * dutycycle / 100;
}

void GPIOF_Handler()
{
    static uint8_t led_table[4] = {0x2, 0x4, 0x8, 0xE};
    GPIOF->ICR |= 1<<4;

    if (SysTick->CTRL & (1<<16))
    {
        oct_base = (oct_base + 8) & (32-1);  // mod 32
        GPIOF->DATA = led_table[oct_base>>3];  // div 8
        SysTick->VAL = 1;   //W1C to reset SysTick
    }    
}

void do_slide()
{
    int i, j;
    
    // slide from 100hz to 10000hz; going up by one semi-tone each time
    for(i=0; i<LOAD_TABLE_SIZE; i++)
    {
        PWM0->_3_LOAD = load_table[i];
        PWM0->_3_CMPA = load_table[i] >> 1;
        for(j=0; j<0x8FFFF; j++);    // random delay
    }       
}

#define HBD_KEY 3
void do_hbd()
{
   int i,j;
   static uint8_t notes[] = {1,1,2,1,4,3,0xFF,1,1,2,1,5,4,0xFF,1,1,8,6,4,3,2,0xFF,7,7,6,4,5,4};
   
   is_hbd = 1;
   for(i=0;i<sizeof(notes);i++){
       if(notes[i] == 0xFF){
           for(j=0; j<0xFFFFF; j++);  
       }else{
           PWM0->_3_LOAD = load_table[notes[i]+(HBD_KEY*8)+pitch_bend];
           //PWM0->_3_CMPA = load_table[notes[i]+(HBD_KEY*8)+pitch_bend] >> 1;
           GPIOC->DEN |= (1<<4);
           for(j=0; j<0x7FFFF; j++); 
       }
       GPIOC->DEN &= ~(1<<4);
       for(j=0; j<0x7FFFF; j++);
   }
   is_hbd = 0;
}

int main()
{
    int n;
    float freq;
    
    for(freq = 65.41, n = 0; n<LOAD_TABLE_SIZE; n++)
    {
        load_table[n] = ((SystemCoreClock >> 6)/freq) - 1;
        if ((n%7 == 2) || (n%7 == 6))
            freq *= 1.05946f;
        else
            freq *= 1.12246f;
    }
    
    SYSCTL->RCGCGPIO |= (1<<2) | (1<<4) | (1<<5);
    SYSCTL->RCGCTIMER |= 1<<0;
    SYSCTL->RCGCADC |= 1<<0;
    SYSCTL->RCGCPWM |= 1<<0;
    SYSCTL->RCC |= (1<<20) | (0x7<<17); // PWM clock is SycClk / 64
    
    GPIOC->AFSEL |= 1<<4;
	GPIOC->PCTL |= 4<<(4*4);
    GPIOC->DR8R |= 1<<4;
	GPIOC->DEN |= 1<<4; 
    
    GPIOE->AMSEL |= (1<<3) | (1<<2);
    GPIOC->DEN &= ~((1<<3) | (1<<2));
    
    GPIOF->PUR |= 1<<4;
    GPIOF->IM |= 1<<4;
    GPIOF->DIR |= 1<<3 | 1<<2 | 1<<1;
    GPIOF->DEN |= 1<<4 | 1<<3 | 1<<2 | 1<<1;
    GPIOF->DATA |= 1<<1;
    
    PWM0->_3_CTL = 0;
	PWM0->_3_GENA = 0x8C; 
	PWM0->_3_CTL |= 1<<0;    
	PWM0->ENABLE |= 1<<6;
    
    TIMER0->CFG = 0;            // 32bit mode
    TIMER0->TAMR |= 0x02<<0;    // periodic mode 
    TIMER0->TAILR = SystemCoreClock/16000;
    TIMER0->IMR |= 1<<0;
    TIMER0->TAV = 0;
    TIMER0->CTL |= (1<<0) | (1<<5);
    
    ADC0->EMUX |= 0x5<<0;
    ADC0->SSMUX0 |= 1<<4;
    ADC0->SSCTL0 |= (1<<5) | (1<<6);
    ADC0->ACTSS |= 1<<0;
    ADC0->IM |= 1<<0;
    
    SysTick->LOAD = SystemCoreClock / 1000 * 50;  // 50ms
    SysTick->CTRL |= 1<<0;
    
    do_slide();
    
    NVIC_EnableIRQ(GPIOF_IRQn);
    NVIC_EnableIRQ(ADC0SS0_IRQn);
    
    do_hbd();
    
    while(1)
    {
            
        
    }
}
