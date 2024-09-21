#ifndef MAIN_H
#define MAIN_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <freertos_irq.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>
#include <timers.h>

#include <tinyusb/src/tusb.h>
#include <xbox/xbox.h>
#include <fatfs/ff.h>
#include <fatfs/diskio.h>

#define THREAD_PRIORITY_LOWEST  0
#define THREAD_PRIORITY_LOW     1
#define THREAD_PRIORITY_NORMAL  2
#define THREAD_PRIORITY_HIGH    3
#define THREAD_PRIORITY_HIGHEST 4

int printf_r(const char *format, ...);

void usb_init(void);
void interrupts_init(void);
int doom_entry();




#endif

