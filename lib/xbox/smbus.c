#include <stdint.h>
#include "xbox.h"

#define SBMUS_IO_BASE 0xc000
#define SMBUS_STATUS (SBMUS_IO_BASE + 0x00)
#define SMBUS_CONTROL (SBMUS_IO_BASE + 0x02)
#define SMBUS_ADDRESS (SBMUS_IO_BASE + 0x04)
#define SMBUS_DATA (SBMUS_IO_BASE + 0x06)
#define SMBUS_COMMAND (SBMUS_IO_BASE + 0x08)
#define SMBUS_BLOCK_DATA (SBMUS_IO_BASE + 0x09)

int smbus_write(uint8_t address, uint8_t reg, uint8_t size_of_data, uint32_t data)
{
    int retries = 50;

    while (io_input_word(SMBUS_STATUS) & 0x0800)
    {
        // Spin while bus is busy with any master traffic
    }

    while (retries--)
    {
        uint8_t status;
        uint16_t temp;

        io_output_byte(SMBUS_ADDRESS, (address << 1) | 0);
        io_output_byte(SMBUS_COMMAND, reg);

        switch (size_of_data)
        {
        case 4:
            io_output_byte(SMBUS_BLOCK_DATA, data & 0xFF);
            io_output_byte(SMBUS_BLOCK_DATA, (data >> 8) & 0xFF);
            io_output_byte(SMBUS_BLOCK_DATA, (data >> 16) & 0xFF);
            io_output_byte(SMBUS_BLOCK_DATA, (data >> 24) & 0xFF);
            io_output_word(SMBUS_DATA, size_of_data);
            break;
        case 2:
            io_output_word(SMBUS_DATA, data & 0xFFFF);
            break;
        default: // 1
            io_output_word(SMBUS_DATA, data & 0xFF);
            break;
        }

        temp = io_input_word(SMBUS_STATUS);
        io_output_word(SMBUS_STATUS, temp); // Clear preexisting errors

        switch (size_of_data)
        {
        case 4:
            io_output_byte(SMBUS_CONTROL, 0x1D); // u32 mode
            break;
        case 2:
            io_output_byte(SMBUS_CONTROL, 0x1B); // u16 mode
            break;
        default:                                 // 1
            io_output_byte(SMBUS_CONTROL, 0x1A); // u8 mode
            break;
        }

        status = 0;
        while ((status & 0x36) == 0)
        {
            status = io_input_byte(SMBUS_STATUS);
        }

        if ((status & 0x10) != 0)
        {
            return 1;
        }

        // wait_us(1);
    }

    return 0;
}

int smbus_read(uint8_t address, uint8_t reg, uint8_t size_of_data, uint32_t *data)
{
    int retries = 50;

    while (io_input_word(SMBUS_STATUS) & 0x0800)
    {
        // Spin while bus is busy with any master traffic
    }

    while (retries--)
    {
        uint8_t status;
        int temp;

        io_output_byte(SMBUS_ADDRESS, (address << 1) | 1);
        io_output_byte(SMBUS_COMMAND, reg);

        temp = io_input_word(SMBUS_STATUS);
        io_output_word(SMBUS_STATUS, temp); // Clear preexisting errors

        switch (size_of_data)
        {
        case 4:
            io_output_byte(SMBUS_CONTROL, 0x0D); // u32 mode
            break;
        case 2:
            io_output_byte(SMBUS_CONTROL, 0x0B); // u16 mode
            break;
        default:
            io_output_byte(SMBUS_CONTROL, 0x0A); // u8 mode
            break;
        }

        status = 0;
        while ((status & 0x36) == 0)
        {
            status = io_input_byte(SMBUS_STATUS);
        }

        if (status & 0x24)
        {
            // Handle error
        }

        if (!(status & 0x10))
        {
            // Retry if not complete
        }
        else
        {
            switch (size_of_data)
            {
            case 4:
                io_input_byte(SMBUS_DATA);
                io_input_byte(SMBUS_BLOCK_DATA);
                io_input_byte(SMBUS_BLOCK_DATA);
                io_input_byte(SMBUS_BLOCK_DATA);
                io_input_byte(SMBUS_BLOCK_DATA);
                break;
            case 2:
                *data = io_input_word(SMBUS_DATA);
                break;
            default:
                *data = io_input_byte(SMBUS_DATA);
                break;
            }

            return 1;
        }
    }

    return 0;
}
