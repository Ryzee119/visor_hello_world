// SPDX-License-Identifier: CC0-1.0
#include <stdatomic.h>
#include <stdint.h>

#include "cpu.h"
#include "io.h"
#include "lock.h"
#include "pic8259.h"

static atomic_flag pic_lock;

__attribute__((constructor)) static void pic8259_init(void)
{
    atomic_flag_clear(&pic_lock);
}

void pic8259_irq_enable(uint8_t data_port, uint8_t irq)
{
    spinlock_acquire(&pic_lock);
    uint8_t mask = io_input_byte(data_port);
    io_output_byte(data_port, mask & (uint8_t) ~(1 << irq));
    spinlock_release(&pic_lock);
}

void pic8259_irq_disable(uint8_t data_port, uint8_t irq)
{
    spinlock_acquire(&pic_lock);
    uint8_t mask = io_input_byte(data_port);
    io_output_byte(data_port, mask | (uint8_t)(1 << irq));
    spinlock_release(&pic_lock);
}