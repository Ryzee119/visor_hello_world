// SPDX-License-Identifier: MIT

#include "xbox.h"
#include <stdatomic.h>

atomic_flag serial_lock;

#define SERIAL_PORT 0x3f8
#define SERIAL_THR  0
#define SERIAL_LSR  5

__attribute__((section(".boot_code"))) 
void xbox_serial_init(void)
{
    io_output_byte(0x2e, 0x55);
    io_output_byte(0x2e, 0x07);
    io_output_byte(0x2f, 0x04);
    io_output_byte(0x2e, 0x30);
    io_output_byte(0x2f, 0x01);
    io_output_byte(0x2e, 0x61);
    io_output_byte(0x2f, SERIAL_PORT & 0xff);
    io_output_byte(0x2e, 0x60);
    io_output_byte(0x2f, SERIAL_PORT >> 8);
    io_output_byte(0x2e, 0xAA);
}

__attribute__((section(".boot_code"))) 
void xbox_serial_putchar(char character)
{

    spinlock_acquire(&serial_lock);

    /* Wait for THRE (bit 5) to be high */
    while ((io_input_byte(SERIAL_PORT + SERIAL_LSR) & (1 << 5)) == 0) {
       // system_yield(0);
    }
    io_output_byte(SERIAL_PORT + SERIAL_THR, character);

    if (character == '\n') {
        while ((io_input_byte(SERIAL_PORT + SERIAL_LSR) & (1 << 5)) == 0) {
            //system_yield(0);
        }
        io_output_byte(SERIAL_PORT + SERIAL_THR, character);
    }

    spinlock_release(&serial_lock);
}
