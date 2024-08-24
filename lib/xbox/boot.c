#include "xbox.h"

void main(void);

#ifndef XBOX_DISABLE_STDIO_HOOK
// Hook printf to output to serial port
static int _putc(char c, FILE *file)
{
    (void)file;
    xbox_serial_putchar(c);
    return c;
}

static FILE __stdio = FDEV_SETUP_STREAM(_putc, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdin = &__stdio;
__strong_reference(stdin, stdout);
__strong_reference(stdin, stderr);
#endif

static void boot_pic_challenge_response()
{
    uint8_t bC, bD, bE, bF;
    uint8_t status;

    status = xbox_smbus_input(XBOX_SMBUS_ADDRESS_SMC, 0x1C, &bC, 1);
    if (status < 0) {
        return;
    }

    status = xbox_smbus_input(XBOX_SMBUS_ADDRESS_SMC, 0x1D, &bD, 1);
    if (status < 0) {
        return;
    }

    status = xbox_smbus_input(XBOX_SMBUS_ADDRESS_SMC, 0x1E, &bE, 1);
    if (status < 0) {
        return;
    }

    status = xbox_smbus_input(XBOX_SMBUS_ADDRESS_SMC, 0x1F, &bF, 1);
    if (status < 0) {
        return;
    }

    uint8_t b1 = 0x33;
    uint8_t b2 = 0xED;
    uint8_t b3 = ((bC << 2) ^ (bD + 0x39) ^ (bE >> 2) ^ (bF + 0x63));
    uint8_t b4 = ((bC + 0x0b) ^ (bD >> 2) ^ (bE + 0x1b));
    uint8_t n = 4;

    while (n--) {
        b1 += b2 ^ b3;
        b2 += b1 ^ b4;
    }

    uint16_t w = (b2 << 8) | b1;

    xbox_smbus_output(XBOX_SMBUS_ADDRESS_SMC, 0x1C, 0x2000 | ((w >> 0) & 0xFF), 2);
    xbox_smbus_output(XBOX_SMBUS_ADDRESS_SMC, 0x1D, 0x2100 | ((w >> 8) & 0xFF), 2);
    return;
}

void boot(void)
{
    xbox_led_output(XLED_RED, XLED_RED, XLED_RED, XLED_RED);
    boot_pic_challenge_response();
    xbox_led_output(XLED_ORANGE, XLED_ORANGE, XLED_ORANGE, XLED_ORANGE);

    xbox_serial_init();
    printf("Hello, World!\n");

    // SMC Challenge

    xbox_pci_init();

    // pre_apic_callback();

// You can disable the APIC measurement which will fall back to the hardcoded 733Mhz.
// This will improve bootup time slightly but will not account for modified FSB speeds.
#ifndef XBOX_DISABLE_APIC_MEASUREMENT
    xbox_timer_measure_apic_frequency();
#endif

    void *fb = malloc(1280 * 720 * 4);
    memset(fb, 0x77, 1280 * 720 * 4);
    xbox_gpu_init(0x880B0A02, ARGB8888, fb);
    xbox_led_output(XLED_GREEN, XLED_GREEN, XLED_GREEN, XLED_GREEN);

    xbox_smbus_output(XBOX_SMBUS_ADDRESS_SMC, 0x00, 0x1234, 2);

    uint8_t temp1, temp2;
    xbox_smbus_input(XBOX_SMBUS_ADDRESS_TEMP, 0x00, &temp1, 1);
    xbox_smbus_input(XBOX_SMBUS_ADDRESS_TEMP, 0x01, &temp2, 1);
    printf("Temperature 1: %d\n", temp1);
    printf("Temperature 2: %d\n", temp2);

    xbox_led_reset();
    main();
}
