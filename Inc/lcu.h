/*
 * lcu.h
 *
 *  Created on: Jan 13, 2024
 *      Author: r6djo
 */

#ifndef INC_LCU_H_
#define INC_LCU_H_

#include "main.h"
#include "modbus.h"

void light_level_update(MODBUS_registers *);
uint16_t exp_running_average(uint16_t);
void LCU_update(MODBUS_registers *registers);

#endif /* INC_LCU_H_ */
