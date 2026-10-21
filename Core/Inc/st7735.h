/*
 * st7735.h
 *
 *  Created on: Oct 21, 2026
 *      Author: s
 */

#ifndef INC_ST7735_H_
#define INC_ST7735_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define ST7735_BLACK   0x0000U
#define ST7735_WHITE   0xFFFFU
#define ST7735_RED     0xF800U
#define ST7735_GREEN   0x07E0U
#define ST7735_BLUE    0x001FU
#define ST7735_CYAN    0x07FFU
#define ST7735_YELLOW  0xFFE0U

HAL_StatusTypeDef ST7735_Init(SPI_HandleTypeDef *hspi);

void ST7735_FillScreen(uint16_t colour);

void ST7735_WriteString(uint16_t x, uint16_t y,
                        const char *str,
                        uint16_t colour,
                        uint16_t background);

#endif /* INC_ST7735_H_ */
