#include "CU_TM4C123.h"                     // CU_SYSC4906::Device:Startup

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions

#include <stdio.h>
#include <string.h>
#include "fatfs/ff.h"

#include "ST7066U_LCD.h"
#include "mci.h"
#include "uart.h"
#include "spi.h"
#include "i2c.h"
#include "sta013.h"
#include "player.h"
#include "gui.h"

/*  Global Resources  */
FATFS FatFs;
osMessageQId PlayerMsgQId;
osThreadId  PlayerThreadId;
osThreadId  GUIThreadId;

#define USE_LCD 1

int main_init()
{
#if USE_LCD
    // init LCD
    LCD_init();
    LCD_write_str("Initializing...", LCD_DDRAM_LINE1_ADDR);
#endif
    
    // init UART0
    if (uart_init(UART0, 9600) == -1)
        return -1;
    
    // init STA013
    if (sta_init() == -1)
        return -1;

    // init GUI
    GUI_init();
    
    // mount the sd card
    if (f_mount(&FatFs, "", 1) != FR_OK)
        return -1;
    
    return 0;
}

int main (void)
{
    // initialize CMSIS-RTOS
    osKernelInitialize ();
    
    // init main
    if (main_init() != 0)
    {
#if USE_LCD
        LCD_write_str("Init Failed!   ", LCD_DDRAM_LINE1_ADDR);
        LCD_write_str("MP3 Halted :(", LCD_DDRAM_LINE2_ADDR);
#endif
        return -1;
    }

    // create the player msg queue
    PlayerMsgQId = osMessageCreate(osMessageQ(PlayerMsgQ), NULL);
    
    // create Player thread
    PlayerThreadId = osThreadCreate (osThread(Player_Thread), NULL);

    // create GUI thread
    GUIThreadId = osThreadCreate (osThread(GUI_Thread), NULL);

    // remove the main thread because it's not needed anymore
    osThreadTerminate(osThreadGetId ());
    
    return 0;
}
