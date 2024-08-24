#include <stdint.h>
#include "xbox.h"

// https://xboxdevwiki.net/SMBus
#define SBMUS_IO_BASE    0xc000
#define SMBUS_STATUS     (SBMUS_IO_BASE + 0x00)
#define SMBUS_CONTROL    (SBMUS_IO_BASE + 0x02)
#define SMBUS_ADDRESS    (SBMUS_IO_BASE + 0x04)
#define SMBUS_DATA       (SBMUS_IO_BASE + 0x06)
#define SMBUS_COMMAND    (SBMUS_IO_BASE + 0x08)
#define SMBUS_BLOCK_DATA (SBMUS_IO_BASE + 0x09)

#define SMBUS_STATUS_ABORT          0x0001
#define SMBUS_STATUS_COLLISION      0x0002
#define SMBUS_STATUS_PROTOCOL_ERROR 0x0004
#define SMBUS_STATUS_BUSY           0x0008
#define SMBUS_STATUS_CYCLE_COMPLETE 0x0010
#define SMBUS_STATUS_TIMEOUT        0x0020
#define SMBUS_STATUS_ERROR                                                                                             \
    (SMBUS_STATUS_ABORT | SMBUS_STATUS_COLLISION | SMBUS_STATUS_PROTOCOL_ERROR | SMBUS_STATUS_TIMEOUT)

#define SMBUS_CONTROL_TRANSFER_TYPE_DWORD 0x05
#define SMBUS_CONTROL_TRANSFER_TYPE_WORD  0x03
#define SMBUS_CONTROL_TRANSFER_TYPE_BYTE  0x02
#define SMBUS_CONTROL_START               0x08
#define SMBUS_CONTROL_USE_INTERRUPT       0x10
#define SMBUS_CONTROL_ABORT               0x20

#define SMBUS_RETURN_SUCCESS 0
#define SMBUS_RETURN_ERROR   -1

int8_t xbox_smbus_output(uint8_t address, uint8_t reg, uint32_t data, uint8_t data_len)
{
    uint16_t status;
    while (io_input_word(SMBUS_STATUS) & SMBUS_STATUS_BUSY)
        ;

    io_output_byte(SMBUS_ADDRESS, address &= ~0x01);
    io_output_byte(SMBUS_COMMAND, reg);

    // Clear previous status flags
    status = io_input_word(SMBUS_STATUS);
    io_output_word(SMBUS_STATUS, status);

    // Start the transfer
    if (data_len == 1) {
        io_output_word(SMBUS_DATA, data & 0xFF);
        io_output_byte(SMBUS_CONTROL, SMBUS_CONTROL_TRANSFER_TYPE_BYTE | SMBUS_CONTROL_START);
    } else if (data_len == 2) {
        io_output_word(SMBUS_DATA, data & 0xFFFF);
        io_output_byte(SMBUS_CONTROL, SMBUS_CONTROL_TRANSFER_TYPE_WORD | SMBUS_CONTROL_START);
    } else if (data_len == 4) {
        io_output_byte(SMBUS_BLOCK_DATA, (data >> 0) & 0xFF);
        io_output_byte(SMBUS_BLOCK_DATA, (data >> 8) & 0xFF);
        io_output_byte(SMBUS_BLOCK_DATA, (data >> 16) & 0xFF);
        io_output_byte(SMBUS_BLOCK_DATA, (data >> 24) & 0xFF);
        io_output_byte(SMBUS_DATA, data_len);
        io_output_byte(SMBUS_CONTROL, SMBUS_CONTROL_TRANSFER_TYPE_DWORD | SMBUS_CONTROL_START);
    }

    // Wait for completion
    while (io_input_byte(SMBUS_STATUS) & SMBUS_STATUS_BUSY)
        ;

    // Return the transfer status
    status = io_input_word(SMBUS_STATUS);
    if (status & SMBUS_STATUS_ERROR) {
        return SMBUS_RETURN_ERROR;
    } else {
        return SMBUS_RETURN_SUCCESS;
    }
}

int8_t xbox_smbus_input(uint8_t address, uint8_t reg, void *data, uint8_t data_len)
{
    uint16_t status;
    while (io_input_word(SMBUS_STATUS) & SMBUS_STATUS_BUSY)
        ;

    io_output_byte(SMBUS_ADDRESS, address | 0x01);
    io_output_byte(SMBUS_COMMAND, reg);

    // Clear previous status flags
    status = io_input_word(SMBUS_STATUS);
    io_output_word(SMBUS_STATUS, status);

    // Start the transfer
    if (data_len == 1) {
        io_output_byte(SMBUS_CONTROL, SMBUS_CONTROL_TRANSFER_TYPE_BYTE | SMBUS_CONTROL_START);
    } else if (data_len == 2) {
        io_output_byte(SMBUS_CONTROL, SMBUS_CONTROL_TRANSFER_TYPE_WORD | SMBUS_CONTROL_START);
    } else if (data_len == 4) {
        io_output_byte(SMBUS_CONTROL, SMBUS_CONTROL_TRANSFER_TYPE_DWORD | SMBUS_CONTROL_START);
    }

    // Wait for completion
    while (io_input_byte(SMBUS_STATUS) & SMBUS_STATUS_BUSY)
        ;

    // Read in the data
    status = io_input_word(SMBUS_STATUS);
    if (status & SMBUS_STATUS_ERROR) {
        return SMBUS_RETURN_ERROR;
    } else {
        if (data_len == 1) {
            *(uint8_t *)data = io_input_byte(SMBUS_DATA);
            return SMBUS_RETURN_SUCCESS;
        } else if (data_len == 2) {
            *(uint16_t *)data = io_input_word(SMBUS_DATA);
            return SMBUS_RETURN_SUCCESS;
        } else if (data_len == 4) {
            uint32_t data32 = io_input_byte(SMBUS_BLOCK_DATA);
            data32 |= io_input_byte(SMBUS_BLOCK_DATA) << 8;
            data32 |= io_input_byte(SMBUS_BLOCK_DATA) << 16;
            data32 |= io_input_byte(SMBUS_BLOCK_DATA) << 24;
            *(uint32_t *)data = data32;
            return SMBUS_RETURN_SUCCESS;
        }
    }
    return SMBUS_RETURN_ERROR;
}
