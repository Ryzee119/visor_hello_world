#ifndef SMC_H
#define SMC_H

#include "xbox.h"

//  Each time you read that register, the next of the 3 characters is returned. The counter can be reset to the first letter by writing 0x00 to this register
#define XBOX_SMC_GET_VERSION 0x01 // 3 ASCII chars
#define XBOX_SMC_GET_TRAY_STATE 0x03
#define XBOX_SMC_GET_AV_PACK 0x04
#define XBOX_SMC_GET_CPU_TEMP 0x09 // Celcius
#define XBOX_SMC_GET_MB_TEMP 0x0A // Celcius
#define XBOX_SMC_GET_FAN_SPEED 0x10 // 0 to 50 = 0 to 100%
#define XBOX_SMC_GET_IRQ 0x11

#define XBOX_SMC_SET_PWR 0x02 // 0x01 = reset, 0x40 - power cycle, 0x80 - power off
#define XBOX_SMC_SET_FAN_MODE 0x05 // 0 = auto, 1 = manual
#define XBOX_SMC_SET_FAN_SPEED 0x06 // 0 to 50 = 0 to 100%
#define XBOX_SMC_SET_TRAY_CLOSED 0x0C // 0 = open, 1 = load
#define XBOX_SMC_SET_IRQ_EN 0x1A // 1 = enable. Cannot disable

#endif