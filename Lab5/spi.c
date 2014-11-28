#include "CU_TM4C123.h"                 // CU_SYSC4906::Device:Startup
#ifdef __RTX
#include <cmsis_os.h>
#endif
#include "stddef.h"
#include "spi.h"
#include "dma.h"

/* NON-DMA OPERATIONS SEEMS TO BE BROKEN AT HIGHER SPEEDS SO USE DMA FOR NOW! */

/* only support SSI1 for now */
#define USE_DMA 1

void spi_set_clk(SSI0_Type *ssi, uint32_t ssi_clk)
{
    // calculate the clock prescale with SCR = 0
    ssi->CPSR = SystemCoreClock / ssi_clk;
}

int spi_init(SSI0_Type *ssi, uint32_t ssi_clk, SPI_MODE mode)
{
    if (ssi == SSI0)
    {
        /*  
            SSI0
            PA2 = SS0Clk
            PA3 = SS0Fss
            PA4 = SS0Rx
            PA5 = SS0Tx
        */
        
        // enable port A
        SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R0;
        // enable SSI0
        SYSCTL->RCGCSSI |= SYSCTL_RCGCSSI_R0;
        
        // enable AF for PA2-5
        GPIOA->AFSEL |= (0xF<<2);
        // configure pin ctrl
        GPIOA->PCTL |=  GPIO_PCTL_PA2_SSI0CLK | 
                        GPIO_PCTL_PA3_SSI0FSS |
                        GPIO_PCTL_PA4_SSI0RX  |
                        GPIO_PCTL_PA5_SSI0TX;
        // enable pull-ups on all 4 pins
        GPIOA->PUR |= 0xF<<2;
        // all GPIO outputs except MISO(PA4)
        GPIOA->DIR |= (0xF<<2) & ~(1<<4);
        // enable the GPIO pad
        GPIOA->DEN |= 0xF<<2;
        // set all pins to high on GPIO
        GPIOA->DATA |= 0xF<<2;
    }
    else if (ssi == SSI1)
    {
        /*  
            SSI1
            PD0 = SS1Clk
            PD1 = SS1Fss
            PD2 = SS1Rx
            PD4 = SS1Tx
        */
    
        // enable port D
        SYSCTL->RCGCGPIO |= SYSCTL_RCGCGPIO_R3;
        // enable SSI1
        SYSCTL->RCGCSSI |= SYSCTL_RCGCSSI_R1;
        
        // enable AF for PD0-3
        GPIOD->AFSEL |= (0xF<<0);
        // configure pin ctrl
        GPIOD->PCTL |=  GPIO_PCTL_PD0_SSI1CLK | 
                        GPIO_PCTL_PD1_SSI1FSS |
                        GPIO_PCTL_PD2_SSI1RX  |
                        GPIO_PCTL_PD3_SSI1TX;
        // enable pull-ups on all 4 pins
        GPIOD->PUR |= 0xF<<0;
        // all GPIO outputs except MISO(PD2)
        GPIOD->DIR |= (0xF<<0) & ~(1<<2);
        // enable the GPIO pad
        GPIOD->DEN |= 0xF<<0;
        // set all pins to high on GPIO
        GPIOD->DATA |= 0xF<<0;
        
#if USE_DMA
        /* configure for peripheral to memory transfer */
        
        // enable DMA in SSI1
        ssi->DMACTL |= SSI_DMACTL_RXDMAE | SSI_DMACTL_TXDMAE;
        
        // enable clock to UDMA
        SYSCTL->RCGCDMA |= SYSCTL_RCGCDMA_R0;
        
        // enable the DMA
        UDMA->CFG |= UDMA_CFG_MASTEN;
        // write the base address of the control table
        UDMA->CTLBASE = (uint32_t) &dma_table;
        
        /* SSI1 RX is on Ch 24 Enc. 0 */
        
        // set default priority
        UDMA->PRIOCLR |= SS1_RX_DMA_CH;
        // only use the primary table
        UDMA->ALTCLR |= SS1_RX_DMA_CH;
        // allow single or burst requests
        UDMA->USEBURSTCLR |= SS1_RX_DMA_CH;
        // allow requests to come in
        UDMA->REQMASKCLR |= SS1_RX_DMA_CH;

        /* SSI1 TX is on Ch 25 Enc. 0 */
        
        // set default priority
        UDMA->PRIOCLR |= SS1_TX_DMA_CH;
#if DMA_ALT_TABLE == 1
        // enable the alt control table
        UDMA->ALTSET |= SS1_TX_DMA_CH;
#else
        // only use the primary table
        UDMA->ALTCLR |= SS1_TX_DMA_CH;
#endif
        // allow single or burst requests
        UDMA->USEBURSTCLR |= SS1_TX_DMA_CH;
        // allow requests to come in
        UDMA->REQMASKCLR |= SS1_TX_DMA_CH;

        /* some channel control words */
        
        // SSI0 RX (Ch 24)
        dma_table[24].src_endp  = (void*)&ssi->DR;  // set the dst to the SSI FIFO
        dma_table[24].ctrl = DSTSIZE(0)  |      // 8 bit word
                             SRCINC(3)   |      // don't inc src ptr
                             SRCSIZE(0)  |      // 8 bit word
                             ARBSIZE(2)  ;      // 4 tfr per arbitration because FIFO is 8 word deeps
                        
        // SSI0 TX (Ch 25)
        dma_table[25].dst_endp  = (void*)&ssi->DR;  // set the dst to the SSI FIFO
        dma_table[25].ctrl = DSTINC(3)   |      // don't inc dst ptr
                             DSTSIZE(0)  |      // 8 bit word
                             SRCSIZE(0)  |      // 8 bit word
                             ARBSIZE(2)  ;      // 4 tfr per arbitration
        
        /* the rest to be configured in spi_send or spi_recv */
#endif  // if USE_DMA
    }
    else
    {
        // not supported yet
        return -1;
    }
    
    /* common settings */
    
    // clear the SSE bit
    ssi->CR1 &= ~SSI_CR1_SSE;
    // select master mode
    ssi->CR1 &= ~SSI_CR1_MS;
    // configure clock source to master clock
    ssi->CC &= ~SSI_CC_CS_M;
    // set SCR to 0
    ssi->CR0 &= ~SSI_CR0_SCR_M;
    
    // set SPI clock speed to slow at first
    spi_set_clk(ssi, ssi_clk);
    // configure SSI to SPI, and DSS=8
    ssi->CR0 |= SSI_CR0_DSS_8 | SSI_CR0_FRF_MOTO | mode;
    // enable SSI
    ssi->CR1 |= SSI_CR1_SSE;
    
    return 0;
}

int spi_send(SSI0_Type *ssi, void *data, uint16_t len)
{
#if USE_DMA
    uint8_t dummy = DUMMY_DATA, sink;
#else
    uint16_t rx, tx;
#endif
    
    if (!(ssi->CR1 & SSI_CR1_SSE))
        return -1;
    
    // spi_fss_low(ssi);
#if USE_DMA    
    if (ssi == SSI1)
    {
        /* set up the DMA transfer */
        // SSI0 RX (Ch 10)
        dma_table[24].dst_endp  = (void*)&sink;
        dma_table[24].ctrl      = (dma_table[24].ctrl & ~DSTINC_M) | DSTINC(3);
        // SSI0 TX (Ch 11)
        if (data == NULL)
        {
            dma_table[25].src_endp  = (void*)&dummy;
            dma_table[25].ctrl      = (dma_table[25].ctrl & ~SRCINC_M) | SRCINC(3);
            
        }
        else
        {
            dma_table[25].src_endp  = &(((uint8_t*)data)[len-1]);
            dma_table[25].ctrl      = (dma_table[25].ctrl & ~SRCINC_M) | SRCINC(0);
        }
        // set transfer size & basic transfer mode
        dma_table[24].ctrl |= XFERSIZE((len-1) & 0x3FF) | XFERMODE(1);
        dma_table[25].ctrl |= XFERSIZE((len-1) & 0x3FF) | XFERMODE(1);

        // start the transfer
        UDMA->ENASET |= SS1_RX_DMA_CH | SS1_TX_DMA_CH;
        // wait for it to finish
        while( (UDMA->ENASET & SS1_RX_DMA_CH) || (UDMA->ENASET & SS1_TX_DMA_CH) )
        {
        #ifdef __RTX
            osThreadYield();
        #endif
            ;
        }
    }
#else
    {
        rx = tx = 0;    
        /* use RR and assume both FIFOs are empty */
        //while (rx < len || tx < len)
        while(tx < len)
        {
            // if Tx FIFO not full
            if ( (tx < len) && (ssi->SR & SSI_SR_TNF) )
            {
                // put byte in Tx FIFO
                if (data != NULL)
                    ssi->DR = ((uint8_t*)data)[tx];
                else
                    ssi->DR = DUMMY_DATA;
                tx++;
            }
/*
            // if Rx FIFO not empty
            if ( (rx < len) && (ssi->SR & SSI_SR_RNE) )
            {
                // remove a byte from Rx FIFO
                ssi->DR;
                rx++;
            }*/
        }
    }
#endif  //if USE_DMA    
    // spi_fss_high(ssi);
    return 0;
}

int spi_recv(SSI0_Type *ssi, void *data, uint16_t len)
{
#if USE_DMA
    uint8_t dummy = DUMMY_DATA, sink;
#else
    uint16_t rx, tx;
#endif
    
    if ( !(ssi->CR1 & SSI_CR1_SSE) && (data != NULL) )
        return -1;

    // spi_fss_low(ssi);    
#if USE_DMA
    if (ssi == SSI1)
    {
        /* set up the DMA transfer */
        // SSI0 RX (Ch 10)
        if (data == NULL)
        {
            dma_table[24].dst_endp  = (void*)&sink;
            dma_table[24].ctrl      = (dma_table[24].ctrl & ~DSTINC_M) | DSTINC(3);
        }
        else
        {
            dma_table[24].dst_endp  = &(((uint8_t*)data)[len-1]);
            dma_table[24].ctrl      = (dma_table[24].ctrl & ~DSTINC_M) | DSTINC(0);
        }
        // SSI0 TX (Ch 11)
        dma_table[25].src_endp  = (void*)&dummy;
        dma_table[25].ctrl      = (dma_table[25].ctrl & ~SRCINC_M) | SRCINC(3);
        // set transfer size & basic transfer mode
        dma_table[24].ctrl |= XFERSIZE((len-1) & 0x3FF) | XFERMODE(1);
        dma_table[25].ctrl |= XFERSIZE((len-1) & 0x3FF) | XFERMODE(1);
        // start the transfer
        UDMA->ENASET |= SS1_RX_DMA_CH | SS1_TX_DMA_CH;
        // wait for it to finish
        while( (UDMA->ENASET & SS1_RX_DMA_CH) || (UDMA->ENASET & SS1_TX_DMA_CH) )
        #ifdef __RTX
            osThreadYield()
        #endif
            ;
    }
#else
    {
        rx = tx = 0;
        /*  use RR and assume both FIFOs are empty */
        while (rx < len || tx < len)
        {
            // if Tx FIFO not full
            if ( (tx < len) && (ssi->SR & SSI_SR_TNF) )
            {
                // put byte in Tx FIFO
                ssi->DR = DUMMY_DATA;
                tx++;
            }

            // if Rx FIFO not empty
            if ( (rx < len) && (ssi->SR & SSI_SR_RNE) )
            {
                // get a byte from Rx FIFO
                if (data != NULL)
                    ((uint8_t*)data)[rx] = ssi->DR;
                else
                    ssi->DR;
                rx++;
            }
        }
    }
#endif  //if USE_DMA
    // spi_fss_high(ssi);  
    return 0;
}
