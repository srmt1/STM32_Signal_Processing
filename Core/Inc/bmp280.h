/*
 * bmp280.h
 *
 *  Created on: Oct 20, 2026
 *      Author: s
 */

#ifndef INC_BMP280_H_
#define INC_BMP280_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

HAL_StatusTypeDef BMP280_ReadChipID(I2C_HandleTypeDef *hi2c,
                                    uint8_t *chip_id);

#endif /* INC_BMP280_H_ */
