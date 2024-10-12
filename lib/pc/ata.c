#include "ata.h"
#include "atapi.h"
#include "cpu.h"
#include "io.h"
#include "lock.h"
#include "pci.h"
#include <stdint.h>

atomic_flag lock;

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

static __inline void insw(uint16_t __port, void *__buf, unsigned long __n)
{
    __asm__ __volatile__("cld; rep; insw" : "+D"(__buf), "+c"(__n) : "d"(__port));
}

static __inline__ void outsw(uint16_t __port, const void *__buf, unsigned long __n)
{
    uint16_t *buf = (uint16_t *)__buf;
    for (uint32_t i = 0; i < __n; i++) {
        outw(__port, buf[i]);
    }
}

static void ata_io_400ns(ata_bus_t *ata_bus)
{
    volatile uint8_t alt_status;
    alt_status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
    alt_status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
    alt_status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
    alt_status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
}

static int8_t ata_busy_wait(ata_bus_t *ata_bus, uint32_t timeout)
{
    uint8_t status;
    uint32_t loop_count = 0;
    ata_io_400ns(ata_bus);
    do {
        status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
            return -1;
        }
        system_yield(0);
    } while ((status & ATA_STATUS_BSY) && timeout--);
    return (timeout) ? 0 : -2;
}

static int8_t ata_data_wait(ata_bus_t *ata_bus)
{
    uint8_t status;
    uint32_t timeout = ATA_DRQ_TIMEOUT;

    if (ata_busy_wait(ata_bus, ATA_BSY_TIMEOUT) < 0) {
        return -1;
    }

    do {
        status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
            return -1;
        }
        system_yield(0);
    } while (!(status & ATA_STATUS_DRQ) && timeout--);
    return (timeout) ? 0 : -2;
}

// This will reset both ATA devices on the bus
static void ata_bus_reset(ata_bus_t *ata_bus)
{
    uint8_t control_register = inb(ata_bus->ctrl_base);
    control_register |= ATA_CTRL_SOFTWARE_RESET;
    outb(ata_bus->ctrl_base, control_register);

    system_yield(1); // > 5us

    control_register &= ~ATA_CTRL_SOFTWARE_RESET;
    outb(ata_bus->ctrl_base, control_register);

    system_yield(2);

    ata_data_wait(ata_bus);

    // On reset, drive is reset to master automatically, but set it explicitly
    outb(ata_bus->io_base + ATA_IO_DRIVE, ATA_IO_DRIVE_SELECT_0);
}

static void ata_set_irq_en(ata_bus_t *ata_bus, uint8_t enable)
{
    uint8_t control_register = inb(ata_bus->ctrl_base);
    if (enable) {
        control_register &= ~ATA_CTRL_NIEN;
    } else {
        control_register |= ATA_CTRL_NIEN;
    }
    outb(ata_bus->ctrl_base, control_register);
}

static int8_t ata_send_command(ata_bus_t *ata_bus, uint8_t device_index, ata_command_t *ata_command)
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    const uint8_t old_drive_base = inb(ata_bus->io_base + ATA_IO_DRIVE);
    const uint8_t is_lba28 = ATA_CMD_IS_LBA28(ata_command->command);
    const uint8_t is_lba48 = ATA_CMD_IS_LBA48(ata_command->command);
    const uint64_t lba = BSWAP_LE64(ata_command->lba); // Function assumes LE
    int8_t error = 0;

    // Make sure the index is valid
    device_index = (device_index) ? ATA_ACTIVE_SLAVE : ATA_ACTIVE_MASTER;

    // Build the drive select byte
    uint8_t drive_base = ATA_IO_DRIVE_SELECT_0 | (device_index << 4);
    if (is_lba28 || is_lba48) {
        drive_base |= ATA_IO_DRIVE_SELECT_LBA_MASK;
        if (is_lba28) {
            drive_base |= (lba >> 24) & 0x0F;
        }
    }

    // Select the drive, if changed wait for it to be ready
    outb(ata_bus->io_base + ATA_IO_DRIVE, drive_base);
    if ((old_drive_base ^ drive_base) & (1 << 4)) {
        error = ata_data_wait(ata_bus);
        if (error) {
            return error;
        }
    }

    if (is_lba48) {
        outb(ata_bus->io_base + ATA_IO_FEATURES, 0);
        outb(ata_bus->io_base + ATA_IO_SECTOR_COUNT, (uint8_t)(ata_command->sector_count >> 8) & 0xFF);
        outb(ata_bus->io_base + ATA_IO_LBA_LOW, (uint8_t)((lba >> 24) & 0xFF));
        outb(ata_bus->io_base + ATA_IO_LBA_MID, (uint8_t)((lba >> 32) & 0xFF));
        outb(ata_bus->io_base + ATA_IO_LBA_HIGH, (uint8_t)((lba >> 40) & 0xFF));
    }

#if (0)
    printf("[ATA] ATA_IO_DRIVE %02x\n", drive_base);
    printf("[ATA] FEATURE %02x\n", ata_command->feature);
    printf("[ATA] SECTOR_COUNT %02x\n", ata_command->sector_count);
    printf("[ATA] ATA_IO_LBA_LOW %02x\n", (uint8_t)((lba >> 0) & 0xFF));
    printf("[ATA] ATA_IO_LBA_MID %02x\n", (uint8_t)((lba >> 8) & 0xFF));
    printf("[ATA] ATA_IO_LBA_HIGH %02x\n", (uint8_t)((lba >> 16) & 0xFF));
    printf("[ATA] ATA_IO_COMMAND %02x\n", ata_command->command);
#endif

    outb(ata_bus->io_base + ATA_IO_FEATURES, ata_command->feature);
    outb(ata_bus->io_base + ATA_IO_SECTOR_COUNT, ata_command->sector_count & 0xFF);
    outb(ata_bus->io_base + ATA_IO_LBA_LOW, (uint8_t)((lba >> 0) & 0xFF));
    outb(ata_bus->io_base + ATA_IO_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ata_bus->io_base + ATA_IO_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    outb(ata_bus->io_base + ATA_IO_COMMAND, ata_command->command);
    ata_io_400ns(ata_bus);

    if (inb(ata_bus->io_base + ATA_IO_STATUS) & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
        return -1;
    }

    return ata_data_wait(ata_bus);
}

static int8_t atapi_send_command(ata_bus_t *ata_bus, uint8_t device_index, uint8_t atapi_command[ATAPI_CMD_SIZE])
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    int8_t error = 0;
    if (ide_device->is_present == 0) {
        return -1;
    }

    ata_command_t ata_command = {
        .command = ATA_CMD_PACKET,
        .lba = ide_device->sector_size << 8,
        .sector_count = 0,
        .feature = 0,
    };

    error = ata_send_command(ata_bus, device_index, &ata_command);
    if (error) {
        goto bail_out;
    }

    // Send the atapi command
    outsw(ata_bus->io_base + ATA_IO_DATA, atapi_command, ATAPI_CMD_SIZE / 2);

    // Wait for the drive to be ready
    error = ata_data_wait(ata_bus);
    if (error) {
        goto bail_out;
    }

    // Check for errors
    uint8_t status = inb(ata_bus->io_base + ATA_IO_STATUS);
    if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
        error = -1;
    }

bail_out:
    return error;
}

static int8_t atapi_dma_transfer(ata_bus_t *ata_bus, uint8_t device_index, uint8_t atapi_command[12], void *buffer,
                                 uint32_t sector_count)
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    // FIXME: Implement ATAPI DMA
    return -1;
}

static int8_t ata_dma_transfer(ata_bus_t *ata_bus, uint8_t device_index, ata_command_t *ata_command, uint8_t read,
                               void *buffer, uint16_t sector_count)
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    if (ide_device->is_present == 0 || ide_device->is_atapi == 1) {
        return -1;
    }

    int8_t error = 0;
    spinlock_acquire(&lock);

    error = ata_send_command(ata_bus, device_index, ata_command);
    if (error) {
        goto bail_out;
    }

    ata_set_irq_en(ata_bus, 0);

    // Set up the PRD Table
    prd_table[0].base_addr = (uint32_t)buffer;
    prd_table[0].byte_count = ATA_SECTOR_SIZE * sector_count;
    prd_table[0].flags = 0x8000;
    outl(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_PRDT_REG, (uint32_t)prd_table);

    // Prepare the DMA controller
    outb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_COMMAND_REG,
         (read) ? ATA_BUSMASTER_DMA_COMMAND_READ : ATA_BUSMASTER_DMA_COMMAND_WRITE);

    // Clear error & interrupt flags
    outb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_STATUS_REG,
         ATA_BUSMASTER_DMA_STATUS_INTERRUPT | ATA_BUSMASTER_DMA_STATUS_ERROR);

    // Start the DMA transfer
    uint8_t command = inb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_COMMAND_REG);
    command |= ATA_BUSMASTER_DMA_COMMAND_START;
    outb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_COMMAND_REG, command);

    // Wait for the transfer to complete (timeout 5s + 100ms per MB)
    uint32_t timeout = 5000 + ((sector_count * ATA_SECTOR_SIZE) >> 5) * 100;
    while (timeout--) {
        uint8_t dma_status = inb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_STATUS_REG);
        uint8_t device_status = inb(ata_bus->io_base + ATA_IO_STATUS);
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
        outb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_COMMAND_REG, 0);
        error = -1;
    }

    // Check for errors
    error = inb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_STATUS_REG) & ATA_BUSMASTER_DMA_STATUS_ERROR;

    // Clear the interrupt flag and error flag
    outb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_STATUS_REG,
         ATA_BUSMASTER_DMA_STATUS_INTERRUPT | ATA_BUSMASTER_DMA_STATUS_ERROR);

bail_out:
    ata_set_irq_en(ata_bus, 1);
    spinlock_release(&lock);
    return error;
}

static int8_t ide_dma_io(ata_bus_t *ata_bus, uint8_t device_index, uint8_t read, uint32_t lba, void *buffer,
                         uint32_t sector_count)
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    if (ide_device->is_present == 0) {
        return -1;
    }

    if (ide_device->is_atapi) {
        atapi_read12_cmd_t atapi_command = {
            .opcode = (read) ? ATAPI_CMD_READ_12 : ATAPI_CMD_WRITE_12,
            .lba = BSWAP_BE32(lba),
            .transfer_length = BSWAP_BE32(sector_count),
            .control = 0,
            .flags = 0,
            .reserved_10 = 0,
        };
        return atapi_dma_transfer(ata_bus, device_index, (uint8_t *)&atapi_command, buffer, sector_count);
    } else {
        ata_command_t ata_command = {
            .command = (read) ? ATA_CMD_READ_LBA48_DMA : ATA_CMD_WRITE_LBA48_DMA,
            .lba = lba,
            .sector_count = sector_count,
            .feature = 0,
        };
        if (ide_device->ata.lba48_supported == 0) {
            ata_command.command = (read) ? ATA_CMD_READ_LBA28_DMA : ATA_CMD_WRITE_LBA28_DMA;
        }
        return ata_dma_transfer(ata_bus, device_index, &ata_command, read, buffer, sector_count);
    }
}

static int8_t ide_pio_io(ata_bus_t *ata_bus, uint8_t device_index, uint8_t read, uint32_t lba, void *buffer,
                         uint32_t sector_count)
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    if (ide_device->is_present == 0) {
        return -1;
    }

    if (ide_device->is_atapi) {
        atapi_read12_cmd_t atapi_command = {
            .opcode = (read) ? ATAPI_CMD_READ_12 : ATAPI_CMD_WRITE_12,
            .lba = BSWAP_BE32(lba),
            .transfer_length = BSWAP_BE32(sector_count),
            .control = 0,
            .flags = 0,
            .reserved_10 = 0,
        };
        return atapi_pio_transfer(ata_bus, device_index, (uint8_t *)&atapi_command, read, buffer, sector_count);
    } else {
        ata_command_t ata_command = {
            .command = (read) ? ATA_CMD_READ_LBA48_PIO : ATA_CMD_WRITE_LBA48_PIO,
            .lba = lba,
            .sector_count = sector_count,
            .feature = 0,
        };
        // Use LBA28 if LBA48 is not supported
        if (ide_device->ata.lba48_supported == 0) {
            // LBA28 is limited to 28 bits
            if (lba > 0x0FFFFFFF) {
                return -1;
            }
            ata_command.command = (read) ? ATA_CMD_READ_LBA28_PIO : ATA_CMD_WRITE_LBA28_PIO;
        }
        return ata_pio_transfer(ata_bus, device_index, &ata_command, read, buffer);
    }
}

// Send low level commands to ATA devices (HDDs). Read is 1 for read, 0 for write, if there is no data transfer, set
// buffer to NULL
int8_t ata_pio_transfer(ata_bus_t *ata_bus, uint8_t device_index, ata_command_t *ata_command, uint8_t read,
                        void *buffer)
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    if (ide_device->is_present == 0 || ide_device->is_atapi == 1) {
        return -1;
    }

    int8_t error = 0;
    spinlock_acquire(&lock);

    error = ata_send_command(ata_bus, device_index, ata_command);
    if (error) {
        goto bail_out;
    }

    ata_set_irq_en(ata_bus, 0);

    // We are done if no data is being transferred
    if (buffer == NULL) {
        goto bail_out;
    }

    for (uint32_t i = 0; i < ata_command->sector_count; i++) {

        error = ata_data_wait(ata_bus);
        if (error) {
            goto bail_out;
        }

        if (read) {
            insw(ata_bus->io_base + ATA_IO_DATA, ((uint16_t *)buffer) + i * ide_device->sector_size,
                 ide_device->sector_size / 2);
        } else {
            outsw(ata_bus->io_base + ATA_IO_DATA, ((uint16_t *)buffer) + i * ide_device->sector_size,
                  ide_device->sector_size / 2);
        }
    }

    error = ata_data_wait(ata_bus);
    if (error) {
        goto bail_out;
    }

    // Flush the cache on writes
    if (!read) {
        outb(ata_bus->io_base + ATA_IO_COMMAND, ATA_CMD_FLUSH_CACHE);
        ata_data_wait(ata_bus);
    }

bail_out:
    ata_set_irq_en(ata_bus, 1);
    spinlock_release(&lock);
    return error;
}

// Send low level commands to ATAPI devices (CD/DVD). Read is 1 for read, 0 for write, if there is no data transfer, set
// buffer to NULL
int8_t atapi_pio_transfer(ata_bus_t *ata_bus, uint8_t device_index, uint8_t atapi_command[12], uint8_t read,
                          void *buffer, uint32_t sector_count)
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    if (ide_device->is_present == 0 || ide_device->is_atapi == 0) {
        return -1;
    }

    int8_t error = 0;
    spinlock_acquire(&lock);

    error = atapi_send_command(ata_bus, device_index, atapi_command);
    if (error) {
        goto bail_out;
    }

    // We are done if no data is being transferred
    if (buffer == NULL) {
        goto bail_out;
    }

    ata_set_irq_en(ata_bus, 0);

    for (uint32_t i = 0; i < sector_count; i++) {

        error = ata_data_wait(ata_bus);
        if (error) {
            break;
        }

        uint16_t words_available =
            ((inb(ata_bus->io_base + ATA_IO_LBA_HIGH) << 8) | inb(ata_bus->io_base + ATA_IO_LBA_MID)) / 2;

        printf("[ATAPI] Words available %d\n", words_available);
        if (read) {
            insw(ata_bus->io_base + ATA_IO_DATA, ((uint16_t *)buffer) + i * ide_device->sector_size, words_available);
        } else {
            outsw(ata_bus->io_base + ATA_IO_DATA, ((uint16_t *)buffer) + i * ide_device->sector_size, words_available);
        }
    }

    // Flush the cache on writes
    if (!read) {
        outb(ata_bus->io_base + ATA_IO_COMMAND, ATA_CMD_FLUSH_CACHE);
        ata_data_wait(ata_bus);
    }

bail_out:
    ata_set_irq_en(ata_bus, 1);
    spinlock_release(&lock);
    return error;
}

/* IDE functions atuomatically determine if ATA or ATAPI commands should be used, PIO and DMA do the same thing but you
 * probably want to use DMA */
int8_t ide_bus_init(uint16_t busmaster_base, uint16_t ctrl_base, uint16_t io_base, ata_bus_t *ata_bus)
{
    uint8_t status;
    int8_t error;

    memset(ata_bus, 0, sizeof(ata_bus_t));
    ata_bus->ctrl_base = ctrl_base;
    ata_bus->io_base = io_base;
    ata_bus->busmaster_base = busmaster_base;

    // Check for floating bus (no drives at all)
    status = inb(ata_bus->io_base + ATA_IO_STATUS);
    if (status == 0xFF) {
        return -1;
    }

    ata_bus_reset(ata_bus);

    for (uint8_t i = 0; i < 2; i++) {
        ide_device_t *ide_device = (i == 0) ? &ata_bus->master : &ata_bus->slave;
        memset(ide_device, 0, sizeof(ide_device_t));
        ide_device->sector_size = ATA_SECTOR_SIZE;

        ata_set_irq_en(ata_bus, 0);

        // Check for ATA device, if not present, check for ATAPI
        ata_command_t cmd = {0};
        cmd.command = ATA_CMD_IDENTIFY;
        error = ata_send_command(ata_bus, i, &cmd);
        if (error) {
            cmd.command = ATAPI_CMD_SOFT_RESET;
            error = ata_send_command(ata_bus, i, &cmd);

            cmd.command = ATA_CMD_PACKET_IDENTIFY;
            error = ata_send_command(ata_bus, i, &cmd);
            if (error) {
                ata_set_irq_en(ata_bus, 1);
                continue;
            }
            ide_device->is_atapi = 1;
            ide_device->sector_size = ATAPI_SECTOR_SIZE;
            ide_device->atapi.total_sector_count = 0;
        }

        error = ata_data_wait(ata_bus);
        if (error) {
            ata_set_irq_en(ata_bus, 1);
            continue;
        }

        ide_device->is_present = 1;

        // Read in the identify data and pull out some key information
        for (uint16_t i = 0; i < 256; i++) {
            uint16_t wtemp = inw(ata_bus->io_base + ATA_IO_DATA);
            uint8_t *btemp = (uint8_t *)&wtemp;

            // LBA48 support
            if (i == 83) {
                if (wtemp & (1 << 10)) {
                    ide_device->ata.lba48_supported = 1;
                }
            }

            // UDMA information
            else if (i == 88) {
                ide_device->actual_udma_mode = (wtemp >> 8) & 0xFF;
                ide_device->supported_udma_mode = (wtemp & 0xFF);

            }

            else if (i == 93 && (i == 0)) {
                if (wtemp & (1 << 11)) {
                    ata_bus->wire80 = 1;
                } else {
                    ide_device->supported_udma_mode = 1 | 2; // Restrict to UDMA 2 or less
                }
            }

            // LBA Addressing
            else if (i == 60) {
                ide_device->ata.total_sector_count_lba28 = wtemp;
            } else if (i == 61) {
                ide_device->ata.total_sector_count_lba28 |= wtemp << 16;
                ide_device->ata.total_sector_count_lba28 =
                    BSWAP_LE32_TO_NATIVE(ide_device->ata.total_sector_count_lba28);
            }

            // LBA48 Addressing
            else if (i == 100) {
                ide_device->ata.total_sector_count_lba48 = wtemp;
            } else if (i == 101) {
                ide_device->ata.total_sector_count_lba48 |= wtemp << 16;
            } else if (i == 102) {
                ide_device->ata.total_sector_count_lba48 |= (uint64_t)wtemp << 32;
            } else if (i == 103) {
                ide_device->ata.total_sector_count_lba48 |= (uint64_t)wtemp << 48;
                ide_device->ata.total_sector_count_lba48 =
                    BSWAP_LE64_TO_NATIVE(ide_device->ata.total_sector_count_lba48);
            }

            // Model, Serial, Firmware
            else if (i >= 27 && i < 47) {
                uint8_t j = (i - 27) * 2;
                ide_device->model[j] = btemp[1];
                ide_device->model[j + 1] = btemp[0];
            } else if (i >= 10 && i < 20) {
                uint8_t j = (i - 10) * 2;
                ide_device->serial[j] = btemp[1];
                ide_device->serial[j + 1] = btemp[0];
            } else if (i >= 23 && i < 27) {
                uint8_t j = (i - 23) * 2;
                ide_device->firmware[j] = btemp[1];
                ide_device->firmware[j + 1] = btemp[0];
            }
        }

        ata_set_irq_en(ata_bus, 1);

#if (1)
        if (ide_device->is_present) {
            printf("\n[ATA] Mode: %s\n", ide_device->model);
            printf("[ATA] Serial: %s\n", ide_device->serial);
            printf("[ATA] Firmware: %s\n", ide_device->firmware);
            printf("[ATA] LBA28 Sectors: %d\n", ide_device->ata.total_sector_count_lba28);
            printf("[ATA] LBA48 Sectors: %llu\n", ide_device->ata.total_sector_count_lba48);
            // printf("[ATA] Wire80: %d\n", ata_bus->wire80);
            printf("[ATA] Supported UDMA: %d\n", ide_device->supported_udma_mode);
            printf("[ATA] Actual UDMA: %d\n", ide_device->actual_udma_mode);
        }
#endif

        if (ide_device->is_atapi) {
            system_yield(1000);
            uint8_t atapi_command_buffer[ATAPI_CMD_SIZE];
            printf("[ATAPI] Device found on %d\n", i);

            // Temporarily set the sector size to 8 bytes because we don't know the real size yet
            ide_device->sector_size = 8;

            memset(atapi_command_buffer, 0, ATAPI_CMD_SIZE);
            ((atapi_read_capacity_cmd_t *)atapi_command_buffer)->opcode = ATAPI_CMD_READ_CAPACITY;

            atapi_read_capacity_response_t atapi_read_capacity_response;
            error = atapi_pio_transfer(ata_bus, i, atapi_command_buffer, 1, &atapi_read_capacity_response, 1);
            if (error) {
                printf("[ATAPI] Error reading capacity\n");
                continue;
            }

            ide_device->sector_size = BSWAP_BE32_TO_NATIVE(atapi_read_capacity_response.block_size);
            ide_device->atapi.total_sector_count = BSWAP_BE32_TO_NATIVE(atapi_read_capacity_response.last_lba) + 1;

            printf("[ATAPI] Sector Size: %d, Last LBA %llu\n\n\n", ide_device->sector_size,
                   ide_device->atapi.total_sector_count);
        }
    }

    ata_bus_reset(ata_bus);
    return error;
}

int8_t ide_pio_read(ata_bus_t *ata_bus, uint8_t device_index, uint32_t lba, void *buffer, uint32_t sector_count)
{
    return ide_pio_io(ata_bus, device_index, 1, lba, buffer, sector_count);
}

int8_t ide_pio_write(ata_bus_t *ata_bus, uint8_t device_index, uint32_t lba, void *buffer, uint32_t sector_count)
{
    return ide_pio_io(ata_bus, device_index, 0, lba, buffer, sector_count);
}

// For DMA, the data buffers cannot cross a 64K boundary, and must be contiguous in physical memory
int8_t ide_dma_read(ata_bus_t *ata_bus, uint8_t device_index, uint32_t lba, void *buffer, uint32_t sector_count)
{
    return ide_dma_io(ata_bus, device_index, 1, lba, buffer, sector_count);
}

// For DMA, the data buffers cannot cross a 64K boundary, and must be contiguous in physical memory
int8_t ide_dma_write(ata_bus_t *ata_bus, uint8_t device_index, uint32_t lba, void *buffer, uint32_t sector_count)
{
    return ide_dma_io(ata_bus, device_index, 0, lba, buffer, sector_count);
}
