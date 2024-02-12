/*
 * lcu.c
 *
 *  Created on: Jan 13, 2024
 *      Author: r6djo
 */

#include "lcu.h"

void light_level_update(MODBUS_registers *registers)
{
    static uint32_t previous_millis = 0;
    const uint32_t interval = 3000;
    uint32_t current_millis = HAL_GetTick();
    if (current_millis - previous_millis >= interval)
    {
        previous_millis = current_millis;
        uint16_t adc = get_light_level();
        adc = exp_running_average(adc);
        registers->AI[0] = adc;
#ifdef DEBUG
        // UART_Printf("ADC: %hu [ 0x%04X ]\n", adc / 655, adc);
        UART_Printf("Fil ADC: 0x%04X\n", adc);
#endif // DEBUG
    }
}

uint16_t exp_running_average(uint16_t new_val)
{
    static uint16_t fil_val = 0;
    fil_val += (new_val - fil_val) >> 3;
    return fil_val;
}
