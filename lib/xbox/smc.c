// SPDX-License-Identifier: CC0-1.0
// Copyright (c) 2024 Ryzee119

#include "xbox.h"

int8_t xbox_smc_get_tray_state(xbox_tray_state_t *tray_state)
{
    return smbus_input_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_GET_TRAY_STATE, tray_state);
}

int8_t xbox_smc_get_avpack(xbox_av_pack_t *av_pack)
{
    return smbus_input_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_GET_AVPACK, av_pack);
}

int8_t xbox_smc_set_tray_open(uint8_t open)
{
    return smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_SET_TRAY_CLOSED, (open) ? 0 : 1);
}

int8_t xbox_smc_set_power(xbox_power_control_t power)
{
    return smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_SET_POWER, power);
}

void xbox_smc_get_version(uint8_t version[4])
{
    // Reset counter
    smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_GET_VERSION, 0);

    for (uint8_t i = 0; i < 3; i++) {
        smbus_input_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_GET_VERSION, &version[i]);
    }
    version[3] = 0;
}
