#include "xbox.h"

// https://wiki.osdev.org/ACPI_Timer
// On Xbox this is located at 0x8008

uint32_t APIC_SPEED = NB_FSB_FREQ;

uint32_t xbox_timer_query_performance_frequency(void)
{
    return ACPI_TIMER_FREQ;
}

uint32_t xbox_timer_query_performance_counter(void)
{
    return io_input_dword(ACPI_TIMER_IO_PORT);
}

void xbox_timer_spin_wait(uint32_t ticks)
{
    uint32_t start = xbox_timer_query_performance_counter();
    uint32_t end = start + ticks;
    if (end < start) {
        while (xbox_timer_query_performance_counter() > start)
            ;
    }
    while (xbox_timer_query_performance_counter() < end)
        ;
}

// uint32_t timer_get_tick_count(void) {
//     return xTaskGetTickCount();
// }

// On stock this is 133.333Mhz, however overclocked, modified or emulated xboxes (xemu) can have different values so we
// measure it
void xbox_timer_measure_apic_frequency(void)
{
#define APIC_READ(reg)           (*((volatile uint32_t *)(XBOX_APIC_BASE + reg)))
#define APIC_LVT_TIMER           APIC_READ(0x320UL)
#define APIC_TIMER_INITIAL_COUNT APIC_READ(0x380UL)
#define APIC_TIMER_CURRENT_COUNT APIC_READ(0x390UL)
#define APIC_TASK_PRIORITY       APIC_READ(0x80UL)
#define APIC_LDR                 APIC_READ(0xD0UL)
#define APIC_TMRDIV              APIC_READ(0x3E0UL)
#define APIC_LVT_PERF            APIC_READ(0x340UL)
#define APIC_LVT_LINT0           APIC_READ(0x350UL)
#define APIC_LVT_LINT1           APIC_READ(0x360UL)
#define APIC_DISABLE             (1UL << 16UL)
#define APIC_NMI                 (4 << 8)
#define APIC_DIV_1               (0x0B)
#define MEASUREMENT_PERIOD_MS    (5)

    // Setup to clean up
    APIC_LDR = 0xFFFFFFFF;
    APIC_LDR = ((APIC_LDR & 0x00FFFFFF) | 0x00000001);
    APIC_LVT_TIMER = APIC_DISABLE;
    APIC_LVT_PERF = APIC_NMI;
    APIC_LVT_LINT0 = APIC_DISABLE;
    APIC_LVT_LINT1 = APIC_DISABLE;
    APIC_TASK_PRIORITY = 0;

    // Run at full rate
    APIC_TMRDIV = APIC_DIV_1;

    // Set APIC init counter to -1
    APIC_TIMER_INITIAL_COUNT = 0xFFFFFFFF;
    APIC_TIMER_CURRENT_COUNT = 0xFFFFFFFF;

    // Wait to allow APIC to count down
    const uint32_t wait_ticks = MEASUREMENT_PERIOD_MS * xbox_timer_query_performance_frequency() / 1000;
    xbox_timer_spin_wait(wait_ticks);

    // Stop the APIC timer
    APIC_LVT_TIMER = APIC_DISABLE;

    // Now we know how often the APIC timer has ticked in our period
    uint32_t ticks = 0xFFFFFFFF - APIC_TIMER_CURRENT_COUNT;
    APIC_SPEED = ticks * (1000 / MEASUREMENT_PERIOD_MS);

    printf("Measured APIC Speed: %u Hz\n", APIC_SPEED);
}
