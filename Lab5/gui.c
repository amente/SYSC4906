#include "CU_TM4C123.h"                 // CU_SYSC4906::Device:Startup
#include "osObjects.h"

#include <string.h>
#include <stdlib.h>
#include "fatfs/ff.h"
#include "ST7066U_LCD.h"
#include "player.h"
#include "gui.h"

static uint8_t vol = (STA_VOL_MAX-STA_VOL_MIN)/2;

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
    GPIOB->ICR |= 1<<0 | 1<<1 | 1<<5;  // clear the interrupt since RIS will be used
    
    // SW5 & SW6
    SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R4;
    SYSCTL->RCGCGPIO;
    
    GPIOE->DIR &= ~(1<<4 | 1<<5);
    GPIOE->PUR |= 1<<4 | 1<<5;
    GPIOE->IS &= ~(1<<4 | 1<<5);
    GPIOE->IBE &= ~(1<<4 | 1<<5);
    GPIOE->IEV &= ~(1<<4 | 1<<5);
    GPIOE->DEN |= 1<<4 | 1<<5;
    GPIOB->ICR |= 1<<4 | 1<<5;
    
    // LCD_PWM
    SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R5;
    SYSCTL->RCGCGPIO;

    GPIOF->DIR |= 1<<1;
    GPIOF->DEN |= 1<<1;
    GPIOF->DATA |= 1<<1;
}

typedef __packed struct File
{
    struct File *next;
    struct File *prev;
    char *fname;
} File_t;

File_t* get_files(const TCHAR* path)
{
    /**
        This function dynamically creates a doubly-linked list of only mp3 files
        on the heap until its read all of them or we run out of heap space
    */
    DIR dir;
    FILINFO fno;
    uint16_t len;
    char *fn;
    File_t *cur_fp, *prev_fp = NULL, *first_fp = NULL;
#if _USE_LFN
    static char lfn[_MAX_LFN + 1];   /* Buffer to store the LFN */
    fno.lfname = lfn;
    fno.lfsize = sizeof lfn;
#endif    

    if (f_opendir(&dir, path) != FR_OK)
        return NULL;
    
    // while we can successfully read the dir and it's not the end
    while ( (f_readdir(&dir, &fno) == FR_OK) && (fno.fname[0] != '\0') )
    {

#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif

        len = strlen(fn);

        // skip dir and non mp3 files
        if ( (fno.fattrib & AM_DIR) ||
             (len < 5)              ||
             (strcmp(&fn[len-4], ".mp3")) )
            continue;
        
        // allocate space for size of File_t struct and file name
        if ((cur_fp = (File_t*) malloc(sizeof(File_t)+len)) == NULL)
            break;

        // point fname to the address right after Filt_t
        cur_fp->fname = (char*)cur_fp+sizeof(File_t);
        // copy the fname
        strcpy(cur_fp->fname, fn);
        // cur_fp is the end of the list
        cur_fp->next = NULL;

        // if the list is empty
        if (first_fp == NULL)
        {
            first_fp = cur_fp;
            first_fp->prev = NULL;
        }
        // if there's only one item in the list
        else if (prev_fp == NULL)
        {
            first_fp->next = prev_fp = cur_fp;
            prev_fp->prev = first_fp;
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
    return first_fp;
}
    
/* this delay gives the Player thread time to run and debounces the switches */
#define GUI_DELAY 500
    
void GUI_Thread (void const *argument)
{
    /**
        This GUI makes the assumes that, during operation, the SD card will not be removed and
        the contects of the SD card will not be changed. It also assumes that all of mp3 files
        are on the top dir and there's subdirs are not supported.
    */

    static uint8_t status = GUI_STATUS_STOPPED;
    static char *cur_song;

    File_t *cur_fp;
    osEvent sig;

    // get the list of mp3s
    cur_fp = get_files("");

    LCD_clear_1();
    // see if there's at least one song
    if (cur_fp != NULL)
    {
        // if so, display the song
        LCD_write_str(cur_fp->fname, LCD_DDRAM_LINE1_ADDR);
    }
    else
    {
        // otherwise display an error msg and quit
        LCD_write_str("<No Songs Found>", LCD_DDRAM_LINE1_ADDR);
        return;
    }

    while(1)
    {
        /** 
            below are the code to handle button presses. 
            the buttons are polled because it greatly simplifies code dev
            (interrupts and RTOSes can get a bit complicated)
            and the real time requirements are very lenient.
        */
            
        if (GPIOB->RIS & (1<<0))
        {
            /* Vol+ (SW2) */
            if (vol < STA_VOL_MAX)
                sta_set_vol(++vol);
            
            // clear int
            GPIOB->ICR |= 1<<0;
        }
        else if (GPIOB->RIS & (1<<5))
        {
            /* VOl- (SW1) */
            if (vol > STA_VOL_MIN)
                sta_set_vol(--vol);
            
            // clear int
            GPIOB->ICR |= 1<<5;
        }    
        else if (GPIOB->RIS & (1<<1))
        {
            /* Next (SW3) */
            if (cur_fp->next != NULL)
            {
                cur_fp = cur_fp->next;
                LCD_clear_1();
                LCD_write_str(cur_fp->fname, LCD_DDRAM_LINE1_ADDR);

                if (status == GUI_STATUS_PLAYING)
                {
                    osSignalSet(PlayerThreadId, PLAYER_SIG_SKIP);
                    PLAYER_PLAY(cur_fp->fname);
                    cur_song = cur_fp->fname;
                }
            }
            
            // clear int
            GPIOB->ICR |= 1<<1;
        }
        else if (GPIOE->RIS & (1<<4))
        {
            /* Play/Pause (SW4) */
            if (status == GUI_STATUS_STOPPED)
            {
                // if no songs have played
                if (cur_song == NULL)
                {
                    // don't skip
                    PLAYER_PLAY(cur_fp->fname);
                    cur_song = cur_fp->fname;
                }
                // if the song had changed
                else if (cur_song != cur_fp->fname)
                {
                    osSignalSet(PlayerThreadId, PLAYER_SIG_SKIP);
                    PLAYER_PLAY(cur_fp->fname);
                    cur_song = cur_fp->fname;
                }
                sta_play();
                status = GUI_STATUS_PLAYING;
            }
            else if (status == GUI_STATUS_PLAYING)
            {
                sta_stop();
                status = GUI_STATUS_STOPPED;
            }

            // clear int
            GPIOE->ICR |= 1<<4;
        }
        else if (GPIOE->RIS & (1<<5))
        {
            /* Prev (SW5) */
            if (cur_fp->prev != NULL)
            {
                cur_fp = cur_fp->prev;
                LCD_clear_1();
                LCD_write_str(cur_fp->fname, LCD_DDRAM_LINE1_ADDR);

                if (status == GUI_STATUS_PLAYING)
                {
                    osSignalSet(PlayerThreadId, PLAYER_SIG_SKIP);
                    PLAYER_PLAY(cur_fp->fname);
                    cur_song = cur_fp->fname;
                }
            }
            
            // clear int
            GPIOE->ICR |= 1<<5;
        }

        // check if the player is done playing
        sig = osSignalWait(GUI_SIG_NEXT, 0);
        if ( (sig.status == osEventSignal) && (sig.value.signals & GUI_SIG_NEXT) )
        {
            if (cur_fp->next != NULL)
            {
                cur_fp = cur_fp->next;
                LCD_clear_1();
                LCD_write_str(cur_fp->fname, LCD_DDRAM_LINE1_ADDR);

                if (status == GUI_STATUS_PLAYING)
                {
                    PLAYER_PLAY(cur_fp->fname);
                    cur_song = cur_fp->fname;
                }
            }
            else
            {
                sta_stop();
                status = GUI_STATUS_STOPPED;
            }
        }

        osDelay(GUI_DELAY);
    }
}
