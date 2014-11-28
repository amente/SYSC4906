#include "CU_TM4C123.h"                 // CU_SYSC4906::Device:Startup
#include <stdint.h>

#ifndef __DMA_H
#define __DMA_H

// we don't need the alt control table for now
#define DMA_ALT_TABLE  0

#define DSTINC(x)      ((unsigned)(x)<<30)
#define DSTSIZE(x)     ((unsigned)(x)<<28)
#define SRCINC(x)      ((unsigned)(x)<<26)
#define SRCSIZE(x)     ((unsigned)(x)<<24)
#define ARBSIZE(x)     ((unsigned)(x)<<14)
#define XFERSIZE(x)    ((unsigned)(x)<<4)
#define NXTUSEBURST(x) ((unsigned)(x)<<3)
#define XFERMODE(x)    ((unsigned)(x)<<0)
    
#define DSTINC_M       0xC0000000
#define DSTSIZE_M      0x30000000
#define SRCINC_M       0x0C000000
#define SRCSIZE_M      0x03000000
#define ARBSIZE_M      0x0003C000
#define XFERSIZE_M     0x00033FF0
#define NXTUSEBURST_M  0x00000008
#define XFERMODE_M     0x00000007
    
typedef __packed struct
{
    void *src_endp;     // Last address (inclusive)!
    void *dst_endp;     // Last address (inclusive)!
    volatile uint32_t ctrl;
    const uint32_t rsv;    
} DMA_CTRL_ENT;

/* 32 DMA channels */
extern DMA_CTRL_ENT dma_table[32];

#endif
