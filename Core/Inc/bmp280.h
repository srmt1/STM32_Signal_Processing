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

typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

} BMP280_Calibration;

HAL_StatusTypeDef BMP280_ReadCalibration(
    I2C_HandleTypeDef *hi2c,
    BMP280_Calibration *calibration);

HAL_StatusTypeDef BMP280_ReadTemperature(
    I2C_HandleTypeDef *hi2c,
    const BMP280_Calibration *calibration,
    float *temperature);

HAL_StatusTypeDef BMP280_Init(I2C_HandleTypeDef *hi2c);

HAL_StatusTypeDef BMP280_ReadPressure(
    I2C_HandleTypeDef *hi2c,
    const BMP280_Calibration *calibration,
    float *pressure_hpa);

HAL_StatusTypeDef BMP280_ReadRawData(
    I2C_HandleTypeDef *hi2c,
    uint32_t *raw_pressure,
    uint32_t *raw_temperature);

#endif /* INC_BMP280_H_ */
