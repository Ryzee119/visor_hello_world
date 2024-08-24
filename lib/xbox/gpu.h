#ifndef GPU_H
#define GPU_H

#include "xbox.h"
#include "video.h"

void xbox_gpu_init(uint32_t mode_coding, framebuffer_format_t format, void *frame_buffer);

#endif