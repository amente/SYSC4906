#include "CU_TM4C123.h"                     // CU_SYSC4906::Device:Startup
#include "osObjects.h"

#include "fatfs/ff.h"
#include "sta013.h"
#include "player.h"
#include "gui.h"
#include "spi.h"

// this should be multiples of 512 due to SD card block sizes
static uint8_t block[512];
static UINT nbytes;

void Player_Thread (void const *argument)
{
    uint16_t i;
    FIL fil;
    osEvent msg, sig;

    /* assume the STA013 is already init'd */
    while(1)
    {
        // wait for a path to a file to play
        msg = osMessageGet(PlayerMsgQId, osWaitForever);
        
        // try to open the mp3
        if ( (msg.value.p == NULL) || (f_open(&fil, msg.value.p, FA_READ) != FR_OK) )
            // fail silently
            continue;
        
        // start playing the file
        do
        {
            f_read(&fil, block, sizeof(block), &nbytes);
            for(i=0; i<nbytes; i++)
            {
                // check for the stop or pause signal
                sig = osSignalWait(0, 0);
                if (sig.status == osEventSignal)
                {
                    // check for the skip signal
                    if (sig.value.signals & PLAYER_SIG_SKIP)
                    {
                        // stop playing by using a goto to break out of 2 loops
                        // Beware: http://xkcd.com/292/
                        goto skip;
                    }
                }
                
                /* save a bit of overhead by dealing with SSI directly */
                // wait for Tx FIFO to not be full
                while( !( (SSI0->SR & SSI_SR_TNF) && DATA_REQ_POL(GPIOA->DATA & (1<<4)) ) )
                    ;
                // send out one byte
                STA_SSI->DR = block[i];
            }
        } while(nbytes > 0);
        
        skip:
        // done playing so we close the file
        f_close(&fil);
    }   
}
