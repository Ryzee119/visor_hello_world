#include "main.h"

static uint8_t freertos_running = 0;
extern void vPortYieldCall(void);
extern void vPortTimerHandler(void);

void system_yield(uint32_t ms)
{
    if (freertos_running) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
        xbox_timer_spin_wait(XBOX_TIMER_MS_TO_TICKS(ms));
    }
}

static void freertos_entry(void *parameters)
{
    (void)parameters;
    printf("FreeRTOS entry\n");

    // FreeRTOS x86 port uses the LAPIC timer. This must be used in-conjunction with the IOAPIC interrupt controller.
    // Although the xbox should have both of these peripherals, I could not get the IOAPIC to work. I suspect there is
    // some PCI address space write to enable it (where!?). Also APU overlays IOAPIC address space (fixable).
    // Therefore we replace the timer callbacks with the PIC timer interrupt and disable the APIC
    mmio_output_dword(XBOX_APIC_BASE + APIC_LVT_LINT0, 0x00000700);
    mmio_output_dword(XBOX_APIC_BASE + APIC_SIV, 0);
    xPortInstallInterruptHandler(vPortTimerHandler, XBOX_PIC1_BASE_VECTOR_ADDRESS + XBOX_PIT_TIMER_IRQ);
    pic8259_irq_enable(XBOX_PIC1_DATA_PORT, XBOX_PIT_TIMER_IRQ);

    __asm__ __volatile__("" ::: "memory");
    freertos_running = 1;

    interrupts_init();
    usb_init();

    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}

int main(void)
{
    // We create this task statically. FreeRTOS calls freertos_entry immediately after vTaskStartScheduler without any context switch
    // which is good because we can setup the PIC timer with FreeRTOS context before the scheduler actually starts.
    static StaticTask_t freertos_entry_task;
    static StackType_t freertos_entry_stack[configMINIMAL_STACK_SIZE];
    xTaskCreateStatic(freertos_entry, "Entry", configMINIMAL_STACK_SIZE, NULL, THREAD_PRIORITY_LOWEST, freertos_entry_stack, &freertos_entry_task);

    vTaskStartScheduler();

    // Should never get here
    assert(0);
    return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("Stack overflow in task %s\n", pcTaskName);
}
