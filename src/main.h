#ifndef MAIN_H
#define MAIN_H

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <freertos_irq.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>
#include <timers.h>

#include <fatfs/ff.h>
#include <font/unscii_16.h>
#include <tinyusb/src/tusb.h>
#include <xbox/xbox.h>

#include <fatfs/diskio.h>
#include <tusb_xinput/xinput_host.h>

#define THREAD_PRIORITY_LOWEST  0
#define THREAD_PRIORITY_LOW     1
#define THREAD_PRIORITY_NORMAL  2
#define THREAD_PRIORITY_HIGH    3
#define THREAD_PRIORITY_HIGHEST 4

#if THREAD_PRIORITY_HIGHEST > configMAX_PRIORITIES
#error "THREAD_PRIORITY_HIGHEST is higher than configMAX_PRIORITIES"
#endif

typedef struct fileio_drv
{
    DSTATUS (*disk_status)(BYTE pdrv);
    DSTATUS (*disk_initialize)(BYTE pdrv);
    DRESULT (*disk_read)(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count);
    DRESULT (*disk_write)(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count);
    DRESULT (*disk_ioctl)(BYTE pdrv, BYTE cmd, void *buff);
} fileio_drv_t;

int8_t fileio_register_driver(uint8_t pdrv, fileio_drv_t *driver);
int8_t fileio_unregister_driver(uint8_t pdrv);

int printf_r(const char *format, ...);

void display_init(void);
void display_write_char(const char c);
void display_clear(void);

void usb_init(void);
void interrupts_init(void);

int doom_entry(const char *wad_path);
void dooom_new_input(uint16_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry, uint8_t lt, uint8_t rt);
void doom_sound_task(void *parameters);
#endif
