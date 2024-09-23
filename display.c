#include "main.h"
#define DISPLAY_BG_COLOR 0x10

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
static const uint8_t MARGIN = 20;

void display_init()
{
    const int width = 640;
    const int height = 480;
    const int bpp = 4;

    uint8_t *fb = malloc(width * height * bpp);
    memset(fb, DISPLAY_BG_COLOR, width * height * bpp);
    fb = (uint8_t *)(0xF0000000 | (intptr_t)fb);
    xbox_video_init(0x04010101, (bpp == 2) ? RGB565 : ARGB8888, fb);

    cpuid_eax_01 cpuid_info;
    cpu_read_cpuid(CPUID_VERSION_INFO, &cpuid_info.eax.flags, &cpuid_info.ebx.flags, &cpuid_info.ecx.flags, &cpuid_info.edx.flags);

    XPRINTF("[CPU] Family: %d\n", cpuid_info.eax.family_id);
    XPRINTF("[CPU] Model: %d\n", cpuid_info.eax.model);
    XPRINTF("[CPU] Stepping: %d\n", cpuid_info.eax.stepping_id);
    XPRINTF("[CPU] Type: %d\n", cpuid_info.eax.processor_type);
    XPRINTF("[CPU] Extended Family: %d\n", cpuid_info.eax.extended_family_id);
    XPRINTF("[CPU] Extended Model: %d\n", cpuid_info.eax.extended_model_id);
    XPRINTF("[CPU] Feature Bits (EDX): 0x%08x\n", cpuid_info.edx.flags);
    XPRINTF("[CPU] Feature Bits (ECX): 0x%08x\n", cpuid_info.ecx.flags);

    uint8_t temp1, temp2;
    xbox_smbus_input_byte(XBOX_SMBUS_ADDRESS_TEMP, 0x00, &temp1);
    xbox_smbus_input_byte(XBOX_SMBUS_ADDRESS_TEMP, 0x01, &temp2);
    XPRINTF("[SYS] CPU: %d C\n", temp1);
    XPRINTF("[SYS] MB: %d C\n", temp2);
}

void display_clear()
{
    const display_information_t *display = xbox_video_get_display_information();
    if (!display->frame_buffer) {
        return;
    }
    memset(display->frame_buffer, DISPLAY_BG_COLOR, display->width * display->height * display->bytes_per_pixel);
    cursor_x = MARGIN;
    cursor_y = MARGIN;
}

void display_write_char(const char c)
{
    const display_information_t *display = xbox_video_get_display_information();
    if (!display->frame_buffer) {
        return;
    }

    if (display->frame_buffer) {
        if (c == '\n') {
            cursor_x = MARGIN;
            cursor_y += UNSCII_FONT_HEIGHT;
        } else if (c == '\r') {
            cursor_x = MARGIN;
        } else {
            const uint8_t *glyph = unscii_16 + (c * ((UNSCII_FONT_WIDTH + 7) / 8) * UNSCII_FONT_HEIGHT);
            for (int h = 0; h < UNSCII_FONT_HEIGHT; h++) {
                uint8_t mask = 0x80;
                for (int w = 0; w < UNSCII_FONT_WIDTH; w++) {
                    if (*glyph & (mask >>= 1)) {
                        // Draw pixel inverted colour to what is already there
                        if (display->bytes_per_pixel == 2) {
                            uint16_t *fb16 = display->frame_buffer;
                            uint16_t *pixel = &fb16[(cursor_y + h) * display->width + cursor_x + w];
                            *pixel = 0xFFFF;
                        } else {
                            uint32_t *fb32 = display->frame_buffer;
                            uint32_t *pixel = &fb32[(cursor_y + h) * display->width + cursor_x + w];
                            *pixel = 0xFFFFFFFF;
                        }
                    }
                }
                glyph++; // Next line of glyph
            }
            cursor_x += UNSCII_FONT_WIDTH;
        }

        if (cursor_x + UNSCII_FONT_WIDTH >= (display->width - MARGIN)) {
            cursor_x = MARGIN;
            cursor_y += UNSCII_FONT_HEIGHT;
        }

        if (cursor_y + UNSCII_FONT_HEIGHT >= (display->height - MARGIN)) {
            display_clear();
        }
    }
}
