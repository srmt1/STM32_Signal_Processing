/*
 * sensor_task.h
 *
 *  Created on: 21 Oct 2026
 *      Author: s
 */

#ifndef INC_SENSOR_TASK_H_
#define INC_SENSOR_TASK_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>

typedef struct
{
    float aht20_temperature;
    float aht20_humidity;

    float bmp280_temperature;
    float bmp280_pressure_hpa;

    HAL_StatusTypeDef aht20_status;
    HAL_StatusTypeDef bmp280_status;

    uint32_t timestamp_ms;

} SensorData;

/* FreeRTOS queue handle */
extern osMessageQueueId_t sensorDataQueueHandle;
extern osMutexId_t i2c1MutexHandle;

/* Sensor acquisition task */
void StartSensorTask(void *argument);
void StartSensorPrintTask(void *argument);

#endif /* INC_SENSOR_TASK_H_ */
