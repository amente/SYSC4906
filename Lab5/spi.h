#ifndef __SPI_H
#define __SPI_H

#include "CU_TM4C123.h"                 // CU_SYSC4906::Device:Startup
#include <stdint.h>

#define SPI_CLK_SLOW        400000   // 400 kHz
#define SPI_CLK_FAST        8000000  // 8 MHz
#define DUMMY_DATA          0xFF


/*            CPOL    CPHA
SPI_MODE_0     0       0
SPI_MODE_1 	   0	   1
SPI_MODE_2	   1	   0
SPI_MODE_3	   1	   1 
*/
#define SPI_MODE_0          (0)
#define SPI_MODE_1          (SSI_CR0_SPH)
#define SPI_MODE_2          (SSI_CR0_SPO)
#define SPI_MODE_3          (SSI_CR0_SPH | SSI_CR0_SPO)
typedef uint8_t             SPI_MODE;

#define SS1_RX_DMA_CH       (1<<24)
#define SS1_TX_DMA_CH       (1<<25)

void spi_set_clk(SSI0_Type *ssi, uint32_t ssi_clk);
int spi_init(SSI0_Type *ssi, uint32_t ssi_clk, SPI_MODE mode);
void spi_fss_low(SSI0_Type *ssi);
void spi_fss_high(SSI0_Type *ssi);
void spi_fss_sw(SSI0_Type *ssi);
void spi_fss_hw(SSI0_Type *ssi);
int spi_send(SSI0_Type *ssi, void *data, uint16_t len);
int spi_recv(SSI0_Type *ssi, void *data, uint16_t len);

#endif
