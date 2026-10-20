/*
 * aht20.c
 *
 *  Created on: Oct 20, 2026
 *      Author: s
 */

#include "aht20.h"

/* AHT20 I2C address: 0x38 */
#define AHT20_I2C_ADDR          (0x38 << 1)

/* AHT20 commands */
#define AHT20_CMD_INITIALIZE    0xBE

HAL_StatusTypeDef AHT20_Init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef status;
    uint8_t command[3];
    uint8_t sensor_status;

    /* Allow the sensor to complete its power-on sequence */
    HAL_Delay(40);

    /* Read the sensor status byte */
    status = HAL_I2C_Master_Receive(hi2c,
                                   AHT20_I2C_ADDR,
                                   &sensor_status,
                                   1,
                                   100);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Check the calibration-enable bit (bit 3) */
    if ((sensor_status & 0x08U) == 0U)
    {
        command[0] = AHT20_CMD_INITIALIZE;
        command[1] = 0x08;
        command[2] = 0x00;

        status = HAL_I2C_Master_Transmit(hi2c,
                                         AHT20_I2C_ADDR,
                                         command,
                                         sizeof(command),
                                         100);

        if (status != HAL_OK)
        {
            return status;
        }

        HAL_Delay(10);
    }

    return HAL_OK;
}

HAL_StatusTypeDef AHT20_Read(I2C_HandleTypeDef *hi2c,
                             float *temperature,
                             float *humidity)
{
    HAL_StatusTypeDef status;
    uint8_t command[3] = {0xAC, 0x33, 0x00};
    uint8_t data[7];
    uint32_t raw_humidity;
    uint32_t raw_temperature;

    /* Trigger a measurement */
    status = HAL_I2C_Master_Transmit(hi2c,
                                     AHT20_I2C_ADDR,
                                     command,
                                     sizeof(command),
                                     100);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Allow the measurement to complete */
    HAL_Delay(80);

    /* Read the measurement data */
    status = HAL_I2C_Master_Receive(hi2c,
                                    AHT20_I2C_ADDR,
                                    data,
                                    sizeof(data),
                                    100);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Bit 7 indicates that the sensor is busy */
    if ((data[0] & 0x80U) != 0U)
    {
        return HAL_BUSY;
    }

    /* Extract the 20-bit humidity and temperature measurements */
    raw_humidity = ((uint32_t)data[1] << 12)
                 | ((uint32_t)data[2] << 4)
                 | ((uint32_t)data[3] >> 4);

    raw_temperature = (((uint32_t)data[3] & 0x0FU) << 16)
                    | ((uint32_t)data[4] << 8)
                    | (uint32_t)data[5];

    /* Convert raw measurements into physical units */
    *humidity = ((float)raw_humidity * 100.0f) / 1048576.0f;

    *temperature = ((float)raw_temperature * 200.0f) / 1048576.0f
                 - 50.0f;

    return HAL_OK;
}
