/*
 * bmp280.c
 *
 *  Created on: Oct 20, 2026
 *      Author: s
 */


#include "bmp280.h"
#include <stdio.h>

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

#define BMP280_REG_CALIBRATION  0x88

HAL_StatusTypeDef BMP280_ReadCalibration(
    I2C_HandleTypeDef *hi2c,
    BMP280_Calibration *calibration)
{
    HAL_StatusTypeDef status;
    uint8_t data[24];

    status = HAL_I2C_Mem_Read(hi2c,
                              BMP280_I2C_ADDR,
                              BMP280_REG_CALIBRATION,
                              I2C_MEMADD_SIZE_8BIT,
                              data,
                              sizeof(data),
                              100);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Temperature calibration coefficients */
    calibration->dig_T1 = (uint16_t)(
        ((uint16_t)data[1] << 8) | data[0]);

    calibration->dig_T2 = (int16_t)(
        ((uint16_t)data[3] << 8) | data[2]);

    calibration->dig_T3 = (int16_t)(
        ((uint16_t)data[5] << 8) | data[4]);

    /* Pressure calibration coefficients */
    calibration->dig_P1 = (uint16_t)(
        ((uint16_t)data[7] << 8) | data[6]);

    calibration->dig_P2 = (int16_t)(
        ((uint16_t)data[9] << 8) | data[8]);

    calibration->dig_P3 = (int16_t)(
        ((uint16_t)data[11] << 8) | data[10]);

    calibration->dig_P4 = (int16_t)(
        ((uint16_t)data[13] << 8) | data[12]);

    calibration->dig_P5 = (int16_t)(
        ((uint16_t)data[15] << 8) | data[14]);

    calibration->dig_P6 = (int16_t)(
        ((uint16_t)data[17] << 8) | data[16]);

    calibration->dig_P7 = (int16_t)(
        ((uint16_t)data[19] << 8) | data[18]);

    calibration->dig_P8 = (int16_t)(
        ((uint16_t)data[21] << 8) | data[20]);

    calibration->dig_P9 = (int16_t)(
        ((uint16_t)data[23] << 8) | data[22]);

    return HAL_OK;
}

#define BMP280_REG_TEMP_MSB    0xFA

/* Stores the intermediate temperature value needed for pressure compensation. */
static int32_t bmp280_t_fine;

HAL_StatusTypeDef BMP280_ReadTemperature(
    I2C_HandleTypeDef *hi2c,
    const BMP280_Calibration *calibration,
    float *temperature)
{
    HAL_StatusTypeDef status;
    uint8_t data[3];
    int32_t adc_temperature;
    int32_t var1;
    int32_t var2;
    int32_t temp_fine;
    int32_t temp_100;

    /* Read the three raw temperature bytes. */
    status = HAL_I2C_Mem_Read(hi2c,
                              BMP280_I2C_ADDR,
                              BMP280_REG_TEMP_MSB,
                              I2C_MEMADD_SIZE_8BIT,
                              data,
                              sizeof(data),
                              100);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Combine the bytes into the 20-bit ADC measurement. */
    adc_temperature = ((int32_t)data[0] << 12)
                    | ((int32_t)data[1] << 4)
                    | ((int32_t)data[2] >> 4);

    /* Bosch BMP280 integer temperature compensation. */
    var1 = ((((adc_temperature >> 3)
             - ((int32_t)calibration->dig_T1 << 1)))
             * (int32_t)calibration->dig_T2) >> 11;

    var2 = (((((adc_temperature >> 4)
             - (int32_t)calibration->dig_T1)
             * ((adc_temperature >> 4)
             - (int32_t)calibration->dig_T1)) >> 12)
             * (int32_t)calibration->dig_T3) >> 14;

    temp_fine = var1 + var2;

    /* Save t_fine for the later pressure calculation. */
    bmp280_t_fine = temp_fine;

    /* Temperature in units of 0.01 degrees Celsius. */
    temp_100 = (temp_fine * 5 + 128) >> 8;

    *temperature = (float)temp_100 / 100.0f;

    return HAL_OK;
}

#define BMP280_REG_RESET       0xE0
#define BMP280_REG_CTRL_MEAS   0xF4
#define BMP280_REG_CONFIG      0xF5

HAL_StatusTypeDef BMP280_Init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef status;
    uint8_t value;

    /* Reset the BMP280. */
    value = 0xB6;

    status = HAL_I2C_Mem_Write(hi2c,
                               BMP280_I2C_ADDR,
                               BMP280_REG_RESET,
                               I2C_MEMADD_SIZE_8BIT,
                               &value,
                               1,
                               100);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Allow the sensor to complete its reset. */
    HAL_Delay(10);

    /* Configure standby time and filter. */
    value = 0x00;

    status = HAL_I2C_Mem_Write(hi2c,
                               BMP280_I2C_ADDR,
                               BMP280_REG_CONFIG,
                               I2C_MEMADD_SIZE_8BIT,
                               &value,
                               1,
                               100);

    if (status != HAL_OK)
    {
        return status;
    }

    /*
     * Temperature oversampling x2,
     * pressure oversampling x16,
     * normal measurement mode.
     */
    value = 0x57;

    status = HAL_I2C_Mem_Write(hi2c,
                               BMP280_I2C_ADDR,
                               BMP280_REG_CTRL_MEAS,
                               I2C_MEMADD_SIZE_8BIT,
                               &value,
                               1,
                               100);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Allow the sensor to complete its initial measurements. */
    HAL_Delay(100);

    return HAL_OK;
}

#define BMP280_REG_PRESS_MSB  0xF7

HAL_StatusTypeDef BMP280_ReadPressure(
    I2C_HandleTypeDef *hi2c,
    const BMP280_Calibration *calibration,
    float *pressure_hpa)
{
    HAL_StatusTypeDef status;
    uint8_t data[3];
    uint8_t data_check[3];

    int32_t adc_pressure;
    int64_t var1;
    int64_t var2;
    int64_t pressure;

    /* Read the pressure registers for the first time. */
    status = HAL_I2C_Mem_Read(
        hi2c,
        BMP280_I2C_ADDR,
        BMP280_REG_PRESS_MSB,
        I2C_MEMADD_SIZE_8BIT,
        data,
        sizeof(data),
        100);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Wait briefly before reading the same registers again. */
    HAL_Delay(10);

    status = HAL_I2C_Mem_Read(
        hi2c,
        BMP280_I2C_ADDR,
        BMP280_REG_PRESS_MSB,
        I2C_MEMADD_SIZE_8BIT,
        data_check,
        sizeof(data_check),
        100);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Assemble the first 20-bit pressure ADC reading. */
    adc_pressure = ((int32_t)data_check[0] << 12)
                 | ((int32_t)data_check[1] << 4)
                 | ((int32_t)data_check[2] >> 4);

    /*
     * Bosch BMP280 pressure compensation.
     * bmp280_t_fine must have been calculated by
     * BMP280_ReadTemperature() before this function is called.
     */
    var1 = (int64_t)bmp280_t_fine - 128000;

    var2 = var1 * var1 * (int64_t)calibration->dig_P6;
    var2 += (var1 * (int64_t)calibration->dig_P5) << 17;
    var2 += ((int64_t)calibration->dig_P4) << 35;

    var1 = ((var1 * var1 * (int64_t)calibration->dig_P3) >> 8)
         + ((var1 * (int64_t)calibration->dig_P2) << 12);

    var1 = (((((int64_t)1) << 47) + var1)
          * (int64_t)calibration->dig_P1) >> 33;

    if (var1 == 0)
    {
        return HAL_ERROR;
    }

    pressure = 1048576 - adc_pressure;
    pressure = (((pressure << 31) - var2) * 3125) / var1;

    var1 = ((int64_t)calibration->dig_P9
           * (pressure >> 13)
           * (pressure >> 13)) >> 25;

    var2 = ((int64_t)calibration->dig_P8 * pressure) >> 19;

    pressure = ((pressure + var1 + var2) >> 8)
             + (((int64_t)calibration->dig_P7) << 4);

    /* Convert pressure from Q24.8 Pa to hPa. */
    *pressure_hpa = (float)pressure / 25600.0f;

    return HAL_OK;
}

HAL_StatusTypeDef BMP280_ReadRawData(
    I2C_HandleTypeDef *hi2c,
    uint32_t *raw_pressure,
    uint32_t *raw_temperature)
{
    HAL_StatusTypeDef status;
    uint8_t data[6];

    status = HAL_I2C_Mem_Read(
        hi2c,
        BMP280_I2C_ADDR,
        0xF7,
        I2C_MEMADD_SIZE_8BIT,
        data,
        sizeof(data),
        100);

    if (status != HAL_OK)
    {
        return status;
    }

    *raw_pressure =
        ((uint32_t)data[0] << 12) |
        ((uint32_t)data[1] << 4) |
        ((uint32_t)data[2] >> 4);

    *raw_temperature =
        ((uint32_t)data[3] << 12) |
        ((uint32_t)data[4] << 4) |
        ((uint32_t)data[5] >> 4);

    return HAL_OK;
}
