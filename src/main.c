#include "main.h"

static uint8_t freertos_running = 0;
extern void vPortYieldCall(void);
extern void vPortTimerHandler(void);

// System wide yield function that spin waits or yields if possible
void system_yield(uint32_t ms)
{
    if (freertos_running) {
        if (ms == 0) {
            taskYIELD();
        } else {
            vTaskDelay(pdMS_TO_TICKS(ms));
        }
    } else {
        xbox_timer_spin_wait(XBOX_TIMER_MS_TO_TICKS(ms));
    }
}

SemaphoreHandle_t doom_mutex;
static void doom_task(void *parameters)
{
    while (1) {
        printf_r("[DOOM] Waiting for USB device...\n");
        xSemaphoreTake(doom_mutex, portMAX_DELAY);

        {
            FILINFO fno;
            DIR dir;
            FRESULT res = f_opendir(&dir, "");
            if (res != FR_OK) {
                printf_r("[DOOM] f_opendir failed with error %d\n", res);
                continue;
            }

            printf_r("[DOOM] Reading files on USB\n");
            for (;;) {
                res = f_readdir(&dir, &fno);
                if (res != FR_OK || fno.fname[0] == 0) {
                    break;
                }
                if (!(fno.fattrib & AM_DIR)) {
                    if (strstr(fno.fname, ".wad") || strstr(fno.fname, ".WAD")) {
                        printf_r("   %8lu  %s\n", fno.fsize, fno.fname);
                    }
                }
            }
        }
        doom_entry("0:/doom1.wad");
    }
}

ata_bus_t ata_bus;

static void freertos_entry(void *parameters)
{
    (void)parameters;
    printf("FreeRTOS entry\n");

    xbox_led_output(XLED_RED, XLED_RED, XLED_RED, XLED_RED);

    // FreeRTOS x86 port uses the LAPIC timer. This must be used in-conjunction with the IOAPIC interrupt controller.
    // Although the xbox should have both of these peripherals, I could not get the IOAPIC to work. I suspect there is
    // some PCI address space write to enable it (where!?). Also APU overlays IOAPIC address space (fixable).
    // Therefore we replace the timer callbacks with the PIC timer interrupt and disable the APIC
    mmio_output_dword(XBOX_APIC_BASE + APIC_LVT_LINT0, 0x00000700);
    mmio_output_dword(XBOX_APIC_BASE + APIC_SIV, 0);
    xPortInstallInterruptHandler(vPortTimerHandler, XBOX_PIC1_BASE_VECTOR_ADDRESS + XBOX_PIT_TIMER_IRQ);
    pic8259_irq_enable(XBOX_PIC1_DATA_PORT, XBOX_PIT_TIMER_IRQ);

    freertos_running = 1;

    doom_mutex = xSemaphoreCreateBinary();
    xTaskCreate(doom_task, "Doom!", configMINIMAL_STACK_SIZE * 2, NULL, THREAD_PRIORITY_NORMAL, NULL);

    display_init();
    interrupts_init();
    usb_init();
    ide_bus_init(XBOX_ATA_BUSMASTER_BASE, XBOX_ATA_PRIMARY_BUS_CTRL_BASE, XBOX_ATA_PRIMARY_BUS_IO_BASE, &ata_bus);
#if (1)
    uint8_t *sector_buffer = pvPortMalloc(ATA_SECTOR_SIZE * 4096);
    //aligned to 4096 bites
    sector_buffer = (uint8_t *)(((uint32_t)sector_buffer + 4095) & ~4095);
    printf_r("[ATA] Reading sector 0\n");
    int8_t error = ide_pio_read(&ata_bus, 0, 3, sector_buffer, 1);
    if (error) {
        printf_r("[ATA] Error reading sector 0\n");
    } else {
        printf_r("[ATA] Sector 0: %p\n", sector_buffer);
        for (uint32_t i = 0; i < 8; i++) {
            printf_r("%02x", sector_buffer[i]);
        }
        printf_r("\n");
    }
#endif 
    vTaskDelay(pdMS_TO_TICKS(10000));
    cpuid_eax_01 cpuid_info;
    cpu_read_cpuid(CPUID_VERSION_INFO, &cpuid_info.eax.flags, &cpuid_info.ebx.flags, &cpuid_info.ecx.flags,
                   &cpuid_info.edx.flags);

    printf_r("[CPU] Family: %d\n", cpuid_info.eax.family_id);
    printf_r("[CPU] Model: %d\n", cpuid_info.eax.model);
    printf_r("[CPU] Stepping: %d\n", cpuid_info.eax.stepping_id);
    printf_r("[CPU] Type: %d\n", cpuid_info.eax.processor_type);
    printf_r("[CPU] Extended Family: %d\n", cpuid_info.eax.extended_family_id);
    printf_r("[CPU] Extended Model: %d\n", cpuid_info.eax.extended_model_id);
    printf_r("[CPU] Feature Bits (EDX): 0x%08x\n", cpuid_info.edx.flags);
    printf_r("[CPU] Feature Bits (ECX): 0x%08x\n", cpuid_info.ecx.flags);

    uint8_t temp1, temp2;
    smbus_input_byte(XBOX_SMBUS_ADDRESS_TEMP, 0x00, &temp1);
    smbus_input_byte(XBOX_SMBUS_ADDRESS_TEMP, 0x01, &temp2);
    printf_r("[SYS] CPU: %d C\n", temp1);
    printf_r("[SYS] MB: %d C\n", temp2);

    xbox_led_output(XLED_GREEN, XLED_GREEN, XLED_GREEN, XLED_GREEN);

    vTaskDelete(NULL);
}

int main(void)
{
    // We create this task statically; should always succeed.
    // FreeRTOS calls freertos_entry immediately after vTaskStartScheduler without any tick
    // which is good because we can setup the PIC timer with FreeRTOS context before the scheduler actually starts.
    static StaticTask_t freertos_entry_task;
    static StackType_t freertos_entry_stack[configMINIMAL_STACK_SIZE];
    xTaskCreateStatic(freertos_entry, "FreeRTOS!", configMINIMAL_STACK_SIZE, NULL, THREAD_PRIORITY_NORMAL,
                      freertos_entry_stack, &freertos_entry_task);
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

void _exit(int code)
{
    while (1)
        ;
}