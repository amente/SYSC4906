#include "CU_TM4C123.h"                 // CU_SYSC4906::Device:Startup
#include "osObjects.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fatfs/ff.h"
#include "ST7066U_LCD.h"
#include "player.h"
#include "gui.h"

/* Global Vars */
static uint8_t vol = (STA_VOL_MAX-STA_VOL_MIN)/2;
static uint8_t GUI_Status = GUI_STATUS_STOPPED;
static char *cur_song;  // this keeps track of the current song playing
static File_t *cur_fp;  // this keeps track of the current song selected
static uint8_t btn_pressed = 0;
static uint16_t len, count = 1;
static uint8_t spin_index = 0, spin_str[4] = {0x2f, 0x2d, LCD_FONT_BSLASH_CODE, 0x7c};
static uint8_t vol_btn_pressed = 0 , vol_displayed = 0;

/* This is the debouncing delay */
#define BTN_DELAY   200
/* Play with this value to adjust the spinning speed */
#define SPIN_SPD    2
/* Play with these values to adjust the song name scrolling speed */
#define SCRL_SPD    5
#define EXT_DELAY   2


void GUI_init()
{
    // SW1, SW2 & SW3
    SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R1;
    SYSCTL->RCGCGPIO;

    GPIOB->DIR &= ~(1<<0 | 1<<1 | 1<<5);
    GPIOB->PUR |= 1<<0 | 1<<1 | 1<<5;
    GPIOB->IS &= ~(1<<0 | 1<<1 | 1<<5);
    GPIOB->IBE &= ~(1<<0 | 1<<1 | 1<<5);
    GPIOB->IEV &= ~(1<<0 | 1<<1 | 1<<5);
    GPIOB->DEN |= 1<<0 | 1<<1 | 1<<5;
    GPIOB->ICR |= 1<<0 | 1<<1 | 1<<5;
    GPIOB->IM |= 1<<0 | 1<<1 | 1<<5;
    NVIC_EnableIRQ(GPIOB_IRQn);

    // SW5 & SW6
    SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R4;
    SYSCTL->RCGCGPIO;

    GPIOE->DIR &= ~(1<<4 | 1<<5);
    GPIOE->PUR |= 1<<4 | 1<<5;
    GPIOE->IS &= ~(1<<4 | 1<<5);
    GPIOE->IBE &= ~(1<<4 | 1<<5);
    GPIOE->IEV &= ~(1<<4 | 1<<5);
    GPIOE->DEN |= 1<<4 | 1<<5;
    GPIOE->ICR |= 1<<4 | 1<<5;
    GPIOE->IM |= 1<<4 | 1<<5;
    NVIC_EnableIRQ(GPIOE_IRQn);

    // LCD_PWM (M1PWM5, pwm2B', PF1)
    SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R5;
    SYSCTL->RCGCPWM  |= SYSCTL_RCGCPWM_R1;

    GPIOF->AFSEL |= 1<<1;
    GPIOF->PCTL |= GPIO_PCTL_PF1_M1PWM5;
    GPIOF->DEN |= 1<<1;

    PWM1->_2_CTL = 0;
    PWM1->_2_GENB = 0x80C;
    PWM1->_2_LOAD = LCD_PWM_LOAD;
    PWM1->_2_CMPB = LCD_PWM_HIGH;  // 100% duty
    PWM1->_2_CTL |= 1<<0;
    PWM1->ENABLE |= 1<<5;

    // use Timer0 to debounce switches
    SYSCTL->RCGCTIMER |= SYSCTL_RCGCTIMER_R0;
    // set Timer0 to 32bit mode
    TIMER0->CFG = TIMER_CFG_32_BIT_TIMER;
    // one-shot mode
    TIMER0->TAMR |= TIMER_TAMR_TAMR_1_SHOT;
    // set timeout period in ms
    TIMER0->TAILR = SystemCoreClock / 1000 * BTN_DELAY;
    // fire timer once to timeout
    TIMER0->CTL |= TIMER_CTL_TAEN;

    // write the custom font to the LCD
    LCD_write_nstr_f(LCD_FONT_BSLASH, 8, LCD_FONT_BSLASH_ADDR);
}

uint16_t get_files(const TCHAR* path, File_t** first_fp)
{
    /**
        This function dynamically creates a doubly-linked list of only mp3 files
        on the heap until its read all of them or we run out of heap space
    */
    DIR dir;
    FILINFO fno;
    uint16_t fnlen, len = 0;
    char *fn;
    File_t *cur_fp, *prev_fp = NULL;
#if _USE_LFN
    static char lfn[_MAX_LFN + 1];   /* Buffer to store the LFN */
    fno.lfname = lfn;
    fno.lfsize = sizeof lfn;
#endif

    // set first to NULL for now
    *first_fp = NULL;

    if (f_opendir(&dir, path) != FR_OK)
        return 0;

    // while we can successfully read the dir and it's not the end
    while ( (f_readdir(&dir, &fno) == FR_OK) && (fno.fname[0] != '\0') )
    {

#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif

        fnlen = strlen(fn);

        // skip dir and non mp3 files
        if ( (fno.fattrib & AM_DIR)             ||
             (fnlen < 4)                        ||
             (strcmp(&fn[fnlen-4], ".mp3") != 0) )
            continue;

        // allocate space for size of File_t struct and file name (+1 for null term char)
        if ((cur_fp = (File_t*) malloc(sizeof(File_t)+fnlen+1)) == NULL)
            break;

        // record the file size in bytes
        cur_fp->fsize = fno.fsize;
        // record the len of song name (fnlen - 4)
        cur_fp->snlen = fnlen - 4;
        // point fname to the address right after Filt_t
        cur_fp->fname = (char*)cur_fp+sizeof(File_t);
        // copy the fname
        strcpy(cur_fp->fname, fn);
        // cur_fp is the end of the list
        cur_fp->next = NULL;
        // increase the len count
        ++len;

        // if the list is empty
        if (*first_fp == NULL)
        {
            *first_fp = cur_fp;
            (*first_fp)->prev = NULL;
        }
        // if there's only one item in the list
        else if (prev_fp == NULL)
        {
            (*first_fp)->next = prev_fp = cur_fp;
            prev_fp->prev = *first_fp;
        }
        else
        {
            prev_fp->next = cur_fp;
            cur_fp->prev = prev_fp;
            prev_fp = cur_fp;
        }
    }  // while (...)

    f_closedir(&dir);

    // return the head of the linked list
    return len;
}

void display_song()
{
    // we can keep track of time because this function is called periodically
    static uint8_t time = 0, extra = EXT_DELAY;
    static uint8_t index = 0;

    // clear the first row
    LCD_clear_1();

    // if song name is longer than screen width
    if (cur_fp->snlen > LCD_MAX_WIDTH-2)
    {
        // display from index up to LCD_MAX_WIDTH-2
        LCD_write_nstr(&(cur_fp->fname[index]), LCD_MAX_WIDTH-2, LCD_DDRAM_LINE1_ADDR);
        // if it's time to scroll
        if (time++ == SCRL_SPD)
        {
            // if we are in the front or the back
            if ( (extra) && ((index == 0) || (index + LCD_MAX_WIDTH-2 == cur_fp->snlen)) )
            {
                // extra delay, don't scroll yet
                --extra;
            }
            else
            {
                // increment index and wrap around check
                if ( (++index) + LCD_MAX_WIDTH-2 > cur_fp->snlen )
                    // wrap around
                    index = 0;
                // reset extra
                extra = EXT_DELAY;
            }

            // reset time count
            time = 0;
        }
    }
    else
    {
        // display the song name
        LCD_write_nstr(cur_fp->fname, cur_fp->snlen, LCD_DDRAM_LINE1_ADDR);
        // reset time count
        time = 0;
        // reset index
        index = 0;
    }
}

void display_stats()
{
    uint8_t cntlen;
    char buf[LCD_MAX_WIDTH+1];

    // display the song name here first to sync up the LCD update
    display_song();

    // clear the second row
    LCD_clear_2();

    // format the file size
    if (cur_fp->fsize >= 1024*1024)
        // display file size in MB
        snprintf(buf, LCD_MAX_WIDTH, "%.2f MB", cur_fp->fsize / (1024.0 * 1024.0));
    else
        // display file size in KB
        snprintf(buf, LCD_MAX_WIDTH, "%.2f KB", cur_fp->fsize / 1024.0);
    // display the file size
    LCD_write_nstr(buf, LCD_MAX_WIDTH, LCD_DDRAM_LINE2_ADDR);

    // right justify and display the song count
    cntlen = snprintf(buf, LCD_MAX_WIDTH, "%u/%u", count, len);
    LCD_write_nstr(buf, LCD_MAX_WIDTH, LCD_DDRAM_LINE2_ADDR+LCD_MAX_WIDTH-cntlen);
}

void display_volume(){
    // Assume vol is two digits
    char buf[2];   
    uint8_t i;    
    i = vol;

    buf[1] = i%10 + '0';
    buf[0] = i/10 + '0';

    // clear screen
    LCD_write(RS_ADDR, 0x01);
    if(i == 0){
       LCD_write_str("Mute",LCD_DDRAM_LINE1_ADDR+6);
    }else{
        LCD_write_nstr(buf,2,LCD_DDRAM_LINE1_ADDR+7);
    }
    LCD_write(RS_ADDR,LCD_DDRAM_LINE2_ADDR+3);
    while(i-- > 0) LCD_write(RS_DATA,255);    
}

void GPIOB_Handler()
{
    // check if the delay has passed
    if (TIMER0->RIS & TIMER_RIS_TATORIS)
    {
        // assume a button was pressed
        btn_pressed = 1;

        if (GPIOB->MIS & (1<<0))
        {
            /* Vol+ (SW2) */
            if (vol < STA_VOL_MAX)
                sta_set_vol(++vol);
            vol_btn_pressed = 1;
            // clear int
            GPIOB->ICR |= 1<<0;
        }
        else if (GPIOB->MIS & (1<<5))
        {
            /* VOl- (SW1) */
            if (vol > STA_VOL_MIN)
                sta_set_vol(--vol);
            vol_btn_pressed = 1;
            // clear int
            GPIOB->ICR |= 1<<5;
        }
        else if (GPIOB->MIS & (1<<1))
        {
            /* Next (SW3) */
            if (cur_fp->next != NULL)
            {
                ++count;
                cur_fp = cur_fp->next;
                display_stats();

                if (GUI_Status == GUI_STATUS_PLAYING)
                {
                    osSignalSet(PlayerThreadId, PLAYER_SIG_SKIP);
                    PLAYER_PLAY(cur_fp->fname);
                    cur_song = cur_fp->fname;
                }
            }

            // clear int
            GPIOB->ICR |= 1<<1;
        }
        // no buttons were pressed therefore...
        else
        {
            btn_pressed = 0;
        }

        // reset the timer
        TIMER0->ICR |= TIMER_ICR_TATOCINT;
        TIMER0->CTL |= TIMER_CTL_TAEN;
    }
    else
    {
        // no timeout yet so we reset all int
        GPIOB->ICR |= 1<<0 | 1<<1 | 1<<5;
    }
}

void GPIOE_Handler()
{

    // check if the delay has passed
    if (TIMER0->RIS & TIMER_RIS_TATORIS)
    {
        // assume a button was pressed
        btn_pressed = 1;

        if (GPIOE->MIS & (1<<4))
        {
            /* Play/Pause (SW4) */
            if (GUI_Status == GUI_STATUS_STOPPED)
            {
                // if the player is waiting for a song
                if (Player_State == PLAYER_STATE_WAITING)
                {
                    PLAYER_PLAY(cur_fp->fname);
                    cur_song = cur_fp->fname;
                }
                // if a new song is selected
                else if (cur_song != cur_fp->fname)
                {
                    // skip the current song and queue the next song
                    osSignalSet(PlayerThreadId, PLAYER_SIG_SKIP);
                    PLAYER_PLAY(cur_fp->fname);
                    cur_song = cur_fp->fname;
                }
                // otherwise resume playing
                sta_play();
                GUI_Status = GUI_STATUS_PLAYING;
            }
            else if (GUI_Status == GUI_STATUS_PLAYING)
            {
                sta_stop();
                GUI_Status = GUI_STATUS_STOPPED;
            }
        }
        else if (GPIOE->MIS & (1<<5))
        {
            /* Prev (SW5) */
            if (cur_fp->prev != NULL)
            {
                --count;
                cur_fp = cur_fp->prev;
                display_stats();

                if (GUI_Status == GUI_STATUS_PLAYING)
                {
                    osSignalSet(PlayerThreadId, PLAYER_SIG_SKIP);
                    PLAYER_PLAY(cur_fp->fname);
                    cur_song = cur_fp->fname;
                }
            }

            // clear int
            GPIOE->ICR |= 1<<5;
        }
        // no buttons were pressed therefore...
        else
        {
            btn_pressed = 0;
        }

        // reset the timer
        TIMER0->ICR |= TIMER_ICR_TATOCINT;
        TIMER0->CTL |= TIMER_CTL_TAEN;
    }

    // clear int
    GPIOE->ICR |= 1<<4 | 1<<5;
}

/* shouldn't be too small... */
#define GUI_DELAY   100
/* how many multiples of GUI_DELAY before LCD timeout */
#define DELAY_MUL   50      // timeout is about GUI_DELAY * DELAY_MUL in milliseconds
#define VOL_DELAY   10
void GUI_Thread (void const *argument)
{
    /**
        This GUI makes the assumes that, during operation, the SD card will not be removed and
        the contects of the SD card will not be changed. It also assumes that all of mp3 files
        are on the top dir and there's subdirs are not supported.
    */

    osEvent sig;
    uint32_t spin_delay = 0, lcd_timeout = 0;

    // get the list of mp3s
    len = get_files("", &cur_fp);

    // see if there's at least one song
    if (cur_fp != NULL)
    {
        // if so, display the song
        display_stats();
    }
    else
    {
        // otherwise display an error msg and quit
        LCD_write_str("<No Songs Found>", LCD_DDRAM_LINE1_ADDR);
        LCD_write_str("MP3 Halted :(   ", LCD_DDRAM_LINE2_ADDR);
        return;
    }

    while(1)
    {
        // check if the player is done playing
        sig = osSignalWait(GUI_SIG_NEXT, 0);
        if ( (sig.status == osEventSignal) && (sig.value.signals & GUI_SIG_NEXT) )
        {
            if (cur_fp->next != NULL)
            {
                ++count;
                cur_fp = cur_fp->next;
                display_stats();

                if (GUI_Status == GUI_STATUS_PLAYING)
                {
                    PLAYER_PLAY(cur_fp->fname);
                    cur_song = cur_fp->fname;
                }
            }
            else
            {
                sta_stop();
                GUI_Status = GUI_STATUS_STOPPED;
            }

            // emulate a button press so the screen would light up
            btn_pressed = 1;
        }

        /* light up the screen if any of the buttons were pressed */
        if (btn_pressed)
        {
            // light up the screen
            PWM1->_2_CMPB = LCD_PWM_HIGH;
            // reset timeout
            lcd_timeout = 0;
            btn_pressed = 0;
            if(vol_btn_pressed){
                display_volume(); 
                vol_displayed = 1;  
                vol_btn_pressed = 0;
            }else{
                display_stats();
                vol_displayed = 0;
            }
        }
        // assume lcd_timeout is longer than vol_timeout
        else if (lcd_timeout == VOL_DELAY)
        {
            // switch to song screen
            // Also default display to song
            if(vol_displayed){
                display_stats();
                vol_displayed = 0;
            }
        }
        else if (lcd_timeout == DELAY_MUL)
        {
            // timeout, so we dimm the screen
            PWM1->_2_CMPB = LCD_PWM_LOW;            
        }
        ++lcd_timeout;  // will take a while before uint32_t rolls over

        /* spin animation */
        if (GUI_Status == GUI_STATUS_PLAYING)
        {
            if (spin_delay == SPIN_SPD)
            {
                spin_index = (spin_index + 1) % 4;
                spin_delay = 0;
            }
            else
            {
                ++spin_delay;
            }
        }
        if(!vol_displayed){
            display_song();
            LCD_write_nstr_f((char*)&spin_str[spin_index], 1, LCD_DDRAM_LINE1_ADDR+LCD_MAX_WIDTH-1);
        }

        osDelay(GUI_DELAY);
    }
}
