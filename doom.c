#define DOOM_IMPLEMENTATION
#include "main.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wsometimes-uninitialized"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wdeprecated-non-prototype"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wparentheses"
#include <PureDOOM/PureDOOM.h>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

static void dooom_printf(const char *str)
{
    printf_r("%s", str);
}

static void *dooom_malloc(int size)
{
    return pvPortMalloc(size);
}

static void dooom_free(void *ptr)
{
    vPortFree(ptr);
}

static void dooom_gettime(int *sec, int *usec)
{
    static uint32_t base_ms = 0;
    if (base_ms == 0) {
        base_ms = pdTICKS_TO_MS(xTaskGetTickCount());
    }
    int ms = pdTICKS_TO_MS(xTaskGetTickCount()) - base_ms;
    *sec = ms / 1000;
    *usec = (ms % 1000) * 1000;
}

static const char *cached_wad_path = NULL;
static uint8_t *cached_wad_data = NULL;
static uint32_t cached_wad_size = 0;
static uint32_t cached_wad_cursor = 0;
static void *dooom_open(const char *filename, const char *mode)
{
    if (strcmp(filename, cached_wad_path) == 0) {
        cached_wad_cursor = 0;
        return cached_wad_data;
    }

    FIL *fp = pvPortMalloc(sizeof(FIL));
    BYTE fatfs_mode = 0;
    while (*mode) {
        if (*mode == 'r') {
            fatfs_mode |= FA_READ;
        } else if (*mode == 'w') {
            fatfs_mode |= FA_WRITE | FA_CREATE_ALWAYS;
        } else if (*mode == 'a') {
            fatfs_mode |= FA_WRITE | FA_OPEN_APPEND;
        } else if (*mode == '+') {
            fatfs_mode |= FA_READ | FA_WRITE;
        } else if (*mode == 'x') {
            fatfs_mode |= FA_CREATE_NEW;
        }
        mode++;
    }

    if (f_open(fp, filename + 2, fatfs_mode) != FR_OK) {
        vPortFree(fp);
        return NULL;
    } else {
        printf_r("dooom_open: %s, success, %2x\n", filename, fatfs_mode);
    }
    return fp;
}

static void dooom_close(void *handle)
{
    if (handle == cached_wad_data) {
        return;
    }
    f_close(handle);
    vPortFree(handle);
}

static int dooom_read(void *handle, void *buf, int count)
{
    if (handle == cached_wad_data) {
        memcpy(buf, &cached_wad_data[cached_wad_cursor], count);
        cached_wad_cursor += count;
        return count;
    }
    UINT bytes_read;
    if (f_read((FIL *)handle, buf, count, &bytes_read) != FR_OK) {
        printf("dooom_read, failed\n");
        return -1;
    }
    return bytes_read;
}

static int dooom_write(void *handle, const void *buf, int count)
{
    if (handle == cached_wad_data) {
        return -1;
    }
    UINT bytes_written;
    if (f_write(handle, buf, count, &bytes_written) != FR_OK) {
        printf("dooom_write, failed\n");
        return -1;
    }
    return bytes_written;
}

static int dooom_seek(void *handle, int offset, doom_seek_t origin)
{
    if (handle == cached_wad_data) {
        if (origin == DOOM_SEEK_SET) {
            cached_wad_cursor = offset;
        } else if (origin == DOOM_SEEK_CUR) {
            cached_wad_cursor += offset;
        } else if (origin == DOOM_SEEK_END) {
            cached_wad_cursor = cached_wad_size - offset;
        }
        return 0;
    }
    if (f_lseek(handle, offset) != FR_OK) {
        return -1;
    }
    return 0;
}

static int dooom_tell(void *handle)
{
    if (handle == cached_wad_data) {
        return cached_wad_cursor;
    }
    return f_tell((FIL *)handle);
}

static int dooom_eof(void *handle)
{
    if (handle == cached_wad_data) {
        return cached_wad_cursor >= cached_wad_size;
    }
    return f_eof((FIL *)handle);
}

void *doom_memcpy(void *destination, const void *source, int num)
{
    return memcpy(destination, source, num);
}

void doom_memset(void *ptr, int value, int num)
{
    memset(ptr, value, num);
}

void dooom_new_input(uint16_t buttons, int16_t lx, int16_t ly)
{
    static uint16_t old_buttons = 0;
    uint16_t buttons_changed = buttons ^ old_buttons;

    if (buttons_changed) {
        // printf_r("buttons: %04x, lx: %d, ly: %d\n", buttons, lx, ly);
    } else {
        return;
    }

    vPortEnterCritical();

    if (buttons_changed & XINPUT_GAMEPAD_A) {
        if (buttons & XINPUT_GAMEPAD_A) {
            doom_key_down(DOOM_KEY_CTRL);
        } else {
            doom_key_up(DOOM_KEY_CTRL);
        }
    }
    if (buttons_changed & XINPUT_GAMEPAD_X) {
        if (buttons & XINPUT_GAMEPAD_X) {
            doom_key_down(DOOM_KEY_SPACE);
        } else {
            doom_key_up(DOOM_KEY_SPACE);
        }
    }
    if (buttons_changed & XINPUT_GAMEPAD_B) {
        if (buttons & XINPUT_GAMEPAD_B) {
            doom_key_down(DOOM_KEY_SHIFT);
        } else {
            doom_key_up(DOOM_KEY_SHIFT);
        }
    }
    if (buttons_changed & XINPUT_GAMEPAD_DPAD_UP) {
        if (buttons & XINPUT_GAMEPAD_DPAD_UP) {
            doom_key_down(DOOM_KEY_UP_ARROW);
        } else {
            doom_key_up(DOOM_KEY_UP_ARROW);
        }
    }
    if (buttons_changed & XINPUT_GAMEPAD_DPAD_DOWN) {
        if (buttons & XINPUT_GAMEPAD_DPAD_DOWN) {
            doom_key_down(DOOM_KEY_DOWN_ARROW);
        } else {
            doom_key_up(DOOM_KEY_DOWN_ARROW);
        }
    }
    if (buttons_changed & XINPUT_GAMEPAD_DPAD_LEFT) {
        if (buttons & XINPUT_GAMEPAD_DPAD_LEFT) {
            doom_key_down(DOOM_KEY_LEFT_ARROW);
        } else {
            doom_key_up(DOOM_KEY_LEFT_ARROW);
        }
    }
    if (buttons_changed & XINPUT_GAMEPAD_DPAD_RIGHT) {
        if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) {
            doom_key_down(DOOM_KEY_RIGHT_ARROW);
        } else {
            doom_key_up(DOOM_KEY_RIGHT_ARROW);
        }
    }
    if (buttons_changed & XINPUT_GAMEPAD_START) {
        if (buttons & XINPUT_GAMEPAD_START) {
            doom_key_down(DOOM_KEY_ENTER);
        } else {
            doom_key_up(DOOM_KEY_ENTER);
        }
    }

    vPortExitCritical();
    old_buttons = buttons;
}

int doom_entry(const char *wad_path)
{
    static char *args[] = {"doom", "-iwad", NULL, NULL};
    int argc = 3;
    args[2] = (char *)wad_path;

    // Read in WAD file to RAM. Doing it in a big block here seems to work a lot better
    printf_r("[DOOM] Reading WAD file: %s\n", wad_path);
    {
        FIL wad_file;
        if (f_open(&wad_file, wad_path, FA_READ) != FR_OK) {
            printf_r("Failed to open WAD file\n");
            return -1;
        }

        cached_wad_size = f_size(&wad_file);
        cached_wad_data = pvPortMalloc(cached_wad_size);
        const uint32_t chunk_size = cached_wad_size / 16;
        for (uint32_t i = 0; i < cached_wad_size; i += chunk_size) {
            uint32_t remaining = cached_wad_size - i;
            uint32_t read_len = remaining > chunk_size ? chunk_size : remaining;
            if (f_read(&wad_file, &cached_wad_data[i], read_len, NULL) != FR_OK) {
                printf_r("Failed to read WAD file\n");
                return -1;
            }
            printf(".");
        }
        f_close(&wad_file);
    }
    printf_r("WAD file read, size: %d\n", cached_wad_size);

    cached_wad_path = pvPortMalloc(strlen(wad_path) + 3);
    sprintf((char *)cached_wad_path, "./%s", wad_path);

    doom_set_print(dooom_printf);
    doom_set_malloc(dooom_malloc, dooom_free);
    doom_set_gettime(dooom_gettime);
    doom_set_file_io(dooom_open, dooom_close, dooom_read, dooom_write, dooom_seek, dooom_tell, dooom_eof);

    printf_r("[DOOM] Initializing...\n");
    doom_init(argc, args, 0);

    uint32_t *final_screen_buffer = pvPortMalloc(640 * 480 * 4);
    final_screen_buffer = (uint32_t *)(0xF0000000 | (intptr_t)final_screen_buffer);
    while (1) {
        doom_update();

        // The doom framebuffer is palette indexed at 320x200.
        // We scale by 2 so it is 640x400 and apply the palette  to convert to aARGB8888.
        extern unsigned char screen_palette[256 * 3];
        uint8_t *indexed_framebuffer = (uint8_t *)doom_get_framebuffer(1);
        uint32_t *screen_buffer_ptr = final_screen_buffer;

        screen_buffer_ptr += (SCREENWIDTH * 2) * 40; // 40 to shift down so it is centered
        for (int pixel = 0; pixel < SCREENWIDTH * SCREENHEIGHT; pixel++) {
            uint32_t index = indexed_framebuffer[pixel] * 3;

            uint32_t argb = 0xff000000 | (screen_palette[index] << 16) | (screen_palette[index + 1] << 8) | screen_palette[index + 2];

            *screen_buffer_ptr++ = argb;
            *screen_buffer_ptr++ = argb;

            if ((pixel + 1) % SCREENWIDTH == 0) {
                memcpy(screen_buffer_ptr, screen_buffer_ptr - SCREENWIDTH * 2, SCREENWIDTH * 2 * 4);
                screen_buffer_ptr += SCREENWIDTH * 2;
            }
        }

        xbox_video_set_option(XBOX_VIDEO_OPTION_FRAMEBUFFER, final_screen_buffer);
        taskYIELD();
    }
    return 0;
}
