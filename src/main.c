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

uint32_t system_tick(void)
{
    return xTaskGetTickCount();
}

void *system_get_physical_address(void *virtual_address)
{
    return virtual_address;
}

SemaphoreHandle_t doom_mutex;
static void doom_task(void *parameters)
{
    while (1) {
        xSemaphoreTake(doom_mutex, portMAX_DELAY);
        doom_entry("C:/doom1.wad");
    }
}

ata_bus_t ata_bus;

uint32_t crc32_table[256];
#define POLYNOMIAL 0xEDB88320
void init_crc32_table() {
    uint32_t crc;
    for (uint32_t i = 0; i < 256; i++) {
        crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
}

uint32_t crc32(const uint8_t *data, size_t length) {
    static int init = 0;
    if (!init) {
        init_crc32_table();
        init = 1;
    }
    uint32_t crc = 0xFFFFFFFF;  // Initial CRC value
    while (length--) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ *data++) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;  // Final CRC value
}

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

    display_init();
    interrupts_init();
    usb_init();
    ide_bus_init(XBOX_ATA_BUSMASTER_BASE, XBOX_ATA_PRIMARY_BUS_CTRL_BASE, XBOX_ATA_PRIMARY_BUS_IO_BASE, &ata_bus);
    
    extern fs_io_ll_t ata_ll_io;
    extern fs_io_ll_t usb_ll_io;
    extern fs_io_t fat_io;
    extern fs_io_t fatx_io;
    extern fs_io_t iso9660_io;

    if (fileio_register_driver('C', &fatx_io, &ata_ll_io, NULL, &ata_bus) != 0) {
        printf_r("[FS] Error mounting drive C as FATX\n");
    }
    if (fileio_register_driver('E', &fatx_io, &ata_ll_io, NULL, &ata_bus) != 0) {
        printf_r("[FS] Error mounting drive E as FATX\n");
    }

    //if (fileio_register_driver('D', &iso9660_io, &ata_ll_io, NULL, &ata_bus) != 0) {
    //    printf_r("[FS] Error mounting drive D as ISO9660\n");
    //}

    printf_r("[FS] Filesystem mounted\n");

    doom_mutex = xSemaphoreCreateBinary();
    xTaskCreate(doom_task, "Doom!", configMINIMAL_STACK_SIZE * 2, NULL, THREAD_PRIORITY_NORMAL, NULL);

#if (0)
    directory_handle_t *dir;
    // List files in C and print their names
    #if (1)
    dir = opendir("C:/");
    if (dir != NULL) {
        printf_r("[FS] Opened directory C\n");
        directory_entry_t *entry;
        while ((entry = readdir(dir)) != NULL) {
            printf_r("[FS] Found file %s %d B\n", entry->file_name, entry->file_size);
        }
        printf_r("[FS] Done listing files in C. Closing Dir\n");
        closedir(dir);
    } else {
        printf_r("[FS] Failed to open directory\n");
    }
    printf_r("[FS] Done listing files in C\n");

    dir = opendir("E:/");
    if (dir != NULL) {
        printf_r("[FS] Opened directory E\n");
        directory_entry_t *entry;
        while ((entry = readdir(dir)) != NULL) {
            printf_r("[FS] Found file %s %d B\n", entry->file_name, entry->file_size);
        }
        closedir(dir);
    } else {
        printf_r("[FS] Failed to open directory\n");
    }

    dir = opendir("D:/");
    if (dir != NULL) {
        printf_r("[FS] Opened directory D\n");
        directory_entry_t *entry;
        while ((entry = readdir(dir)) != NULL) {
            printf_r("[FS] Found file %s %d B\n", entry->file_name, entry->file_size);
        }
        closedir(dir);
    } else {
        printf_r("[FS] Failed to open directory\n");
    }
    #endif

    //reading in C:/doom1.wad and calc crc32
    FILE *file = fopen("C:/doom1.wad", "rb");
    if (file == NULL) {
        printf_r("[FS] Failed to open file\n");
    } else {
        printf_r("[FS] Reading in doom1.wad\n");
        uint8_t *buffer = pvPortMalloc(6 * 1024 * 1024);
        memset(buffer, 0, 6 * 1024 * 1024);
        size_t total_bytes_read = 0;
        uint32_t tick_start = xTaskGetTickCount();
        while (1) {
            uint32_t random_chunk = 16384;
            size_t bytes_read = fread(&buffer[total_bytes_read], 1, random_chunk, file);
            if (bytes_read == 0) {
                break;
            }
            total_bytes_read += bytes_read;
            
        }
        uint32_t tick_end = xTaskGetTickCount();
        uint32_t crc = crc32(buffer, total_bytes_read);
        printf_r("[FS] bytes read %d CRC32: %08x, took %d ms\n", total_bytes_read, crc, tick_end - tick_start);
        fclose(file);
        vPortFree(buffer);
    }

    #if (0)
    do {
        vTaskDelay(pdMS_TO_TICKS(5000));
        dir = opendir("0:/");
    } while (dir == NULL);
    if (dir != NULL) {
        directory_entry_t *entry;
        while ((entry = readdir(dir)) != NULL) {
            printf_r("[FS] Found file %s %d B\n", entry->file_name, entry->file_size);
        }
        closedir(dir);
    } else {
        printf_r("[FS] Failed to open directory\n");
    }
    #endif
#endif

    xSemaphoreGive(doom_mutex);
    vTaskDelete(NULL);
    return;

    cpuid_eax_01 cpuid_info;
    cpu_read_cpuid(CPUID_VERSION_INFO, &cpuid_info.eax.flags, &cpuid_info.ebx.flags, &cpuid_info.ecx.flags,
                   &cpuid_info.edx.flags);

#if (0)
    printf_r("[CPU] Family: %d\n", cpuid_info.eax.family_id);
    printf_r("[CPU] Model: %d\n", cpuid_info.eax.model);
    printf_r("[CPU] Stepping: %d\n", cpuid_info.eax.stepping_id);
    printf_r("[CPU] Type: %d\n", cpuid_info.eax.processor_type);
    printf_r("[CPU] Extended Family: %d\n", cpuid_info.eax.extended_family_id);
    printf_r("[CPU] Extended Model: %d\n", cpuid_info.eax.extended_model_id);
    printf_r("[CPU] Feature Bits (EDX): 0x%08x\n", cpuid_info.edx.flags);
    printf_r("[CPU] Feature Bits (ECX): 0x%08x\n", cpuid_info.ecx.flags);
#endif

    uint8_t temp1, temp2;
    smbus_input_byte(XBOX_SMBUS_ADDRESS_TEMP, 0x00, &temp1);
    smbus_input_byte(XBOX_SMBUS_ADDRESS_TEMP, 0x01, &temp2);
    // printf_r("[SYS] CPU: %d C\n", temp1);
    // printf_r("[SYS] MB: %d C\n", temp2);

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
int gettimeofday(struct timeval *restrict tv, void *restrict tz)
{
    return -1;
}