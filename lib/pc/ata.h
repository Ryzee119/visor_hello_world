// SPDX-License-Identifier: CC0-1.0

#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#define ATA_BUSMASTER_DMA_COMMAND_REG 0x00
#define ATA_BUSMASTER_DMA_STATUS_REG  0x02
#define ATA_BUSMASTER_DMA_PRDT_REG    0x04

#define ATA_BUSMASTER_DMA_COMMAND_START (1 << 0)
#define ATA_BUSMASTER_DMA_COMMAND_READ  (1 << 3)
#define ATA_BUSMASTER_DMA_COMMAND_WRITE (0 << 3)

#define ATA_BUSMASTER_DMA_STATUS_ERROR      (1 << 1)
#define ATA_BUSMASTER_DMA_STATUS_INTERRUPT  (1 << 2)

#define ATA_IO_DATA         0
#define ATA_IO_ERROR        1
#define ATA_IO_FEATURES     1
#define ATA_IO_SECTOR_COUNT 2
#define ATA_IO_LBA_LOW      3
#define ATA_IO_LBA_MID      4
#define ATA_IO_LBA_HIGH     5
#define ATA_IO_DRIVE        6
#define ATA_IO_COMMAND      7
#define ATA_IO_STATUS       7

#define ATA_CTRL_SOFTWARE_RESET (1 << 2)
#define ATA_CTRL_NIEN           (1 << 1)

#define ATA_CMD_READ_LBA28_DMA  0xC8
#define ATA_CMD_READ_LBA48_DMA  0x25
#define ATA_CMD_WRITE_LBA28_DMA 0xCA
#define ATA_CMD_WRITE_LBA48_DMA 0x35
#define ATA_CMD_READ_LBA28_PIO  0x20
#define ATA_CMD_READ_LBA48_PIO  0x24
#define ATA_CMD_WRITE_LBA28_PIO 0x30
#define ATA_CMD_WRITE_LBA48_PIO 0x34
#define ATA_CMD_FLUSH_CACHE     0xE7
#define ATA_CMD_IDENTIFY        0xEC

#define ATA_STATUS_ERR (1 << 0) // Indicates an error occurred. Send a new command to clear it
#define ATA_STATUS_DRQ (1 << 3) // Set when the drive has PIO data to transfer, or is ready to accept PIO data.
#define ATA_STATUS_SRV (1 << 4) // Overlapped Mode Service Request. This bit is set when the drive requires service in an overlapped operation.
#define ATA_STATUS_DF  (1 << 5) // Drive Fault. Indicates that a fault occurred during a drive operation.
#define ATA_STATUS_RDY (1 << 6) // Bit is clear when drive is spun down, or after an error. Set otherwise.
#define ATA_STATUS_BSY (1 << 7) // Indicates the drive is preparing to send/receive data (wait for it to clear).

#define ATA_SECTOR_SIZE 512 // Sector size in bytes

#ifndef ATA_DRQ_TIMEOUT
#define ATA_DRQ_TIMEOUT 2000
#endif

#ifndef ATA_BSY_TIMEOUT
#define ATA_BSY_TIMEOUT 5000
#endif


typedef struct ata_device
{
    union
    {
        uint8_t flags;
        struct
        {
            uint8_t slave : 1;
            uint8_t lba48 : 1;
            uint8_t wire80 : 1;
            uint8_t reserved : 5;
        };
    };
    uint16_t ctrl_base;
    uint16_t io_base;
    uint16_t busmaster_base;

    uint8_t supported_udma_mode;
    uint8_t actual_udma_mode;
    uint32_t total_sector_count_lba28;
    uint64_t total_sector_count_lba48;

    uint8_t model[41];
    uint8_t serial[21];
    uint8_t firmware[9];

} ata_device_t;

int8_t ata_device_init(ata_device_t *ata_device, uint16_t busmaster_base, uint16_t ctrl_base, uint16_t io_base,
                       uint8_t master);

#endif // ATA_H