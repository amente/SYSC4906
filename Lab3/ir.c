#include "CU_TM4C123.h"
#include "ir.h"

#define INVERT_OUT

// Send out NEC IR signals
void ir_init()
{
    // use PE5 aka U5TX as IR output
    // PB5 and PE5 are fed into a NOR gate
    // PE5 needs to be inverted 
    
    // enable clock to Port B & E
    SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R4;
    // enable clock to TIMER1
    SYSCTL->RCGCTIMER |= SYSCTL_RCGCTIMER_R1;
    
    // configure PE5 as GPIO
    GPIOE->DIR |= 1<<5;
    GPIOE->AFSEL &= ~(1<<5);
    GPIOE->PCTL &= ~GPIO_PCTL_PE5_M;
    GPIOE->DEN |= 1<<5;
#ifdef INVERT_OUT
    GPIOE->DATA |= 1<<5;  // normally 1 after inversion
#endif
    
    // configure T1CCP1 aka Timer B for one-shot
    TIMER1->CFG = TIMER_CFG_16_BIT;
    TIMER1->TBPR = 16 - 1;  // prescale = 16
    TIMER1->TBMR |= TIMER_TBMR_TBILD | TIMER_TBMR_TBMR_1_SHOT;
    //TIMER1->IMR |= TIMER_IMR_TBTOIM;

    //NVIC_EnableIRQ(TIMER1B_IRQn);
}

/*      PB5 IS INVERTED     */
void __ir_sendhigh(uint16_t div)
{
    TIMER1->TBILR = SystemCoreClock / 16 / div - 1;
#ifdef INVERT_OUT
    GPIOE->DATA &= ~(1<<5);
#else
    GPIOE->DATA |= 1<<5;
#endif
    TIMER1->CTL |= TIMER_CTL_TBEN;
    while( !(TIMER1->RIS & TIMER_RIS_TBTORIS) )
        ;
    TIMER1->ICR |= TIMER_ICR_TBTOCINT;
}

/*      PB5 IS INVERTED     */
void __ir_sendlow(uint16_t div)
{
    TIMER1->TBILR = SystemCoreClock / 16 / div - 1;
#ifdef INVERT_OUT
    GPIOE->DATA |= 1<<5;
#else
    GPIOE->DATA &= ~(1<<5);
#endif
    TIMER1->CTL |= TIMER_CTL_TBEN;
    while( !(TIMER1->RIS & TIMER_RIS_TBTORIS) )
        ;
    TIMER1->ICR |= TIMER_ICR_TBTOCINT;
}

/*      PB5 IS INVERTED     */
void __ir_sendbit(uint8_t b)
{
    __ir_sendhigh(DIV_562_5);   // 562.5 us
    __ir_sendlow( (b ? DIV_562_5 / 3: DIV_562_5) );   // 562.5 us or 1.6875 ms
}

void __ir_sendbyte(uint8_t byte)
{
    uint8_t i;
    for (i=0; i<8; i++)
    {
            __ir_sendbit(byte & 1);
            byte >>= 1;
    }
}

/*      PB5 IS INVERTED     */
void ir_sendcmd(uint8_t addr, uint8_t cmd)
{
    // make sure carrier_off() is called before using this function
    static uint8_t last_cmd = 0;
    
    //if (cmd == last_cmd)
    {
        // start
        __ir_sendhigh(DIV_562_5 / 16);  // 9    ms
        __ir_sendlow(DIV_562_5 / 8);    // 4.5  ms
        
        // address
        __ir_sendbyte(addr);
        __ir_sendbyte(~addr);
        
        // cmd
        __ir_sendbyte(cmd);
        __ir_sendbyte(~cmd);
        
        // stop
        __ir_sendhigh(DIV_562_5);       // 562.5 us
        
        // cool down
        __ir_sendlow(DIV_562_5 / 71);   // ~40 ms
        
        last_cmd = cmd;
    }
    //else
    {
        // TODO: repeat code
    }   
}
