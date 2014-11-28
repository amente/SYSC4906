#include "CU_TM4C123.h"                 // CU_SYSC4906::Device:Startup
#include <stddef.h>

#include "fatfs/diskio.h"
#include "spi.h"
#include "mci.h"

/*  This file works with the Tiva C dev board and assumes an SD card has been attached to SSI1 on pins PD0-3  */

static struct SD_INFO
{
    uint8_t sd_status;
    uint8_t sd_type;
} sd_info;
    
static __packed struct Cmd_Frm
{
    uint8_t cmd;
    uint32_t arg;
    uint8_t crc;
} cmdfrm;

// return the opposite endianness
#define tog_endian(in)  ((in << 24)              |  \
                        ((in & 0x0000FF00) << 8) |  \
                        ((in & 0x00FF0000) >> 8) |  \
                        (in >> 24))

int mci_send_cmd(uint8_t cmd, uint32_t arg)
{
    // check for ACMDs
    if (cmd & 0x80)
    {
        mci_send_cmd(CMD55, 0);
        mci_wait_reply(MCI_REPLY_R1, NULL, MCI_WAIT_SHORT);
    }
    
    // setup cmd frame
    cmdfrm.cmd = cmd & ~(0x80);
    cmdfrm.arg = tog_endian(arg);
    
    // set CRC
    if (cmd == CMD0)
        cmdfrm.crc = 0x95;
    else if (cmd == CMD8)
        cmdfrm.crc = 0x87;
    /*
    else: we don't care about the CRC
    */
    
    // send cmd
    spi_send(MCI_SPI, &cmdfrm, sizeof(cmdfrm));
    
    return 0;
}

int mci_wait_reply(MCI_REPLY_TYPE type, MCI_REPLY *reply, uint8_t tries)
{
    uint8_t data;
    while(tries--)
    {
        spi_recv(MCI_SPI, &data, sizeof(data));
        
        // check if MSB is not zero
        if (data & 0x80)
        {
            // still busy so we skip it
            continue;
        }
        
        if (type == MCI_REPLY_R1)
        {
            if (reply != NULL)
                reply->r1 = data;
            return data;
        }
        else if (type == MCI_REPLY_R1b)
        {
            // wait for reply
            break;
        }
        else if (type == MCI_REPLY_R3)
        {
            reply->r1 = data;
            spi_recv(MCI_SPI, &reply->ocr, sizeof(reply->ocr));
            reply->ocr = tog_endian(reply->ocr);
            return data;
        }
    }
    
    if (type == MCI_REPLY_R1b)
    {
        // wait for not busy (non-zero on data)
        do
        {
            spi_recv(MCI_SPI, &data, sizeof(data));
        } while (data == 0);
        return 0;
    }
    
    // time out
    return -1;
}

#define mci_dummy_clocks()  spi_send(MCI_SPI, NULL, 10)

DSTATUS mci_disk_initialize (void)
{
    MCI_REPLY reply;
    int rc, t = 750;
        
    if (sd_info.sd_status & (STA_NOINIT | STA_NODISK))
        return sd_info.sd_status;
    
    sd_info.sd_status = STA_NOINIT | STA_NODISK;
    sd_info.sd_type = SD_UNKNOWN;
    
    // init spi
    spi_init(MCI_SPI, SPI_CLK_SLOW, SPI_MODE_3);
    // send out some dummy clocks to power up
    mci_dummy_clocks();
    // use SPI mode
    mci_send_cmd(CMD0, 0);
    if (mci_wait_reply(MCI_REPLY_R1, &reply, MCI_WAIT_SHORT) == -1)
        return sd_info.sd_status;
    else if (reply.r1 != 0x01)
        if (mci_wait_reply(MCI_REPLY_R1, NULL, MCI_WAIT_SHORT) != 0x01)
            return sd_info.sd_status;

    // raise the SPI clock speed
    spi_set_clk(MCI_SPI, SPI_CLK_FAST);
 
    // check card version
    mci_send_cmd(CMD8, 0x1AA);
    if (mci_wait_reply(MCI_REPLY_R3, &reply, MCI_WAIT_SHORT) & R1_ILLIGALCMD_M)
    {
        sd_info.sd_type = SD_V1;
    }

    // bring card out of idle
    do
    {
        mci_send_cmd(A(CMD41), 0x40000000);
        rc = mci_wait_reply(MCI_REPLY_R1, NULL, MCI_WAIT_LONG);
    } while((rc == -1 || rc == 0x01) && t--);

    // check if card is out of idle
    if (rc != 0)
        return sd_info.sd_status;
    
    // check for HC
    mci_send_cmd(CMD58, 0);
    mci_wait_reply(MCI_REPLY_R3, &reply, MCI_WAIT_SHORT);
    if (reply.ocr & OCR_CCS_M)
        sd_info.sd_type = SD_V2_HC;
    else
        sd_info.sd_type = SD_V2_SC;
        
    return sd_info.sd_status &= ~(STA_NODISK | STA_NOINIT);
}
    
DSTATUS mci_disk_status (void)
{
    return sd_info.sd_status;
}

DRESULT mci_disk_read (BYTE* buff, DWORD sector, UINT count)
{
    uint8_t token;
    
    if (count == 0)
        return RES_PARERR;
    if (sd_info.sd_status & STA_NOINIT)
        return RES_NOTRDY;
    
    /*
     * On a SDSC card, the sector address is a byte address on the SD Card
     * On a SDHC card, the sector address is address by sector blocks
     */
    if (sd_info.sd_type != SD_V2_HC)
        sector *= SD_SECTOR_SIZE;  // Convert to byte address
    
    /* Single block read */
    if (count == 1)
    {
        //if (mci_send_cmd(CMD17, sector, buff, SD_SECTOR_SIZE, MCI_WAIT_SHORT) != -1)
        mci_send_cmd(CMD17, sector);
        if (mci_wait_reply(MCI_REPLY_R1, NULL, MCI_WAIT_SHORT) == 0)
        {
            // cmd accepted wait for token
            do
            {
                spi_recv(MCI_SPI, &token, 1);
            } while(token == 0xFF);
            
            // check if error token received
            if (token & 0xE0)
            {
                // not an error token
                spi_recv(MCI_SPI, buff, SD_SECTOR_SIZE);
                // ignore CRC
                spi_recv(MCI_SPI, NULL, 2);
                count--;
            }
        }
    }
    /* Multiple block read */
    else
    {
        mci_send_cmd(CMD18, sector);
        if (mci_wait_reply(MCI_REPLY_R1, NULL, MCI_WAIT_SHORT) == 0)
        {
            // cmd accepted
            while(count--)
            {
                // wait for token
                do
                {
                    spi_recv(MCI_SPI, &token, 1);
                } while(token == 0xFF);
                
                // check if error token received
                if (token & 0xE0)
                {
                    // not an error token, get block
                    spi_recv(MCI_SPI, buff, SD_SECTOR_SIZE);
                    // ignore CRC
                    spi_recv(MCI_SPI, NULL, 2);
                    buff += SD_SECTOR_SIZE;
                }
                else
                {
                    break;
                }
            }

            /* STOP_TRANSMISSION */
            mci_send_cmd(CMD12, 0);
            // skip a byte after CMD12
            spi_recv(MCI_SPI, NULL, 1);
            // wait for reply
            mci_wait_reply(MCI_REPLY_R1b, NULL, MCI_WAIT_LONG);
        }
    }
    
    return (count ? RES_ERROR : RES_OK);
}

DRESULT mci_disk_write (const BYTE* buff, DWORD sector, UINT count)
{
    return RES_NOTRDY;
}

DRESULT mci_disk_ioctl (BYTE cmd, void* buff)
{
    return RES_NOTRDY;
}


