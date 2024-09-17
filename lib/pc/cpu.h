// SPDX-License-Identifier: CC0-1.0

#ifndef IA32_CPU_H
#define IA32_CPU_H

#include <stdint.h>

static inline __attribute__((always_inline)) void cpu_read_msr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

static inline __attribute__((always_inline)) void cpu_write_msr(uint32_t msr, uint32_t lo, uint32_t hi)
{
    asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

static inline __attribute__((always_inline)) void cpu_disable_cache(void)
{
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 30);
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

static inline __attribute__((always_inline)) void cpu_enable_cache(void)
{
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 30);
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

static inline __attribute__((always_inline)) void cpu_read_cpuid(uint32_t code, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(code));
}

void system_yield(uint32_t ms);

static inline __attribute__((always_inline)) void cpu_read_msr64(uint32_t msr, uint64_t *p)
{
    uint32_t __lo, __hi;
    cpu_read_msr(msr, &__lo, &__hi);
    *(p) = ((uint64_t)__hi << 32) | __lo;
}

static inline __attribute__((always_inline)) void cpu_write_msr64(uint32_t msr, uint64_t v)
{
    uint32_t __lo = (uint32_t)(v);
    uint32_t __hi = (uint32_t)((v) >> 32);
    cpu_write_msr(msr, __lo, __hi);
}

#endif // IA32_CPU_H