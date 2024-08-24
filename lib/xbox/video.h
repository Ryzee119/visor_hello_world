#ifndef VIDEO_H
#define VIDEO_H

#include "xbox.h"

enum
{
    ENCODER_CONEXTANT,
    ENCODER_FOCUS,
    ENCODER_XYCLOPS,
};

enum {
    ARGB1555,
    RGB565,
    ARGB8888,
};
typedef uint8_t framebuffer_format_t;

#endif