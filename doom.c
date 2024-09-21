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

int doom_entry()
{
    char *args[] = {"doom", "-iwad", "doom1.wad", "-file", "doom1.wad", NULL};
    int argc = 5;

    doom_set_print(dooom_printf);
    doom_set_malloc(dooom_malloc, dooom_free);
    doom_init(argc, args, 0);
    while (true) {
        printf("doom_update\n");
        doom_update();

        xbox_video_set_option(XBOX_VIDEO_OPTION_FRAMEBUFFER, (uint32_t *)doom_get_framebuffer(4));
    }
    return 0;
}