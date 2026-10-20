/*
 * aht20.h
 *
 *  Created on: Oct 20, 2026
 *      Author: s
 */

#ifndef INC_AHT20_H_
#define INC_AHT20_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

HAL_StatusTypeDef AHT20_Init(I2C_HandleTypeDef *hi2c);

HAL_StatusTypeDef AHT20_Read(I2C_HandleTypeDef *hi2c,
                             float *temperature,
                             float *humidity);

#endif /* INC_AHT20_H_ */
