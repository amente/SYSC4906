#ifndef __STA013_H
#define __STA013_H

#include <stdint.h>
#include "i2c.h"

#define STA_SSI                 SSI0
#define STA_I2C                 I2C1

#define STA_ADDR                0x43

#define STA_IDENT_VAL           0xAC

typedef uint8_t STA_VOL;
#define STA_VOL_MIN             0
#define STA_VOL_MAX             10

#define STA_REG_IDENT           0x01
#define STA_REG_PLAY            0x13
#define STA_REG_MUTE            0x14
#define STA_REG_DLA             0x46
#define STA_REG_DRA             0x48
#define STA_REG_RUN             0x72

// as oppose to !(x)
#define DATA_REQ_POL(x)         (x)
#define DATA_REQ_INT_EN()       GPIOA->IM |= 1<<4
#define DATA_REQ_INT_DIS()      GPIOA->IM &= ~(1<<4); STA_SSI->CR1 |= SSI_CR1_SSE

int sta_init(void);
void sta_set_vol(STA_VOL vol);

#define sta_play()              i2c_WriteRegister(STA_ADDR, STA_REG_PLAY, 0x01)
#define sta_stop()              i2c_WriteRegister(STA_ADDR, STA_REG_PLAY, 0x00)
#define sta_mute()              i2c_WriteRegister(STA_ADDR, STA_REG_MUTE, 0x01)
#define sta_unmute()            i2c_WriteRegister(STA_ADDR, STA_REG_MUTE, 0x00)

#endif
