// SPDX-License-Identifier: MIT

#include "xbox.h"
#include <stdatomic.h>

static atomic_flag lock;

#define SERIAL_THR 0
#define SERIAL_LSR 5

void xbox_serial_init(void)
{
    // Enter config mode
    io_output_byte(XBOX_SIO_INDEX_PORT, 0x55);

    // Select COM1
    io_output_byte(XBOX_SIO_INDEX_PORT, 0x07);
    io_output_byte(XBOX_SIO_DATA_PORT, 0x04);

    // Enable COM1
    io_output_byte(XBOX_SIO_INDEX_PORT, 0x30);
    io_output_byte(XBOX_SIO_DATA_PORT, 0x01);

    // Set COM1 base address
    io_output_byte(XBOX_SIO_INDEX_PORT, 0x61);
    io_output_byte(XBOX_SIO_DATA_PORT, (XBOX_SERIAL_COM1 >> 0) & 0xFF);
    io_output_byte(XBOX_SIO_INDEX_PORT, 0x60);
    io_output_byte(XBOX_SIO_DATA_PORT, (XBOX_SERIAL_COM1 >> 8) & 0xFF);

    // Exit config mode
    io_output_byte(XBOX_SIO_INDEX_PORT, 0xAA);
}

__attribute__((section(".boot_code"))) void xbox_serial_putchar(char character)
{
    spinlock_acquire(&lock);

    /* Wait for THRE (bit 5) to be high */
    while ((io_input_byte(XBOX_SERIAL_COM1 + SERIAL_LSR) & (1 << 5)) == 0) {
        system_yield(0);
    }
    io_output_byte(XBOX_SERIAL_COM1 + SERIAL_THR, character);

#if (0)
    if (character == '\n') {
        while ((io_input_byte(XBOX_SERIAL_COM1 + SERIAL_LSR) & (1 << 5)) == 0) {
            system_yield(0);
        }
        io_output_byte(XBOX_SERIAL_COM1 + SERIAL_THR, '\r');
    }
#endif
    spinlock_release(&lock);
}
