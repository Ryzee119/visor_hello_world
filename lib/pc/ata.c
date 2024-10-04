#include "ata.h"
#include "cpu.h"
#include "io.h"
#include "pci.h"
#include <stdint.h>

#define outb(port, val) io_output_byte(port, val)
#define inb(port)       io_input_byte(port)
#define inw(port)       io_input_word(port)
#define outw(port, val) io_output_word(port, val)
#define outl(port, val) io_output_dword(port, val)

#define ATA_ACTIVE_MASTER 0x00
#define ATA_ACTIVE_SLAVE  0x01

static uint8_t active_primary_bus_device = ATA_ACTIVE_MASTER;
static uint8_t active_secondary_bus_device = ATA_ACTIVE_MASTER;
static uint8_t primary_bus_80wire = 0;
static uint8_t secondary_bus_80wire = 0;

struct prd_entry
{
    uint32_t base_addr;
    uint16_t byte_count;
    uint16_t flags;
} __attribute__((packed));

static struct prd_entry prd_table[1];

static void ata_io_wait(ata_device_t *ata_device)
{
    volatile uint8_t alt_status;
    alt_status = inb(ata_device->ctrl_base);
    alt_status = inb(ata_device->ctrl_base);
    alt_status = inb(ata_device->ctrl_base);
    alt_status = inb(ata_device->ctrl_base);
}

static int8_t ata_busy_wait(ata_device_t *ata_device, uint32_t timeout)
{
    uint8_t status;
    do {
        status = inb(ata_device->io_base + ATA_IO_STATUS);
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
            return -1;
        }
        system_yield(1);
    } while ((status & ATA_STATUS_BSY) && timeout--);
    return (timeout) ? 0 : -2;
}

static int8_t ata_data_wait(ata_device_t *ata_device)
{
    uint8_t status;
    uint32_t timeout = ATA_DRQ_TIMEOUT;
    do {
        status = inb(ata_device->io_base + ATA_IO_STATUS);
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
            return -1;
        }
        system_yield(1);
    } while (!(status & ATA_STATUS_DRQ) && timeout--);
    return (timeout) ? 0 : -2;
}

// This will reset both ATA devices on the bus
static void ata_bus_reset(ata_device_t *ata_device)
{
    outb(ata_device->ctrl_base, ATA_CTRL_SOFTWARE_RESET);
    ata_io_wait(ata_device);
    ata_io_wait(ata_device);
    ata_io_wait(ata_device);
    outb(ata_device->ctrl_base, 0x00);
}

void ata_bus_init()
{
}

int8_t ata_device_init(ata_device_t *ata_device, uint16_t busmaster_base, uint16_t ctrl_base, uint16_t io_base,
                       uint8_t master)
{
    uint8_t status;
    int8_t error;
    ata_device->ctrl_base = ctrl_base;
    ata_device->io_base = io_base;
    ata_device->busmaster_base = busmaster_base;
    ata_device->slave = (master) ? 0 : 1;

    ata_bus_reset(ata_device);
    ata_io_wait(ata_device);

    // Check for floating bus (no drives at all)
    status = inb(ata_device->io_base + ATA_IO_STATUS);
    if (status == 0xFF) {
        return -1;
    }

    // Place bread crumbs then read back (Poor mans way of checking if this port is probably ATA bus)
    outb(ata_device->io_base + ATA_IO_LBA_LOW, 0x55);
    outb(ata_device->io_base + ATA_IO_LBA_MID, 0xAA);
    if (inb(ata_device->io_base + ATA_IO_LBA_LOW) != 0x55 || inb(ata_device->io_base + ATA_IO_LBA_MID) != 0xAA) {
        return -1;
    }

    // Select the device and prepare for an identify command
    outb(ata_device->io_base + ATA_IO_DRIVE, 0xA0 | (ata_device->slave << 4));
    outb(ata_device->io_base + ATA_IO_SECTOR_COUNT, 0);
    outb(ata_device->io_base + ATA_IO_LBA_LOW, 0);
    outb(ata_device->io_base + ATA_IO_LBA_MID, 0);
    outb(ata_device->io_base + ATA_IO_LBA_HIGH, 0);

    // Send identify
    outb(ata_device->io_base + ATA_IO_COMMAND, ATA_CMD_IDENTIFY);
    status = inb(ata_device->io_base + ATA_IO_STATUS);
    if (status == 0) {
        return -1;
    }

    error = ata_busy_wait(ata_device, ATA_BSY_TIMEOUT);
    if (error) {
        return error;
    }

    // Because of some ATAPI drives do not follow spec, at this point check the LBA mid and LBA hi ports
    // to see if they are non-zero. If so, the drive is not ATA, and you should stop polling.
    if (inb(ata_device->io_base + ATA_IO_LBA_MID) != 0 || inb(ata_device->io_base + ATA_IO_LBA_HIGH) != 0) {
        return -1;
    }

    // Read in the identify data and pull out some key information
    for (uint16_t i = 0; i < 256; i++) {
        uint16_t wtemp = inw(ata_device->io_base + ATA_IO_DATA);
        uint8_t *btemp = (uint8_t *)&wtemp;

        // LBA48 support
        if (i == 83) {
            if (wtemp & (1 << 10)) {
                ata_device->lba48 = 1;
            }

        }

        // UDMA information
        else if (i == 88) {
            ata_device->actual_udma_mode = (wtemp >> 8) & 0xFF;
            ata_device->supported_udma_mode = (wtemp & 0xFF);

        }


        else if (i == 93 && master) {
            if (wtemp & (1 << 11)) {
                ata_device->wire80 = 1;
            } else {
                ata_device->supported_udma_mode = 1 | 2; // Restrict to UDMA 2 or less
            }
        }

        // LBA Addressing
        else if (i == 60) {
            ata_device->total_sector_count_lba28 = wtemp;
        } else if (i == 61) {
            ata_device->total_sector_count_lba28 |= wtemp << 16;
        }

        // LBA48 Addressing
        else if (i == 100) {
            ata_device->total_sector_count_lba48 = wtemp;
        } else if (i == 101) {
            ata_device->total_sector_count_lba48 |= wtemp << 16;
        } else if (i == 102) {
            ata_device->total_sector_count_lba48 |= (uint64_t)wtemp << 32;
        } else if (i == 103) {
            ata_device->total_sector_count_lba48 |= (uint64_t)wtemp << 48;
        }

        // Model, Serial, Firmware
        else if (i >= 27 && i < 47) {
            uint8_t j = (i - 27) * 2;
            ata_device->model[j] = btemp[1];
            ata_device->model[j + 1] = btemp[0];
        } else if (i >= 10 && i < 20) {
            uint8_t j = (i - 10) * 2;
            ata_device->serial[j] = btemp[1];
            ata_device->serial[j + 1] = btemp[0];
        } else if (i >= 23 && i < 27) {
            uint8_t j = (i - 23) * 2;
            ata_device->firmware[j] = btemp[1];
            ata_device->firmware[j + 1] = btemp[0];
        }
    }

#if (1)
    printf("[ATA] Mode: %s\n", ata_device->model);
    printf("[ATA] Serial: %s\n", ata_device->serial);
    printf("[ATA] Firmware: %s\n", ata_device->firmware);
    printf("[ATA] LBA28 Sectors: %d\n", ata_device->total_sector_count_lba28);
    printf("[ATA] LBA48 Sectors: %llu\n", ata_device->total_sector_count_lba48);
    printf("[ATA] Wire80: %d\n", ata_device->wire80);
    printf("[ATA] LBA48: %d\n", ata_device->lba48);
    printf("[ATA] Supported UDMA: %d\n", ata_device->supported_udma_mode);
    printf("[ATA] Actual UDMA: %d\n", ata_device->actual_udma_mode);
#endif

    // Wait for the drive to be ready
    error = ata_data_wait(ata_device);

    return error;
}

// For DMA, the data buffers cannot cross a 64K boundary, and must be contiguous in physical memory
int8_t ata_io(ata_device_t *ata_device, uint64_t lba, void *buffer, uint8_t sector_count, uint8_t dma, uint8_t read)
{
    uint8_t error = 0;
    uint8_t drive_base;
    if (ata_device->lba48) {
        drive_base = 0x40 | (ata_device->slave << 4);
    } else {
        drive_base = 0xE0 | (ata_device->slave << 4);
    }

    // Set up the PRD Table
    if (dma) {
        prd_table[0].base_addr = (uint32_t)buffer;
        prd_table[0].byte_count = ATA_SECTOR_SIZE * sector_count;
        prd_table[0].flags = 0x8000;

        // Set the PRD Table address in the Bus Master IDE controller using BAR4
        outl(ata_device->busmaster_base + ATA_BUSMASTER_DMA_PRDT_REG, (uint32_t)prd_table);

        // Prepare the DMA controller
        outb(ata_device->busmaster_base + ATA_BUSMASTER_DMA_COMMAND_REG,
             (read) ? ATA_BUSMASTER_DMA_COMMAND_READ : ATA_BUSMASTER_DMA_COMMAND_WRITE);

        // Clear error & interrupt flags
        outb(ata_device->busmaster_base + ATA_BUSMASTER_DMA_STATUS_REG,
             ATA_BUSMASTER_DMA_STATUS_INTERRUPT | ATA_BUSMASTER_DMA_STATUS_ERROR);
    }

    if (ata_device->lba48) {
        outb(ata_device->io_base + ATA_IO_DRIVE, drive_base);
        outb(ata_device->io_base + ATA_IO_SECTOR_COUNT, (uint8_t)(sector_count >> 8) & 0xFF);
        outb(ata_device->io_base + ATA_IO_LBA_LOW, (uint8_t)((lba >> 24) & 0xFF));
        outb(ata_device->io_base + ATA_IO_LBA_MID, (uint8_t)((lba >> 32) & 0xFF));
        outb(ata_device->io_base + ATA_IO_LBA_HIGH, (uint8_t)((lba >> 40) & 0xFF));
        outb(ata_device->io_base + ATA_IO_SECTOR_COUNT, sector_count & 0xFF);
        outb(ata_device->io_base + ATA_IO_LBA_LOW, (uint8_t)(lba & 0xFF));
        outb(ata_device->io_base + ATA_IO_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
        outb(ata_device->io_base + ATA_IO_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    } else {
        uint32_t lba28 = (uint32_t)lba;
        outb(ata_device->io_base + ATA_IO_DRIVE, drive_base | ((lba >> 24) & 0x0F));
        outb(ata_device->io_base + ATA_IO_SECTOR_COUNT, sector_count);
        outb(ata_device->io_base + ATA_IO_LBA_LOW, (uint8_t)((lba28 >> 0) & 0xFF));
        outb(ata_device->io_base + ATA_IO_LBA_MID, (uint8_t)((lba28 >> 8) & 0xFF));
        outb(ata_device->io_base + ATA_IO_LBA_HIGH, (uint8_t)((lba28 >> 16) & 0xFF));
    }

    // Disable interrupts - we are polling
    uint8_t control_register = inb(ata_device->ctrl_base);
    control_register |= ATA_CTRL_NIEN;
    outb(ata_device->ctrl_base, control_register);

    if (dma) {
        // Set the direction of the transfer
        if (read) {
            outb(ata_device->io_base + ATA_IO_COMMAND,
                 (ata_device->lba48) ? ATA_CMD_READ_LBA48_DMA : ATA_CMD_READ_LBA28_DMA);
        } else {
            outb(ata_device->io_base + ATA_IO_COMMAND,
                 (ata_device->lba48) ? ATA_CMD_WRITE_LBA48_DMA : ATA_CMD_WRITE_LBA28_DMA);
        }

        // Start the DMA transfer
        uint8_t command = inb(ata_device->busmaster_base + ATA_BUSMASTER_DMA_COMMAND_REG);
        command |= ATA_BUSMASTER_DMA_COMMAND_START;
        outb(ata_device->busmaster_base + ATA_BUSMASTER_DMA_COMMAND_REG, command);

        // Wait for the transfer to complete (timeout 5s + 100ms per MB)
        uint32_t timeout = 5000 + ((sector_count * ATA_SECTOR_SIZE) >> 5) * 100;
        while (timeout--) {
            uint8_t dma_status = inb(ata_device->busmaster_base + ATA_BUSMASTER_DMA_STATUS_REG);
            uint8_t device_status = inb(ata_device->io_base + ATA_IO_STATUS);
            if (!(dma_status & ATA_BUSMASTER_DMA_STATUS_INTERRUPT)) {
                continue;
            }
            if (!(device_status & ATA_STATUS_BSY)) {
                break;
                ;
            }
            system_yield(1);
        }

        if (timeout == 0) {
            // stop the transfer
            outb(ata_device->busmaster_base + ATA_BUSMASTER_DMA_COMMAND_REG, 0);
            error = -1;
        }

        // Check for errors
        error = inb(ata_device->busmaster_base + ATA_BUSMASTER_DMA_STATUS_REG) & ATA_BUSMASTER_DMA_STATUS_ERROR;

        // Clear the interrupt flag and error flag
        outb(ata_device->busmaster_base + ATA_BUSMASTER_DMA_STATUS_REG,
             ATA_BUSMASTER_DMA_STATUS_INTERRUPT | ATA_BUSMASTER_DMA_STATUS_ERROR);

    } else {
        // Set the direction of the transfer
        if (read) {
            outb(ata_device->io_base + ATA_IO_COMMAND,
                 (ata_device->lba48) ? ATA_CMD_READ_LBA48_PIO : ATA_CMD_READ_LBA28_PIO);
        } else {
            outb(ata_device->io_base + ATA_IO_COMMAND,
                 (ata_device->lba48) ? ATA_CMD_WRITE_LBA48_PIO : ATA_CMD_WRITE_LBA28_PIO);
        }

        uint16_t sector_index = 0;
        while (sector_count--) {
            error = ata_busy_wait(ata_device, ATA_BSY_TIMEOUT);
            if (error) {
                break;
            }
            error = ata_data_wait(ata_device);
            if (error) {
                break;
            }

            // Transfer the data (up to 256 words)
            uint32_t buffer_offset = sector_index++ * ATA_SECTOR_SIZE;
            uint32_t i = 256;
            if (read) {
                while (i--) {
                    ((uint16_t *)buffer)[buffer_offset + i] = inw(ata_device->io_base);
                }
            } else {
                while (i--) {
                    outw(ata_device->io_base, ((uint16_t *)buffer)[buffer_offset + i]);
                }
            }

            // After transferring the last uint16_t of a PIO data block to the data IO port, give the drive a 400ns
            // delay to reset its DRQ bit
            ata_io_wait(ata_device);

            // Check if an error occured
            error = inb(ata_device->io_base + ATA_IO_STATUS) & ATA_STATUS_ERR;
        }

        // Flush the cache on writes
        if (!read) {
            outb(ata_device->io_base + ATA_IO_STATUS, ATA_CMD_FLUSH_CACHE);
            ata_busy_wait(ata_device, ATA_BSY_TIMEOUT);
        }
    }

    return (error) ? -1 : 0;
}

int8_t ata_dma_read(ata_device_t *ata_device, uint32_t lba, void *buffer, uint8_t sector_count)
{
    return ata_io(ata_device, lba, buffer, sector_count, 1, 1);
}

int8_t ata_dma_write(ata_device_t *ata_device, uint32_t lba, void *buffer, uint8_t sector_count)
{
    return ata_io(ata_device, lba, buffer, sector_count, 1, 0);
}

int8_t ata_pio_read(ata_device_t *ata_device, uint32_t lba, void *buffer, uint8_t sector_count)
{
    return ata_io(ata_device, lba, buffer, sector_count, 0, 1);
}

int8_t ata_pio_write(ata_device_t *ata_device, uint32_t lba, void *buffer, uint8_t sector_count)
{
    return ata_io(ata_device, lba, buffer, sector_count, 0, 0);
}
