#include "CU_TM4C123.h"                 // CU_SYSC4906::Device:Startup

#include "dma.h"

// here is the dma control table

#if DMA_ALT_TABLE == 1
__align(1024) DMA_CTRL_ENT dma_table[64];
#else
__align(1024) DMA_CTRL_ENT dma_table[32];
#endif
