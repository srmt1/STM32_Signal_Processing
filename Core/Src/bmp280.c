/*
 * bmp280.c
 *
 *  Created on: Oct 20, 2026
 *      Author: s
 */


#include "bmp280.h"

#define BMP280_I2C_ADDR       (0x77 << 1)
#define BMP280_REG_CHIP_ID    0xD0

HAL_StatusTypeDef BMP280_ReadChipID(I2C_HandleTypeDef *hi2c,
                                    uint8_t *chip_id)
{
    return HAL_I2C_Mem_Read(hi2c,
                            BMP280_I2C_ADDR,
                            BMP280_REG_CHIP_ID,
                            I2C_MEMADD_SIZE_8BIT,
                            chip_id,
                            1,
                            100);
}
