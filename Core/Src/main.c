/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include "arm_math.h"
#include <stdio.h>
#include <string.h>
#include "queue.h"
#include "aht20.h"
#include "bmp280.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

DAC_HandleTypeDef hdac;
DMA_HandleTypeDef hdma_dac1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;

/* Definitions for uartTask */
osThreadId_t uartTaskHandle;
const osThreadAttr_t uartTask_attributes = {
  .name = "uartTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for fftTask */
osThreadId_t fftTaskHandle;
TaskHandle_t fftTaskNativeHandle = NULL;
const osThreadAttr_t fftTask_attributes = {
  .name = "fftTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for frequencyQueue */
osMessageQueueId_t frequencyQueueHandle;
const osMessageQueueAttr_t frequencyQueue_attributes = {
  .name = "frequencyQueue"
};
osMessageQueueId_t adcEventQueueHandle;
/* USER CODE BEGIN PV */
volatile uint32_t main_counter = 0;
volatile uint32_t adc_value = 0;
volatile uint32_t adc_millivolts = 0;

#define ADC_BUFFER_SIZE 2048
uint16_t adc_buffer[ADC_BUFFER_SIZE];
volatile uint32_t adc_half_count = 0;
volatile uint32_t adc_full_count = 0;
volatile uint32_t adc_event_queue_overflow_count = 0;

/* CMSIS-DSP FFT */
#define FFT_SIZE 1024

float32_t fft_input[FFT_SIZE];
float32_t fft_output[FFT_SIZE];
float32_t fft_magnitude[FFT_SIZE / 2];
float32_t hann_window[FFT_SIZE];

volatile uint32_t fft_complete_count = 0;

volatile uint32_t fft_half_processed_count = 0;
volatile uint32_t fft_full_processed_count = 0;

volatile uint32_t fft_peak_bin = 0;
volatile float32_t fft_peak_frequency = 0.0f;
uint32_t fft_sample_index = 0;

arm_rfft_fast_instance_f32 fft_instance;

#define DAC_BUFFER_SIZE 256

uint16_t dac_buffer[DAC_BUFFER_SIZE];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM3_Init(void);
static void MX_I2C1_Init(void);
void StartDefaultTask(void *argument);
void StartTask02(void *argument);

/* USER CODE BEGIN PFP */
void ProcessFFT(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_DAC_Init();
  MX_TIM6_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  HAL_StatusTypeDef aht20_status;
  HAL_StatusTypeDef bmp280_76_status;
  HAL_StatusTypeDef bmp280_77_status;

  aht20_status = HAL_I2C_IsDeviceReady(&hi2c1,
                                       (0x38 << 1),
                                       3,
                                       100);

  bmp280_76_status = HAL_I2C_IsDeviceReady(&hi2c1,
                                          (0x76 << 1),
                                          3,
                                          100);

  bmp280_77_status = HAL_I2C_IsDeviceReady(&hi2c1,
                                          (0x77 << 1),
                                          3,
                                          100);

  printf("AHT20 (0x38): %s\r\n",
         (aht20_status == HAL_OK) ? "FOUND" : "NOT FOUND");

  printf("BMP280 (0x76): %s\r\n",
         (bmp280_76_status == HAL_OK) ? "FOUND" : "NOT FOUND");

  printf("BMP280 (0x77): %s\r\n",
         (bmp280_77_status == HAL_OK) ? "FOUND" : "NOT FOUND");

  HAL_StatusTypeDef aht20_init_status;

  aht20_init_status = AHT20_Init(&hi2c1);

  printf("AHT20 initialisation: %s\r\n",
         (aht20_init_status == HAL_OK) ? "OK" : "FAILED");

  float aht20_temperature = 0.0f;
  float aht20_humidity = 0.0f;

  HAL_StatusTypeDef aht20_read_status;

  aht20_read_status = AHT20_Read(&hi2c1,
                                 &aht20_temperature,
                                 &aht20_humidity);

  if (aht20_read_status == HAL_OK)
  {
      printf("AHT20 Temperature: %.2f C\r\n", aht20_temperature);
      printf("AHT20 Humidity: %.2f %%\r\n", aht20_humidity);
  }
  else
  {
      printf("AHT20 measurement failed: %d\r\n",
             (int)aht20_read_status);
  }

  /* ---------- BMP280 chip ID ---------- */

  uint8_t bmp280_chip_id = 0;
  HAL_StatusTypeDef bmp280_id_status;

  bmp280_id_status = BMP280_ReadChipID(&hi2c1, &bmp280_chip_id);

  if (bmp280_id_status == HAL_OK)
  {
      printf("BMP280 Chip ID: 0x%02X\r\n", bmp280_chip_id);

      if (bmp280_chip_id == 0x58)
      {
          printf("BMP280 chip ID check: PASS\r\n");
      }
      else
      {
          printf("BMP280 chip ID check: UNEXPECTED ID\r\n");
      }
  }
  else
  {
      printf("BMP280 chip ID read failed: %d\r\n",
             (int)bmp280_id_status);
  }


  /* ---------- BMP280 initialisation ---------- */

  HAL_StatusTypeDef bmp280_init_status;

  bmp280_init_status = BMP280_Init(&hi2c1);

  printf("BMP280 initialisation: %s\r\n",
         (bmp280_init_status == HAL_OK) ? "OK" : "FAILED");

  if (bmp280_init_status != HAL_OK)
  {
      printf("Stopping BMP280 test because initialisation failed.\r\n");
  }
  else
  {
      /* ---------- Read calibration coefficients ---------- */

      BMP280_Calibration bmp280_cal;
      HAL_StatusTypeDef bmp280_cal_status;

      bmp280_cal_status = BMP280_ReadCalibration(&hi2c1, &bmp280_cal);

      if (bmp280_cal_status == HAL_OK)
      {
          printf("BMP280 Calibration:\r\n");

          printf("T1=%u, T2=%d, T3=%d\r\n",
                 (unsigned int)bmp280_cal.dig_T1,
                 (int)bmp280_cal.dig_T2,
                 (int)bmp280_cal.dig_T3);

          printf("P1=%u, P2=%d, P3=%d\r\n",
                 (unsigned int)bmp280_cal.dig_P1,
                 (int)bmp280_cal.dig_P2,
                 (int)bmp280_cal.dig_P3);

          printf("P4=%d, P5=%d, P6=%d\r\n",
                 (int)bmp280_cal.dig_P4,
                 (int)bmp280_cal.dig_P5,
                 (int)bmp280_cal.dig_P6);

          printf("P7=%d, P8=%d, P9=%d\r\n",
                 (int)bmp280_cal.dig_P7,
                 (int)bmp280_cal.dig_P8,
                 (int)bmp280_cal.dig_P9);


          /* ---------- Compare two raw readings ---------- */

          uint32_t raw_pressure_1 = 0;
          uint32_t raw_temperature_1 = 0;
          uint32_t raw_pressure_2 = 0;
          uint32_t raw_temperature_2 = 0;

          HAL_StatusTypeDef raw_status_1;
          HAL_StatusTypeDef raw_status_2;

          raw_status_1 = BMP280_ReadRawData(
              &hi2c1,
              &raw_pressure_1,
              &raw_temperature_1);

          HAL_Delay(10);

          raw_status_2 = BMP280_ReadRawData(
              &hi2c1,
              &raw_pressure_2,
              &raw_temperature_2);

          if (raw_status_1 == HAL_OK)
          {
              printf("Raw reading 1: pressure=%lu, temperature=%lu\r\n",
                     (unsigned long)raw_pressure_1,
                     (unsigned long)raw_temperature_1);
          }
          else
          {
              printf("Raw reading 1 failed: %d\r\n",
                     (int)raw_status_1);
          }

          if (raw_status_2 == HAL_OK)
          {
              printf("Raw reading 2: pressure=%lu, temperature=%lu\r\n",
                     (unsigned long)raw_pressure_2,
                     (unsigned long)raw_temperature_2);
          }
          else
          {
              printf("Raw reading 2 failed: %d\r\n",
                     (int)raw_status_2);
          }


          /* ---------- Read compensated temperature ---------- */

          float bmp280_temperature = 0.0f;
          HAL_StatusTypeDef bmp280_temp_status;

          bmp280_temp_status = BMP280_ReadTemperature(
              &hi2c1,
              &bmp280_cal,
              &bmp280_temperature);

          if (bmp280_temp_status == HAL_OK)
          {
              printf("BMP280 Temperature: %.2f C\r\n",
                     bmp280_temperature);
          }
          else
          {
              printf("BMP280 temperature read failed: %d\r\n",
                     (int)bmp280_temp_status);
          }


          /* ---------- Read compensated pressure ---------- */

          float bmp280_pressure_hpa = 0.0f;
          HAL_StatusTypeDef bmp280_pressure_status;

          /*
           * ReadTemperature() must run first because it calculates
           * bmp280_t_fine, which the pressure compensation uses.
           */
          if (bmp280_temp_status == HAL_OK)
          {
              bmp280_pressure_status = BMP280_ReadPressure(
                  &hi2c1,
                  &bmp280_cal,
                  &bmp280_pressure_hpa);

              if (bmp280_pressure_status == HAL_OK)
              {
                  printf("BMP280 Pressure: %.2f hPa\r\n",
                         bmp280_pressure_hpa);
              }
              else
              {
                  printf("BMP280 pressure read failed: %d\r\n",
                         (int)bmp280_pressure_status);
              }
          }
          else
          {
              printf("Skipping pressure compensation because "
                     "temperature reading failed.\r\n");
          }
      }
      else
      {
          printf("BMP280 calibration read failed: %d\r\n",
                 (int)bmp280_cal_status);
      }
  }

  arm_rfft_fast_init_f32(&fft_instance, FFT_SIZE);

  /* Pre-calculate the Hann window coefficients. */
  for (uint32_t i = 0; i < FFT_SIZE; i++)
  {
      hann_window[i] = 0.5f * (1.0f - cosf(
          (2.0f * 3.14159265358979f * (float32_t)i)
          / (float32_t)(FFT_SIZE - 1)
      ));
  }

  for (uint32_t i = 0; i < DAC_BUFFER_SIZE; i++)
  {
      float32_t angle = 2.0f * 3.14159265358979f
                      * (float32_t)i / (float32_t)DAC_BUFFER_SIZE;

      dac_buffer[i] = (uint16_t)(1241.0f + 1241.0f * sinf(angle));
  }

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of frequencyQueue */
  frequencyQueueHandle = osMessageQueueNew (4, sizeof(float32_t), &frequencyQueue_attributes);
  adcEventQueueHandle = osMessageQueueNew(
        8,
        sizeof(uint32_t),
        NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of uartTask */
  uartTaskHandle = osThreadNew(StartDefaultTask, NULL, &uartTask_attributes);

  /* creation of fftTask */
  fftTaskHandle = osThreadNew(StartTask02, NULL, &fftTask_attributes);
  fftTaskNativeHandle = (TaskHandle_t)fftTaskHandle;
  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */

  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 61;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 104;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 49;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_half_count++;

        uint32_t event = 1U;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        if (xQueueSendFromISR(
                adcEventQueueHandle,
                &event,
                &xHigherPriorityTaskWoken) != pdPASS)
        {
            adc_event_queue_overflow_count++;
        }

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_full_count++;

        uint32_t event = 2U;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        if (xQueueSendFromISR(
                adcEventQueueHandle,
                &event,
                &xHigherPriorityTaskWoken) != pdPASS)
        {
            adc_event_queue_overflow_count++;
        }

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void ProcessFFT(void)
{
    float32_t mean = 0.0f;

    /* Calculate the mean ADC value to remove the DC offset. */
    for (uint32_t i = 0; i < FFT_SIZE; i++)
    {
        mean += (float32_t)fft_input[i];
    }

    mean /= (float32_t)FFT_SIZE;

    /* Remove DC offset. */
    for (uint32_t i = 0; i < FFT_SIZE; i++)
    {
        fft_input[i] -= mean;
    }

    /* Apply the Hann window. */
    for (uint32_t i = 0; i < FFT_SIZE; i++)
    {
        fft_input[i] *= hann_window[i];
    }

    /* Perform the real FFT. */
    arm_rfft_fast_f32(&fft_instance, fft_input, fft_output, 0);

    /* Calculate magnitudes for bins 1 to 511. */
    for (uint32_t i = 1; i < FFT_SIZE / 2; i++)
    {
        float32_t real = fft_output[2 * i];
        float32_t imag = fft_output[2 * i + 1];

        fft_magnitude[i] = sqrtf((real * real) + (imag * imag));
    }

    /* Find the FFT bin with the largest magnitude. */
    float32_t max_magnitude = 0.0f;
    uint32_t peak_bin = 1;

    for (uint32_t i = 1; i < FFT_SIZE / 2; i++)
    {
        if (fft_magnitude[i] > max_magnitude)
        {
            max_magnitude = fft_magnitude[i];
            peak_bin = i;
        }
    }

    /* Estimate the peak position using parabolic interpolation. */
    float32_t left = fft_magnitude[peak_bin - 1];
    float32_t centre = fft_magnitude[peak_bin];
    float32_t right = fft_magnitude[peak_bin + 1];

    float32_t denominator = left - (2.0f * centre) + right;
    float32_t offset = 0.0f;

    if (denominator != 0.0f)
    {
        offset = 0.5f * (left - right) / denominator;
    }

    /* Convert the interpolated bin position into frequency in Hz. */
    fft_peak_bin = peak_bin;
    fft_peak_frequency =
        ((float32_t)peak_bin + offset) * 16129.03f / (float32_t)FFT_SIZE;

    fft_complete_count++;
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the uartTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
	for (;;)
	{
	    float32_t frequency;

	    if (osMessageQueueGet(frequencyQueueHandle,
	                          &frequency,
	                          NULL,
	                          osWaitForever) == osOK)
	    {
	        char uart_message[64];

	        int length = snprintf(uart_message,
	                              sizeof(uart_message),
	                              "Frequency: %.1f Hz\r\n",
	                              (double)frequency);

	        if (length > 0 && length < sizeof(uart_message))
	        {
	            /*HAL_UART_Transmit(&huart2,
	                              (uint8_t *)uart_message,
	                              (uint16_t)length,
	                              HAL_MAX_DELAY);*/
	        }
	    }
	}
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the fftTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */
	/* Start peripherals here */
	  if (HAL_DAC_Start_DMA(&hdac,
	                        DAC_CHANNEL_1,
	                        (uint32_t *)dac_buffer,
	                        DAC_BUFFER_SIZE,
	                        DAC_ALIGN_12B_R) != HAL_OK)
	  {
	      Error_Handler();
	  }

	  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer, ADC_BUFFER_SIZE);
	  HAL_TIM_Base_Start(&htim2);

	  if (HAL_TIM_Base_Start(&htim6) != HAL_OK)
	  {
	      Error_Handler();
	  }

	  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
	  {
	      Error_Handler();
	  }
    for (;;)
    {
    	/* Wait for the next ADC DMA event. */
    	uint32_t event = 0;

    	if (osMessageQueueGet(
    	        adcEventQueueHandle,
    	        &event,
    	        NULL,
    	        osWaitForever) == osOK)
    	{
    	    if (event == 1U)
    	    {
    	        /* Process the first half of the DMA buffer. */
    	        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

    	        for (uint32_t i = 0; i < FFT_SIZE; i++)
    	        {
    	            fft_input[i] = (float32_t)adc_buffer[i];
    	        }

    	        ProcessFFT();

    	        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    	        float32_t frequency = fft_peak_frequency;

    	        osMessageQueuePut(
    	            frequencyQueueHandle,
    	            &frequency,
    	            0,
    	            0
    	        );

    	        fft_half_processed_count++;
    	    }
    	    else if (event == 2U)
    	    {
    	        /* Process the second half of the DMA buffer. */
    	        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

    	        for (uint32_t i = 0; i < FFT_SIZE; i++)
    	        {
    	            fft_input[i] = (float32_t)adc_buffer[i + FFT_SIZE];
    	        }

    	        ProcessFFT();

    	        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    	        float32_t frequency = fft_peak_frequency;

    	        osMessageQueuePut(
    	            frequencyQueueHandle,
    	            &frequency,
    	            0,
    	            0
    	        );

    	        fft_full_processed_count++;
    	    }
    	}

    	osDelay(1);
    }
  /* USER CODE END StartTask02 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM7 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM7)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
