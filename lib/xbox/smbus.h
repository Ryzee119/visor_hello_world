#ifndef SMBUS_H
#define SMBUS_H

#include "xbox.h"

int8_t xbox_smbus_input(uint8_t address, uint8_t reg, void *data, uint8_t data_len);
int8_t xbox_smbus_output(uint8_t address, uint8_t reg, uint32_t data, uint8_t data_len);

#endif

