#ifndef VIDEO_H
#define VIDEO_H

#include "video.h"
#include "xbox.h"

typedef struct display_information
{
    uint16_t vdisplay_end;
    uint16_t vtotal;
    uint16_t vcrtc;
    uint16_t vsync_start;
    uint16_t vsync_end;
    uint16_t vvalid_start;
    uint16_t vvalid_end;
    uint16_t hdisplay_end;
    uint16_t htotal;
    uint16_t hcrtc;
    uint16_t hsync_start;
    uint16_t hsync_end;
    uint16_t hvalid_start;
    uint16_t hvalid_end;
} display_information_t;

enum
{
    XBOX_VIDEO_OPTION_VIDEO_ENABLE,
    XBOX_VIDEO_OPTION_VIDEO_FLICKER_FILTER,
    XBOX_VIDEO_OPTION_VIDEO_SOFTEN_FILTER,
    XBOX_VIDEO_OPTION_FRAMEBUFFER,
};
typedef uint8_t xbox_video_option_t;

enum
{
    ARGB1555,
    RGB565,
    ARGB8888,
};
typedef uint8_t xbox_framebuffer_format_t;

#define VIDEO_RETURN_SUCCESS 0
#define VIDEO_RETURN_ERROR   -1

typedef struct _VIDEO_MODE_SETTING
{
    uint32_t dwMode;
    int width;
    int height;
    int refresh;
    uint32_t dwStandard;
    uint32_t dwFlags;
} VIDEO_MODE_SETTING;
#define AV_PACK_NONE     0x00000000
#define AV_PACK_STANDARD 0x00000001
#define AV_PACK_RFU      0x00000002
#define AV_PACK_SCART    0x00000003
#define AV_PACK_HDTV     0x00000004
#define AV_PACK_VGA      0x00000005
#define AV_PACK_SVIDEO   0x00000006
#define VIDEO_REGION_NTSCM 0x00000100
#define VIDEO_REGION_NTSCJ 0x00000200
#define VIDEO_REGION_PAL   0x00000300


void xbox_video_init(uint32_t mode_coding, xbox_framebuffer_format_t format, void *frame_buffer);
uint8_t xbox_video_set_option(xbox_video_option_t option, uint32_t *parameter);
void xbox_video_get_display_information(display_information_t *information);
const VIDEO_MODE_SETTING *video_get_settings(uint32_t mode_coding);
#endif


