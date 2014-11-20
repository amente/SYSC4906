#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions

#include "CU_TM4C123.h"                     // CU_SYSC4906::Device:Startup
#include <stdio.h>
#include "uart.h"
#include "ST7066U_LCD.h"
#include <string.h>

#define UART_OUT
//#define SD_OUT

// ADC sampling frequency in Hz
#define SAMPLE_FREQ     5000
#define DIFF_THRS       15000

/*  Global Resources  */
#define CoreClock       16000000
#define ms_ticks        (CoreClock/1000)
static uint16_t BPM[5];
static int32_t PEAK = -1;
static char buf[100];

void ADC0SS0_Handler()
{
    // check if electros are connected
    if (GPIOD->DATA & 0x3)
    {
        // not connected so ADC is sampling nothing
        // send 0x80000000 with MSB for future uses
        osMessagePut(SampleQId, 0x80000000, 0);
    }
    else
    {
        // obtain sample and put it on the sample queue
        osMessagePut(SampleQId, (uint32_t)(ADC0->SSFIFO0 & 0xFFF), 0);
    }
    // clear interrupt status
    ADC0->ISC |= ADC_ISC_IN0;
}

void _add_BPM(uint16_t BPM[5], uint32_t tav)
{
    // shift BPM
    BPM[0] = BPM[1];
    BPM[1] = BPM[2];
    BPM[2] = BPM[3];
    BPM[3] = BPM[4];
    // calculate the BPM
    BPM[4] = 60 * CoreClock / tav;
}

void log_samples(uint16_t sample)
{
#ifdef SD_OUT
        uart_sendbytes(UART5, &sample, sizeof(sample));
#endif
        
#ifdef UART_OUT 
        uart_sendbytes(UART0, &sample, sizeof(sample));
#endif
}

void Samples_Thread (void const *argument)
{
    static uint16_t last_sample;
    static int32_t diff3, last_diff3 = 0;
    uint16_t sample;
    uint32_t tav, zc_stamp = 0;
    int32_t diff;
    osEvent rc;
    
    while(1)
    {
        // wait 1s for a new sample from the ADC
        rc = osMessageGet(SampleQId, osWaitForever);
        
        if (rc.value.v & 0x80000000)
        {
            // add a big tav that will make BPM zero
            _add_BPM(BPM, 0xFFFFFFFF);
            continue;
        }
                
        // get the sample from the Q
        sample = (uint16_t)rc.value.v;
        
        // calucate -ve diff cubed
        diff = last_sample - sample;
        diff3 = diff * diff * diff;

        // save last sample
        last_sample = sample;
        last_diff3 = diff3;
        
        // check if 150ms had passed since the last calculation
        if ((tav = TIMER1->TAV) > 150 * ms_ticks)
        {
            // check if timed out (3000 ms) ie. < 20 BPM
            if(tav >= 3000 * ms_ticks)
            {
                // clear BPM samples
                BPM[0] = BPM[1] = BPM[2] = BPM[3] = BPM[4] = 0;
                // reset the timer
                TIMER1->TAV = 0;
            }
            // check for +ve zero crossing
            else if (diff3 >= 0 && last_diff3 <= 0)
            {
                zc_stamp = tav;
            }
            // check if threshold reached
            else if (diff3 >= DIFF_THRS)
                
            {
                // set peak
                PEAK = diff3;
                // calculate BPM
                _add_BPM(BPM, zc_stamp);
                // reset the timer
                TIMER1->TAV = 0;
            }
        }
        log_samples(sample);
    }  // while(1)
}

#define P_SWAP(a,b) { uint16_t temp=(a);(a)=(b);(b)=temp; }
#define P_SORT(a,b) { if ((a)>(b)) P_SWAP((a),(b)); }
uint16_t get_BPM_med(uint16_t _BPM[5]) {
    // save a copy of the BPM array
    uint16_t bpm[5];
    memcpy(bpm, BPM, 5*sizeof(uint16_t));
    //Optimized median calculation for five values
    P_SORT(bpm[0],bpm[1]) ; P_SORT(bpm[3],bpm[4]) ; P_SORT(bpm[0],bpm[3]) ;
    P_SORT(bpm[1],bpm[4]) ; P_SORT(bpm[1],bpm[2]) ; P_SORT(bpm[2],bpm[3]) ;
    P_SORT(bpm[1],bpm[2]) ;
    return(bpm[2]) ;
}

void GUI_Thread (void const *argument)
{
    uint16_t heart_t = 0, X_t = 0, BPM_t = 0;
    uint8_t heart=0, X=0;
    
    /* all modulus math here are with powers of 2
       because precise timing is not required */
    while(1)
    {      
        // check if electrods are disconnected
        if (GPIOD->DATA & 0x3 || X)
        {
            // display X every 512ms
            if (X_t-- == 0)
            {
                LCD_write(RS_ADDR, LCD_DDRAM_LINE2_ADDR+8);
                LCD_write(RS_DATA, (X ? ' ' : LCD_FONT_X_CODE));
                X = !X;
                X_t = 64;
            }
        }
        // check if heart needs to be displayed
        else if (PEAK != -1)
        {
            // display heart
            LCD_write(RS_ADDR, LCD_DDRAM_LINE2_ADDR+8);
            LCD_write(RS_DATA, LCD_FONT_HEART_CODE);
            // beep!
            PWM0->ENABLE |= PWM_ENABLE_PWM3EN;
            PEAK = -1;
            heart = 1;
        }
        // check if heart is already displaying
        else if(heart)
        {
            if(heart_t-- == 0)
            {
                // clear heart symbol
                LCD_write(RS_ADDR, LCD_DDRAM_LINE2_ADDR+8);
                LCD_write(RS_DATA, ' ');
                // stop beep
                PWM0->ENABLE &= ~PWM_ENABLE_PWM3EN;
                heart = 0;
                heart_t = 16;
            }
        }
        
        // display BPM every 512ms
        if (BPM_t-- == 0)
        {
            snprintf(buf, sizeof(buf), "BPM = %d  ", get_BPM_med(BPM));
            LCD_write_str(buf, LCD_DDRAM_LINE1_ADDR+4);
            BPM_t = 64;
        }
        
        osDelay(8);
    }
}

void main_init()
{
/**
    Sample ECG @ SAMPLE_FREQ on AIN0 (PE3)
    Timer 0 is used to trigger the ADC
    Timer 1 is used to time the heart beats
    PWM Beeper on M0PWM3 (PB5)
    Electro sensing on PD0 and PD1. High = N/C
*/
    
/* Clocks */    
    // enable clock to Port E and Port B and Port D
    SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R4 | SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R3;
    // enable clock to Timer0 and Timer1
    SYSCTL->RCGCTIMER |= SYSCTL_RCGCGPIO_R0 | SYSCTL_RCGCGPIO_R1;
    // enable clock to ADC0
    SYSCTL->RCGCADC |= SYSCTL_RCGCADC_R0;
    // enable clock to PWM Module 0
    SYSCTL->RCGCPWM |= SYSCTL_RCGCPWM_R0;
    // PWM clock is SycClk / 64
    SYSCTL->RCC |= SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_64; 
    
/* ADC input AIN0 (PE3) */  
    // set AFSEL for PE3
    GPIOE->AFSEL |= 1<<3;
    // clear DEN for PE3
    GPIOE->DEN &= ~(1<<3);
    // Disable analog isolation on PE3
    GPIOE->AMSEL |= 1<<3;
    
    // select trigger event to be timer
    ADC0->EMUX |= ADC_EMUX_EM0_TIMER;
    // set the first sample to be from AIN0 (PE3)
    ADC0->SSMUX0 |= 0<<ADC_SSMUX0_MUX0_S;
    // end and interrupt after the first sample
    ADC0->SSCTL0 |= ADC_SSCTL0_END0 | ADC_SSCTL0_IE0;
    // set seq 0 to be active
    ADC0->ACTSS |= ADC_ACTSS_ASEN0;
    // enable x64 over sampling
    ADC0->SAC = ADC_SAC_AVG_64X;
    // enable dithering
    ADC0->CTL |= ADC_CTL_DITHER;
    // enable interrupt from ADC0
    ADC0->IM |= 1<<0;
 
/* ADC trigger timer*/ 
    // set Timer 0 to 32bit mode
    TIMER0->CFG = TIMER_CFG_32_BIT_TIMER;
    // periodic mode
    TIMER0->TAMR |= TIMER_TAMR_TAMR_PERIOD;
    // set sampling freq
    TIMER0->TAILR = CoreClock / SAMPLE_FREQ;
    // enable Timer 0 interrupt
    TIMER0->IMR |= TIMER_IMR_TATOIM;
    // start the timer at 0
    TIMER0->TAV = 0;
    // enable Timer A and ADC trigger
    TIMER0->CTL |= TIMER_CTL_TAEN | TIMER_CTL_TAOTE;
    
/* Free-running timer */
    // set Timer 1 to 32bit mode
    TIMER1->CFG = TIMER_CFG_32_BIT_TIMER;
    // periodic mode and count up
    TIMER1->TAMR |= TIMER_TAMR_TAMR_PERIOD | TIMER_TAMR_TACDIR;
    // set max timeout (268.4s)
    TIMER1->TAILR = 0xFFFFFFFF;
    // enable Timer A
    TIMER1->CTL |= TIMER_CTL_TAEN;
    
/* PWM output for beeper */
    GPIOB->AFSEL |= 1<<5;
    GPIOB->PCTL |= 4<<(5*4);
    GPIOB->DEN  |= 1<<5;
    
    PWM0->_1_CTL &= ~PWM_1_CTL_ENABLE;
	PWM0->_1_GENB = PWM_1_GENB_ACTLOAD_ZERO | PWM_1_GENB_ACTCMPBD_ONE;
    PWM0->_1_LOAD = CoreClock  / (2000 * 64);     // set beeper freq to 2 KHz
    PWM0->_1_CMPB = PWM0->_1_LOAD / 16;
    PWM0->_1_CTL |= PWM_1_CTL_ENABLE;
	PWM0->ENABLE &= ~PWM_ENABLE_PWM3EN;
    
/* Electros Sensing */
    // set PD0 and PD1 to inputs
    GPIOD->DIR &= ~(1<<0 | 1<<1);
    // enable PD0 & PD1 pads
    GPIOD->DEN |= (1<<0 | 1<<1);
    
/* UARTs */
    // init UART0
    uart_init(UART0, 115200);
    
    // init UART5
    uart_init(UART5, 115200);
    
/* LCD */
    // init the LCD
    LCD_init();
    // write the custom fonts
    LCD_write_nstr(LCD_FONT_HEART, 8, LCD_FONT_HEART_ADDR);
    LCD_write_nstr(LCD_FONT_X, 8, LCD_FONT_X_ADDR);
    
/* Interrupts */
    // enable ADC interrupts at the NVIC
    NVIC_EnableIRQ(ADC0SS0_IRQn);
}

int main()
{
    // main init
    main_init();
    
    // initialize CMSIS-RTOS
    osKernelInitialize ();                    

    // create the sample queue
    SampleQId = osMessageCreate(osMessageQ(SampleQ), NULL);
    
    // create GUI thread
    osThreadCreate (osThread(GUI_Thread), NULL);
    
    // create sample thread
    osThreadCreate (osThread(Samples_Thread), NULL);

    // start thread execution 
	osKernelStart ();
}
