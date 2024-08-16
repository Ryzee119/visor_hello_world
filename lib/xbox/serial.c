#include "xbox.h"

#define SERIAL_PORT 0x3f8
#define SERIAL_THR 0
#define SERIAL_LSR 5

void serial_init(void)
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

void serial_putchar(char character)
{
    /* Wait for THRE (bit 5) to be high */
    while ((io_input_byte(SERIAL_PORT + SERIAL_LSR) & (1 << 5)) == 0)
        ;
    io_output_byte(SERIAL_PORT + SERIAL_THR, character);

    if (character == '\n')
    {
        while ((io_input_byte(SERIAL_PORT + SERIAL_LSR) & (1 << 5)) == 0)
            ;
        io_output_byte(SERIAL_PORT + SERIAL_THR, '\r');
    }
}
