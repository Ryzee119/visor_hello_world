#ifndef IA32_IO_H
#define IA32_IO_H

#include <stdint.h>

static __inline void io_output_byte(uint16_t address, uint8_t value)
{
    __asm__ __volatile__("outb %b0,%w1" : : "a"(value), "Nd"(address));
}

static __inline void io_output_word(uint16_t address, uint16_t value)
{
    __asm__ __volatile__("outw %0,%w1" : : "a"(value), "Nd"(address));
}

static __inline void io_output_dword(uint16_t address, uint32_t value)
{
    __asm__ __volatile__("outl %0,%w1" : : "a"(value), "Nd"(address));
}

static __inline uint8_t io_input_byte(uint16_t address)
{
    uint8_t v;
    __asm__ __volatile__("inb %w1,%0" : "=a"(v) : "Nd"(address));
    return v;
}

static __inline uint16_t io_input_word(uint16_t address)
{
    uint16_t _v;
    __asm__ __volatile__("inw %w1,%0" : "=a"(_v) : "Nd"(address));
    return _v;
}

static __inline uint32_t io_input_dword(uint16_t address)
{
    uint32_t _v;
    __asm__ __volatile__("inl %w1,%0" : "=a"(_v) : "Nd"(address));
    return _v;
}

#endif // IA32_IO_H
