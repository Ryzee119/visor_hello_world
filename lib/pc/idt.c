// SPDX-License-Identifier: CC0-1.0

#include "idt.h"
#include <stdint.h>
#include <stdio.h>

void idt_install_interrupt(uint8_t vector, void (*handler)(void), uint8_t type)
{
    // FIXME, should we be disabling interrupts here?
    uint16_t usCodeSegment;
    uint32_t ulBase = (uint32_t)system_get_physical_address(handler);

    // Get the current IDT
    IDTPointer_t idtr;
    __asm__ __volatile__("sidt %0" : "=m"(idtr));
    IDTEntry_t *idt_table = (IDTEntry_t *)idtr.ulTableBase;

    // Fill in the IDT entry
    idt_table[vector].usISRLow = (uint16_t)(ulBase & 0xFFFF);
    idt_table[vector].usISRHigh = (uint16_t)((ulBase >> 16UL) & 0xFFFF);

    __asm volatile("mov %%cs, %0" : "=r"(usCodeSegment));
    idt_table[vector].usSegmentSelector = usCodeSegment;
    idt_table[vector].ucZero = 0;
    idt_table[vector].ucFlags = type;
}
