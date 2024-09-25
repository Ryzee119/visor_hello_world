#include "main.h"
#define DISPLAY_BG_COLOR 0x10

static const uint8_t MARGIN = 20;
static uint32_t cursor_x = MARGIN;
static uint32_t cursor_y = MARGIN;

void display_init()
{
    const int width = 640;
    const int height = 480;
    const int bpp = 4;

    uint8_t *fb = malloc(width * height * bpp);
    memset(fb, DISPLAY_BG_COLOR, width * height * bpp);
    fb = (uint8_t *)(0xF0000000 | (intptr_t)fb);
    xbox_video_init(0x44030307, (bpp == 2) ? RGB565 : ARGB8888, fb);
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
            // To save space we purged the first 32 'non-ascii' characters from the font, so ignore anything below 0x20
            // then offset c by -0x20 to get the correct index into the font
            if (c > 0x7F || c < 0x20) {
                return;
            }
            const uint8_t *glyph = unscii_16 + ((c - 0x20) * ((UNSCII_FONT_WIDTH + 7) / 8) * UNSCII_FONT_HEIGHT);
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

        // Wrap text
        if (cursor_x + UNSCII_FONT_WIDTH >= (display->width - MARGIN)) {
            cursor_x = MARGIN;
            cursor_y += UNSCII_FONT_HEIGHT;
        }

        // New page
        if (cursor_y + UNSCII_FONT_HEIGHT >= (display->height - MARGIN)) {
            display_clear();
        }
    }
}
