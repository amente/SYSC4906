#include "fatfs/diskio.h"

#ifndef __MCI_H
#define __MCI_H

#define MCI_SPI                     SSI1

/* Definitions for MMC/SDC command taken from TI SDSPITiva.c <--- thx */        
#define CMD0                        (0x40+0)    /* GO_IDLE_STATE */
#define CMD1                        (0x40+1)    /* SEND_OP_COND */
#define CMD8                        (0x40+8)    /* SEND_IF_COND */
#define CMD9                        (0x40+9)    /* SEND_CSD */
#define CMD10                       (0x40+10)   /* SEND_CID */
#define CMD12                       (0x40+12)   /* STOP_TRANSMISSION */
#define CMD16                       (0x40+16)   /* SET_BLOCKLEN */
#define CMD17                       (0x40+17)   /* READ_SINGLE_BLOCK */
#define CMD18                       (0x40+18)   /* READ_MULTIPLE_BLOCK */
#define CMD23                       (0x40+23)   /* SET_BLOCK_COUNT */
#define CMD24                       (0x40+24)   /* WRITE_BLOCK */
#define CMD25                       (0x40+25)   /* WRITE_MULTIPLE_BLOCK */
#define CMD41                       (0x40+41)   /* SEND_OP_COND (ACMD) */
#define CMD55                       (0x40+55)   /* APP_CMD */
#define CMD58                       (0x40+58)   /* READ_OCR */
#define A(n)                        (n | 0x80)     /* remove MSB in mci_send_cmd  */

#define R1_IDLE_M                   0x01
#define R1_ILLIGALCMD_M             0x04

#define OCR_CCS_M                   0x40000000

typedef uint8_t  R1;
typedef uint32_t OCR;
typedef uint8_t  MCI_REPLY_TYPE;
typedef struct 
{
    R1 r1;
    OCR ocr;
} MCI_REPLY;
#define MCI_REPLY_R1                0
#define MCI_REPLY_R1b               1
#define MCI_REPLY_R3                2

#define SD_V1                       0
#define SD_V2_SC                    1
#define SD_V2_HC                    2
#define SD_UNKNOWN                  255

#define SD_SECTOR_SIZE              512

#define START_BLOCK_TOKEN           0xFE
#define START_MULTIBLOCK_TOKEN      0xFC
#define STOP_MULTIBLOCK_TOKEN       0xFD

#define MCI_WAIT_SHORT              8
#define MCI_WAIT_LONG               255

/*  FatFs functions  */
DSTATUS mci_disk_initialize (void);
DSTATUS mci_disk_status (void);
DRESULT mci_disk_read (BYTE* buff, DWORD sector, UINT count);
DRESULT mci_disk_write (const BYTE* buff, DWORD sector, UINT count);
DRESULT mci_disk_ioctl (BYTE cmd, void* buff);

int mci_send_cmd(uint8_t cmd, uint32_t arg);
int mci_wait_reply(MCI_REPLY_TYPE type, MCI_REPLY *reply, uint8_t tries);

#endif
