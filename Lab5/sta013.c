#include "CU_TM4C123.h"                 // CU_SYSC4906::Device:Startup

#include <stdio.h>
#include "spi.h"
#include "i2c.h"
#include "sta013.h"
#include "player.h"

extern const unsigned char sta_config_file[];
extern const unsigned int sta_config_file_len;

int sta_init()
{
    /**
        STA013 is connected to SS0(PA2-5), I2C1(PA6-PA7) and DATA_REQ on PA4
        Since SS0 also uses PA4 as MISO, GPIO init should be done after SPI init
    */
    
    uint16_t i;
    
    // init SSI0 module
    spi_init(SSI0, SPI_CLK_FAST, SPI_MODE_1);
    
    // init GPIO for DATA_REQ after SSI because pin conflict on PA4
    GPIOA->DEN &= ~(1<<3);      // disable SS0Fss(PA3) due to a bug on the PCB
    GPIOA->DIR &= ~(1<<4);
    GPIOA->PUR &= ~(1<<4);
    GPIOA->AFSEL &= ~(1<<4);    // undo settings from SSI0
    GPIOA->DEN |= 1<<4;
        
    // init I2C1 module
    i2c_init();

    // PHY check
    if (i2c_ReadRegister(STA_ADDR, STA_REG_IDENT) != STA_IDENT_VAL)
        return -1;

    /**
        Here we upload the modded configuration file from PJRC
        It includes the config file from STM plus value pairs to config the PLL
        http://www.pjrc.com/mp3/sta013.html
    */
    for(i = 0; i < sta_config_file_len; i+=2)
    {
        if (i2c_WriteRegister(STA_ADDR, sta_config_file[i], sta_config_file[i+1]) == -1)
            return -1;
    }
    
    // set STA013 to a safe volume
    sta_set_vol( (STA_VOL_MAX-STA_VOL_MIN)/2 );
    
    // start running
    i2c_WriteRegister(STA_ADDR, STA_REG_RUN, 0x01);
    
    return 0;
}

void sta_set_vol(STA_VOL vol)
{
    // if we see a zero
    if (vol == STA_VOL_MIN)
    {
        // mute the sound
        i2c_WriteRegister(STA_ADDR, STA_REG_DLA, 0xFF);
        i2c_WriteRegister(STA_ADDR, STA_REG_DRA, 0xFF);
    }
    else if (vol <= STA_VOL_MAX)
    {
        // convert a value from 1-10 to -40db-0db
        vol = 4 * (STA_VOL_MAX - vol);
        // set the left channel attnuation in db
        i2c_WriteRegister(STA_ADDR, STA_REG_DLA, vol);
        // set the right channel attnuation in db
        i2c_WriteRegister(STA_ADDR, STA_REG_DRA, vol);
    }
}

    
