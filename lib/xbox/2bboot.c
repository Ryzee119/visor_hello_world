#include "xbox.h"

void main(void);

__attribute__((section(".boot_code"))) static void smc_io(uint8_t command, uint8_t read, void *data)
{
    uint8_t *data8 = (uint8_t *)data;

    if (read) {
        read = 1;
    }

    while (io_input_byte(SMBUS_STATUS) & SMBUS_STATUS_BUSY)
        ;

    io_output_byte(SMBUS_ADDRESS, XBOX_SMBUS_ADDRESS_SMC | read);
    io_output_byte(SMBUS_COMMAND, command);
    io_output_word(SMBUS_STATUS, 0xFFFF);

    if (read == 0) {
        io_output_byte(SMBUS_DATA, *data8);
    }

    io_output_byte(SMBUS_CONTROL, SMBUS_CONTROL_TRANSFER_TYPE_BYTE | SMBUS_CONTROL_START);

    while (io_input_byte(SMBUS_STATUS) & SMBUS_STATUS_BUSY)
        ;

    if (read) {
        *data8 = io_input_byte(SMBUS_DATA);
    }
    return;
}

__attribute__((section(".boot_code"))) void boot_pic_challenge_response(void)
{
    uint8_t bC, bD, bE, bF;

    smc_io(0x1C, SMBUS_READ, &bC);
    smc_io(0x1D, SMBUS_READ, &bD);
    smc_io(0x1E, SMBUS_READ, &bE);
    smc_io(0x1F, SMBUS_READ, &bF);

    uint8_t b1 = 0x33;
    uint8_t b2 = 0xED;
    uint8_t b3 = ((bC << 2) ^ (bD + 0x39) ^ (bE >> 2) ^ (bF + 0x63));
    uint8_t b4 = ((bC + 0x0b) ^ (bD >> 2) ^ (bE + 0x1b));
    uint8_t n = 4;

    while (n--) {
        b1 += b2 ^ b3;
        b2 += b1 ^ b4;
    }

    uint16_t w = ((uint16_t)b2 << 8) | b1;

    uint8_t w1, w2;
    w1 = (w >> 0) & 0xFF;
    w2 = (w >> 8) & 0xFF;
    smc_io(0x20, SMBUS_WRITE, &w1);
    smc_io(0x21, SMBUS_WRITE, &w2);
    return;
}

static void fill_rect_argb8888(void *fb, int fb_width, int fb_height, int x, int y, int w, int h, uint32_t argb)
{
    if (x < 0 || y < 0 || x + w > fb_width || y + h > fb_height) {
        return;
    }
    uint32_t *framebuffer = (uint32_t *)fb;

    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            int pixel_index = (y + j) * fb_width + (x + i);
            framebuffer[pixel_index] = argb;
        }
    }
}

static uint16_t argb8888_to_rgb565(uint32_t argb)
{
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t g = (argb >> 8) & 0xFF;
    uint8_t b = argb & 0xFF;

    uint16_t rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    return rgb565;
}

// Fill rectangle in RGB565 framebuffer
static void fill_rect_rgb565(void *fb, int fb_width, int fb_height, int x, int y, int w, int h, uint32_t argb)
{
    uint16_t color = argb8888_to_rgb565(argb); // Convert ARGB8888 to RGB565
    uint16_t *framebuffer = (uint16_t *)fb;

    // Loop through each pixel within the rectangle and set its color
    for (int j = 0; j < h; ++j) {
        int py = y + j;
        if (py < 0 || py >= fb_height) {
            continue; // Clip vertically
        }

        for (int i = 0; i < w; ++i) {
            int px = x + i;
            if (px < 0 || px >= fb_width) {
                continue; // Clip horizontally
            }

            framebuffer[py * fb_width + px] = color;
        }
    }
}

extern void __libc_init_array(void);
void boot(void)
{
    // Calls functions defined with __attribute__((constructor))
   __libc_init_array();

    xbox_led_output(XLED_RED, XLED_RED, XLED_RED, XLED_RED);

    xbox_serial_init();
    printf("Hello, World!\n");

    cpu_disable_cache();
    cpu_update_microcode();
    cpu_enable_cache();

    cpuid_eax_01 cpuid_info;
    cpu_read_cpuid(CPUID_VERSION_INFO, &cpuid_info.eax.flags, &cpuid_info.ebx.flags, &cpuid_info.ecx.flags, &cpuid_info.edx.flags);

    printf("CPU Family: %d\n", cpuid_info.eax.family_id);
    printf("CPU Model: %d\n", cpuid_info.eax.model);
    printf("CPU Stepping: %d\n", cpuid_info.eax.stepping_id);
    printf("CPU Type: %d\n", cpuid_info.eax.processor_type);
    printf("CPU Extended Family: %d\n", cpuid_info.eax.extended_family_id);
    printf("CPU Extended Model: %d\n", cpuid_info.eax.extended_model_id);

    // Feature bits from edx and ecx registers
    printf("Feature Bits (EDX): 0x%08x\n", cpuid_info.edx.flags);
    printf("Feature Bits (ECX): 0x%08x\n", cpuid_info.ecx.flags);

    xbox_pci_init();

#if (0)
    io_output_byte(0x2E, 0x55);
    io_output_byte(0x2E, 0x26);
    io_output_byte(0x61, 0xff);
    io_output_byte(0x92, 0x01);
    io_output_byte(0xcf9, 0x0); // Reset Port
    io_output_byte(0x43, 0x36); // Timer 0 (system time): mode 3
    io_output_byte(0x40, 0xFF); // 18.2Hz (1.19318MHz/65535)
    io_output_byte(0x40, 0xFF);
    io_output_byte(0x43, 0x54); // Timer 1 (ISA refresh): mode 2
    io_output_byte(0x41, 18);   // 64KHz (1.19318MHz/18)
    io_output_byte(0x00, 0);    // clear base address 0
    io_output_byte(0x00, 0);
    io_output_byte(0x01, 0); // clear count 0
    io_output_byte(0x01, 0);
    io_output_byte(0x02, 0); // clear base address 1
    io_output_byte(0x02, 0);
    io_output_byte(0x03, 0); // clear count 1
    io_output_byte(0x03, 0);
    io_output_byte(0x04, 0); // clear base address 2
    io_output_byte(0x04, 0);
    io_output_byte(0x05, 0); // clear count 2
    io_output_byte(0x05, 0);
    io_output_byte(0x06, 0); // clear base address 3
    io_output_byte(0x06, 0);
    io_output_byte(0x07, 0); // clear count 3
    io_output_byte(0x07, 0);
    io_output_byte(0x0B, 0x40); // set channel 0 to single mode, verify transfer
    io_output_byte(0x0B, 0x41); // set channel 1 to single mode, verify transfer
    io_output_byte(0x0B, 0x42); // set channel 2 to single mode, verify transfer
    io_output_byte(0x0B, 0x43); // set channel 3 to single mode, verify transfer
    io_output_byte(0x08, 0);    // enable controller
    io_output_byte(0xC0, 0);    // clear base address 0
    io_output_byte(0xC0, 0);
    io_output_byte(0xC2, 0); // clear count 0
    io_output_byte(0xC2, 0);
    io_output_byte(0xC4, 0); // clear base address 1
    io_output_byte(0xC4, 0);
    io_output_byte(0xC6, 0); // clear count 1
    io_output_byte(0xC6, 0);
    io_output_byte(0xC8, 0); // clear base address 2
    io_output_byte(0xC8, 0);
    io_output_byte(0xCA, 0); // clear count 2
    io_output_byte(0xCA, 0);
    io_output_byte(0xCC, 0); // clear base address 3
    io_output_byte(0xCC, 0);
    io_output_byte(0xCE, 0); // clear count 3
    io_output_byte(0xCE, 0);
    io_output_byte(0xD6, 0xC0); // set channel 0 to cascade mode
    io_output_byte(0xD6, 0xC1); // set channel 1 to single mode, verify transfer
    io_output_byte(0xD6, 0xC2); // set channel 2 to single mode, verify transfer
    io_output_byte(0xD6, 0xC3); // set channel 3 to single mode, verify transfer
    io_output_byte(0xD0, 0);    // enable controller
    io_output_byte(0x0E, 0);    // enable DMA0 channels
    io_output_byte(0xD4, 0);    // clear chain 4 mask
#endif

    // Write the default handlers to the IDT
    //xbox_interrupt_configure();

    // Enable the 8259 Interrupt Controllers (Master and Slave)
    // All IRQs initially masked except IRQ2 (To ensure PIC2 IRQs work)
    io_output_byte(XBOX_PIC1_COMMAND_PORT, ICW1_INIT | ICW1_INTERVAL4 | ICW1_ICW4);
    io_output_byte(XBOX_PIC1_DATA_PORT, XBOX_PIC1_BASE_VECTOR_ADDRESS);
    io_output_byte(XBOX_PIC1_DATA_PORT, ICW3_MASTER_SLAVE);
    io_output_byte(XBOX_PIC1_DATA_PORT, ICW4_8086_MODE);
    io_output_byte(XBOX_PIC1_DATA_PORT, ~(1 << XBOX_PIC_SLAVE_IRQ));

    io_output_byte(XBOX_PIC2_COMMAND_PORT, ICW1_INIT | ICW1_INTERVAL4 | ICW1_ICW4);
    io_output_byte(XBOX_PIC2_DATA_PORT, XBOX_PIC2_BASE_VECTOR_ADDRESS);
    io_output_byte(XBOX_PIC2_DATA_PORT, ICW3_SLAVE_2);
    io_output_byte(XBOX_PIC2_DATA_PORT, ICW4_8086_MODE);
    io_output_byte(XBOX_PIC2_DATA_PORT, OCW1_MASK_ALL);

    // Enable PIT timer at 1kHz
    io_output_byte(XBOX_PIT_COMMAND_PORT, PIT_ACCESS_LOHIBYTE | PIT_MODE_SQUARE_WAVE | (XBOX_PIT_CHANNEL0 & 0x0F));
    uint16_t diviser = PIC_TIMER_FREQ / 1000;
    io_output_byte(XBOX_PIT_CHANNEL0, diviser & 0xFF);
    io_output_byte(XBOX_PIT_CHANNEL0, (diviser >> 8) & 0xFF);

    // Disable APIC so PIC IRQs work
    ia32_apic_base_register apic_base;
    cpu_read_msr64(MSR_IA32_APIC_BASE, &apic_base.flags);
    apic_base.apic_global_enable = 0;
    cpu_write_msr64(MSR_IA32_APIC_BASE, apic_base.flags);
    mmio_output_dword(XBOX_APIC_BASE + APIC_LVT_LINT0, 0x00000700);
    mmio_output_dword(XBOX_APIC_BASE + APIC_SIV, 0x00000000);

    //?
    // xbox_smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, 0x1A, 0x01);
    // xbox_smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, 0x1B, 0x04);
    // xbox_smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, 0x19, 0x01);

    __asm__("sti");

    const int width = 640;
    const int height = 480;
    const int bpp = 4;

    uint8_t *fb = malloc(width * height * bpp);
    memset(fb, 0, width * height * bpp);
    assert(fb);
    fb = (uint8_t *)(0xF0000000 | (intptr_t)fb);
    if (bpp == 2) {
        fill_rect_rgb565(fb, width, height, 0, 0, width, height, 0xFF111111);
        fill_rect_rgb565(fb, width, height, 0, 0, 100, 100, 0xFFFF0000);
        fill_rect_rgb565(fb, width, height, width - 100, 0, 100, 100, 0xFF00FF00);
        fill_rect_rgb565(fb, width, height, 0, height - 100, 100, 100, 0xFF0000FF);
        fill_rect_rgb565(fb, width, height, width - 100, height - 100, 100, 100, 0xFFFFFFFF);
    } else {
        fill_rect_argb8888(fb, width, height, 0, 0, width, height, 0xFF111111);
        fill_rect_argb8888(fb, width, height, 0, 0, 100, 100, 0xFFFF0000);
        fill_rect_argb8888(fb, width, height, width - 100, 0, 100, 100, 0xFF00FF00);
        fill_rect_argb8888(fb, width, height, 0, height - 100, 100, 100, 0xFF0000FF);
        fill_rect_argb8888(fb, width, height, width - 100, height - 100, 100, 100, 0xFFFFFFFF);
    }
    __asm__ __volatile__("wbinvd" ::: "memory");

    xbox_video_init(0x04010101, (bpp == 2) ? RGB565 : ARGB8888, fb);

    // apply_all_video_modes(fb);
    xbox_led_output(XLED_GREEN, XLED_GREEN, XLED_GREEN, XLED_GREEN);

    uint8_t temp1, temp2;
    xbox_smbus_input_byte(XBOX_SMBUS_ADDRESS_TEMP, 0x00, &temp1);
    xbox_smbus_input_byte(XBOX_SMBUS_ADDRESS_TEMP, 0x01, &temp2);
    printf("Temperature 1: %d\n", temp1);
    printf("Temperature 2: %d\n", temp2);

    xbox_led_output(XLED_ORANGE, XLED_ORANGE, XLED_ORANGE, XLED_ORANGE);

    __asm__("jmp main");
}
