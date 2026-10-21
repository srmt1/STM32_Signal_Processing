/*
 * sensor_task.c
 *
 *  Created on: Oct 21, 2026
 *      Author: s
 */

#include "sensor_task.h"

#include "aht20.h"
#include "bmp280.h"
#include "main.h"

#include <stdio.h>

/* Task function declarations */
void StartSensorTask(void *argument);
void StartSensorPrintTask(void *argument);

/*
 * Sensor acquisition task.
 * Reads both sensors once per second and sends the
 * resulting data structure to the queue.
 */
void StartSensorTask(void *argument)
{
    (void)argument;

    SensorData data = {0};
    BMP280_Calibration bmp280_cal;

    float temperature;
    float humidity;
    float pressure_hpa;

    HAL_StatusTypeDef status;

    const uint32_t sample_period_ms = 1000;
    uint32_t next_sample_tick = osKernelGetTickCount();

    /* Initialise the AHT20. */
    data.aht20_status = AHT20_Init(&hi2c1);

    /* Initialise the BMP280. */
    data.bmp280_status = BMP280_Init(&hi2c1);

    if (data.bmp280_status == HAL_OK)
    {
        data.bmp280_status = BMP280_ReadCalibration(
            &hi2c1,
            &bmp280_cal
        );
    }

    /*
     * Read the sensors periodically.
     */
    for (;;)
    {
        /* Acquire exclusive access to I2C1. */
        if (osMutexAcquire(i2c1MutexHandle, osWaitForever) == osOK)
        {
        	/* AHT20 temperature and humidity */
        	if (data.aht20_status == HAL_OK)
        	{
        		status = AHT20_Read(
        				&hi2c1,
						&temperature,
						&humidity
        		);

        		data.aht20_status = status;

        		if (status == HAL_OK)
        		{
        			data.aht20_temperature = temperature;
        			data.aht20_humidity = humidity;
        		}
        	}

        	/* BMP280 temperature and pressure */
        	if (data.bmp280_status == HAL_OK)
        	{
        		status = BMP280_ReadTemperature(
        				&hi2c1,
						&bmp280_cal,
						&temperature
        		);

        		if (status == HAL_OK)
        		{
        			data.bmp280_temperature = temperature;

        			status = BMP280_ReadPressure(
        					&hi2c1,
							&bmp280_cal,
							&pressure_hpa
        			);
        		}

        		data.bmp280_status = status;

        		if (status == HAL_OK)
        		{
        			data.bmp280_pressure_hpa = pressure_hpa;
        		}
        	}
        	/* Release I2C1 after all sensor transactions. */
        	osMutexRelease(i2c1MutexHandle);
        }
        /* Record when this sample was collected. */
        data.timestamp_ms = HAL_GetTick();

        /*
         * Send a copy of the structure to the queue.
         * Wait up to 10 ms if the queue is full.
         */
        if (osMessageQueuePut(
                sensorDataQueueHandle,
                &data,
                0,
                10) != osOK)
        {
            /*
             * Queue full: the current sample was not sent.
             * The next iteration will try again.
             */
        }

        /* Schedule the next sample at a fixed 1-second interval. */
        next_sample_tick += sample_period_ms;
        osDelayUntil(next_sample_tick);
    }
}


/*
 * Consumer task.
 * Receives sensor measurements from the queue and prints
 * them over UART.
 */
void StartSensorPrintTask(void *argument)
{
    (void)argument;

    SensorData received_data;

    for (;;)
    {
        if (osMessageQueueGet(
                sensorDataQueueHandle,
                &received_data,
                NULL,
                osWaitForever) == osOK)
        {
            printf("\r\nSensor sample at %lu ms\r\n",
                   (unsigned long)received_data.timestamp_ms);

            if (received_data.aht20_status == HAL_OK)
            {
                printf("AHT20 Temperature: %.2f C\r\n",
                       received_data.aht20_temperature);

                printf("AHT20 Humidity: %.2f %%\r\n",
                       received_data.aht20_humidity);
            }
            else
            {
                printf("AHT20 read failed: %d\r\n",
                       (int)received_data.aht20_status);
            }

            if (received_data.bmp280_status == HAL_OK)
            {
                printf("BMP280 Temperature: %.2f C\r\n",
                       received_data.bmp280_temperature);

                printf("BMP280 Pressure: %.2f hPa\r\n",
                       received_data.bmp280_pressure_hpa);
            }
            else
            {
                printf("BMP280 read failed: %d\r\n",
                       (int)received_data.bmp280_status);
            }
        }
    }
}
