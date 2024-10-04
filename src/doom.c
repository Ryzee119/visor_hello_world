#define DOOM_IMPLEMENTATION
#include "main.h"

// Wtf doom
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wsometimes-uninitialized"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wparentheses"
#include <PureDOOM/PureDOOM.h>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop


static SemaphoreHandle_t doom_logic_mutex;
static const char *cached_wad_path = NULL;
static uint8_t *cached_wad_data = NULL;
static uint32_t cached_wad_size = 0;
static uint32_t cached_wad_cursor = 0;

uint8_t doom_initd = 0;
int16_t doom_rightx = 0;

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

static void *dooom_open(const char *filename, const char *mode)
{
    if (strcmp(filename, cached_wad_path) == 0) {
        cached_wad_cursor = 0;
        return cached_wad_data;
    }
    return fopen(filename, mode);
}

static void dooom_close(void *handle)
{
    if (handle == cached_wad_data) {
        return;
    }
    fclose((FILE *)handle);
}

static int dooom_read(void *handle, void *buf, int count)
{
    if (handle == cached_wad_data) {
        memcpy(buf, &cached_wad_data[cached_wad_cursor], count);
        cached_wad_cursor += count;
        return count;
    }
    return fread(buf, 1, count, (FILE *)handle);
}

static int dooom_write(void *handle, const void *buf, int count)
{
    if (handle == cached_wad_data) {
        return -1;
    }
    return fwrite(buf, 1, count, (FILE *)handle);
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
    return fseek(handle, offset, origin);
}

static int dooom_tell(void *handle)
{
    if (handle == cached_wad_data) {
        return cached_wad_cursor;
    }
    return ftell((FILE *)handle);
}

static int dooom_eof(void *handle)
{
    return feof((FILE *)handle);
}

void *doom_memcpy(void *destination, const void *source, int num)
{
    return memcpy(destination, source, num);
}

void doom_memset(void *ptr, int value, int num)
{
    memset(ptr, value, num);
}

char *dooom_getenv(const char *var)
{
    if (strcmp(var, "DOOMWADDIR") == 0) {
        return "0:";
    }
    return 0;
}

int doom_entry(const char *wad_path)
{
    static char *args[] = {"doom", "-iwad", NULL, NULL};
    int argc = 3;
    args[2] = (char *)wad_path;

    // Read in WAD file to RAM. Doing it in a big block here seems to work a lot better
    printf_r("[DOOM] Reading WAD file: %s\n", wad_path);
    FILE *fp = fopen(wad_path, "rb");
    if (fp == NULL) {
        printf_r("[DOOM] Could not open WAD file\n");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    cached_wad_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    cached_wad_data = pvPortMalloc(cached_wad_size);

    int chunk_size = 256 * 1024;
    int i = 0;
    while (chunk_size == 256 * 1024) {
        chunk_size = fread(&cached_wad_data[i], 1, chunk_size, fp);
        i += chunk_size;
        printf_r("i=%d, %d\n", i, chunk_size);
    }
    printf_r("\n");
    fclose(fp);

    cached_wad_path = strdup(wad_path);

    doom_set_print(dooom_printf);
    doom_set_malloc(dooom_malloc, dooom_free);
    doom_set_gettime(dooom_gettime);
    doom_set_file_io(dooom_open, dooom_close, dooom_read, dooom_write, dooom_seek, dooom_tell, dooom_eof);
    doom_set_getenv(dooom_getenv);

    printf_r("[DOOM] Initializing...\n");
    doom_init(argc, args, 0);
    doom_initd = 1;

    doom_logic_mutex = xSemaphoreCreateMutex();
    xTaskCreate(doom_sound_task, "DoomSound", configMINIMAL_STACK_SIZE, &doom_logic_mutex, THREAD_PRIORITY_NORMAL, NULL);

    uint32_t *final_screen_buffer = pvPortMalloc(640 * 480 * 4);
    final_screen_buffer = (uint32_t *)(0xF0000000 | (intptr_t)final_screen_buffer);
    while (1) {
        doom_mouse_move(doom_rightx / 256, 0);
        xSemaphoreTake(doom_logic_mutex, portMAX_DELAY);
        doom_update();
        xSemaphoreGive(doom_logic_mutex);

        // The doom framebuffer is palette indexed at 320x200.
        // We scale by 2 so it is 640x400 and apply the palette to convert to ARGB8888.
        extern unsigned char screen_palette[256 * 3];
        uint8_t *indexed_framebuffer = (uint8_t *)doom_get_framebuffer(1);
        uint32_t *screen_buffer_ptr = final_screen_buffer;

        screen_buffer_ptr += (SCREENWIDTH * 2) * 40; // 40 to shift down so it is centered
        for (int pixel = 0; pixel < SCREENWIDTH * SCREENHEIGHT; pixel++) {
            uint32_t index = indexed_framebuffer[pixel] * 3;

            uint32_t argb = 0xff000000 | (screen_palette[index] << 16) | (screen_palette[index + 1] << 8) |
                            screen_palette[index + 2];

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
