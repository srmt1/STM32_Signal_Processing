/*
 * st7735.c
 *
 *  Created on: Oct 21, 2026
 *      Author: s
 */


#include "st7735.h"
#include <stddef.h>

/* Display dimensions */
#define ST7735_WIDTH   128U
#define ST7735_HEIGHT  160U
#define ST7735_XSTART  2U
#define ST7735_YSTART  1U

/* GPIO assignments from CubeMX */
#define ST7735_CS_PORT     GPIOB
#define ST7735_CS_PIN      GPIO_PIN_12

#define ST7735_DC_PORT     GPIOB
#define ST7735_DC_PIN      GPIO_PIN_10

#define ST7735_RST_PORT    GPIOB
#define ST7735_RST_PIN     GPIO_PIN_2

/* ST7735 commands */
#define ST7735_SWRESET     0x01U
#define ST7735_SLPOUT      0x11U
#define ST7735_NORON       0x13U
#define ST7735_INVOFF      0x20U
#define ST7735_DISPON      0x29U
#define ST7735_CASET       0x2AU
#define ST7735_RASET       0x2BU
#define ST7735_RAMWR       0x2CU
#define ST7735_COLMOD      0x3AU
#define ST7735_MADCTL      0x36U

static SPI_HandleTypeDef *st7735_spi;

static void ST7735_Select(void)
{
    HAL_GPIO_WritePin(ST7735_CS_PORT, ST7735_CS_PIN, GPIO_PIN_RESET);
}

static void ST7735_Unselect(void)
{
    HAL_GPIO_WritePin(ST7735_CS_PORT, ST7735_CS_PIN, GPIO_PIN_SET);
}

static void ST7735_WriteCommand(uint8_t command)
{
    HAL_GPIO_WritePin(ST7735_DC_PORT, ST7735_DC_PIN, GPIO_PIN_RESET);

    ST7735_Select();
    (void)HAL_SPI_Transmit(st7735_spi, &command, 1U, HAL_MAX_DELAY);
    ST7735_Unselect();
}

static void ST7735_WriteData(const uint8_t *data, uint16_t length)
{
    HAL_GPIO_WritePin(ST7735_DC_PORT, ST7735_DC_PIN, GPIO_PIN_SET);

    ST7735_Select();
    (void)HAL_SPI_Transmit(st7735_spi, (uint8_t *)data,
                           length, HAL_MAX_DELAY);
    ST7735_Unselect();
}

static void ST7735_WriteDataByte(uint8_t data)
{
    ST7735_WriteData(&data, 1U);
}

static void ST7735_SetAddressWindow(uint16_t x0, uint16_t y0,
                                    uint16_t x1, uint16_t y1)
{
    uint8_t data[4];

    /* Apply the controller's visible-area offsets. */
    x0 += ST7735_XSTART;
    x1 += ST7735_XSTART;
    y0 += ST7735_YSTART;
    y1 += ST7735_YSTART;

    ST7735_WriteCommand(ST7735_CASET);

    data[0] = (uint8_t)(x0 >> 8);
    data[1] = (uint8_t)x0;
    data[2] = (uint8_t)(x1 >> 8);
    data[3] = (uint8_t)x1;
    ST7735_WriteData(data, sizeof(data));

    ST7735_WriteCommand(ST7735_RASET);

    data[0] = (uint8_t)(y0 >> 8);
    data[1] = (uint8_t)y0;
    data[2] = (uint8_t)(y1 >> 8);
    data[3] = (uint8_t)y1;
    ST7735_WriteData(data, sizeof(data));

    ST7735_WriteCommand(ST7735_RAMWR);
}

HAL_StatusTypeDef ST7735_Init(SPI_HandleTypeDef *hspi)
{
    if (hspi == NULL)
    {
        return HAL_ERROR;
    }

    st7735_spi = hspi;

    /* Hardware reset */
    HAL_GPIO_WritePin(ST7735_CS_PORT, ST7735_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ST7735_RST_PORT, ST7735_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10U);

    HAL_GPIO_WritePin(ST7735_RST_PORT, ST7735_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(20U);

    HAL_GPIO_WritePin(ST7735_RST_PORT, ST7735_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(120U);

    ST7735_WriteCommand(ST7735_SWRESET);
    HAL_Delay(150U);

    ST7735_WriteCommand(ST7735_SLPOUT);
    HAL_Delay(120U);

    ST7735_WriteCommand(ST7735_COLMOD);
    ST7735_WriteDataByte(0x05U); /* 16-bit RGB565 */

    ST7735_WriteCommand(ST7735_MADCTL);
    ST7735_WriteDataByte(0x00U); /* Initial orientation */

    ST7735_WriteCommand(ST7735_INVOFF);
    ST7735_WriteCommand(ST7735_NORON);
    HAL_Delay(10U);

    ST7735_WriteCommand(ST7735_DISPON);
    HAL_Delay(100U);

    ST7735_FillScreen(0x0000U);

    return HAL_OK;
}

void ST7735_FillScreen(uint16_t colour)
{
    uint8_t pixel[2];
    uint32_t count;

    if (st7735_spi == NULL)
    {
        return;
    }

    pixel[0] = (uint8_t)(colour >> 8);
    pixel[1] = (uint8_t)colour;

    ST7735_SetAddressWindow(0U, 0U,
                            ST7735_WIDTH - 1U,
                            ST7735_HEIGHT - 1U);

    HAL_GPIO_WritePin(ST7735_DC_PORT, ST7735_DC_PIN, GPIO_PIN_SET);
    ST7735_Select();

    for (count = 0U;
         count < (uint32_t)ST7735_WIDTH * ST7735_HEIGHT;
         count++)
    {
        if (HAL_SPI_Transmit(st7735_spi, pixel, 2U,
                             HAL_MAX_DELAY) != HAL_OK)
        {
            break;
        }
    }

    ST7735_Unselect();
}

/* Basic 5x7 font.
 * Each character is five columns wide.
 * Each byte stores one column, with bit 0 at the top.
 * Supports digits, uppercase letters, common punctuation and space.
 */
static const uint8_t font5x7[][5] =
{
    /* Space */
    [0] = {0x00, 0x00, 0x00, 0x00, 0x00},

    /* Digits 0-9: indices 1-10 */
    [1]  = {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0 */
    [2]  = {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 1 */
    [3]  = {0x42, 0x61, 0x51, 0x49, 0x46}, /* 2 */
    [4]  = {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 3 */
    [5]  = {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 4 */
    [6]  = {0x27, 0x45, 0x45, 0x45, 0x39}, /* 5 */
    [7]  = {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 6 */
    [8]  = {0x01, 0x71, 0x09, 0x05, 0x03}, /* 7 */
    [9]  = {0x36, 0x49, 0x49, 0x49, 0x36}, /* 8 */
    [10] = {0x06, 0x49, 0x49, 0x29, 0x1E}, /* 9 */

    /* Uppercase A-Z: indices 11-36 */
    [11] = {0x7E, 0x11, 0x11, 0x11, 0x7E}, /* A */
    [12] = {0x7F, 0x49, 0x49, 0x49, 0x36}, /* B */
    [13] = {0x3E, 0x41, 0x41, 0x41, 0x22}, /* C */
    [14] = {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* D */
    [15] = {0x7F, 0x49, 0x49, 0x49, 0x41}, /* E */
    [16] = {0x7F, 0x09, 0x09, 0x09, 0x01}, /* F */
    [17] = {0x3E, 0x41, 0x49, 0x49, 0x7A}, /* G */
    [18] = {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* H */
    [19] = {0x00, 0x41, 0x7F, 0x41, 0x00}, /* I */
    [20] = {0x20, 0x40, 0x41, 0x3F, 0x01}, /* J */
    [21] = {0x7F, 0x08, 0x14, 0x22, 0x41}, /* K */
    [22] = {0x7F, 0x40, 0x40, 0x40, 0x40}, /* L */
    [23] = {0x7F, 0x02, 0x0C, 0x02, 0x7F}, /* M */
    [24] = {0x7F, 0x04, 0x08, 0x10, 0x7F}, /* N */
    [25] = {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* O */
    [26] = {0x7F, 0x09, 0x09, 0x09, 0x06}, /* P */
    [27] = {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* Q */
    [28] = {0x7F, 0x09, 0x19, 0x29, 0x46}, /* R */
    [29] = {0x46, 0x49, 0x49, 0x49, 0x31}, /* S */
    [30] = {0x01, 0x01, 0x7F, 0x01, 0x01}, /* T */
    [31] = {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* U */
    [32] = {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* V */
    [33] = {0x3F, 0x40, 0x38, 0x40, 0x3F}, /* W */
    [34] = {0x63, 0x14, 0x08, 0x14, 0x63}, /* X */
    [35] = {0x07, 0x08, 0x70, 0x08, 0x07}, /* Y */
    [36] = {0x61, 0x51, 0x49, 0x45, 0x43}, /* Z */

    /* Punctuation: indices 37-42 */
    [37] = {0x00, 0x60, 0x60, 0x00, 0x00}, /* . */
    [38] = {0x00, 0x08, 0x08, 0x08, 0x00}, /* - */
    [39] = {0x00, 0x36, 0x36, 0x00, 0x00}, /* : */
    [40] = {0x08, 0x08, 0x08, 0x08, 0x08}, /* _ */
    [41] = {0x00, 0x06, 0x09, 0x09, 0x06}, /* degree-like symbol */
    [42] = {0x00, 0x00, 0x00, 0x00, 0x00}  /* fallback */
};

/* Convert a supported character to its font-table index. */
static uint8_t ST7735_GetFontIndex(char c)
{
    if (c == ' ')
    {
        return 0U;
    }

    if (c >= '0' && c <= '9')
    {
        return (uint8_t)(c - '0' + 1);
    }

    if (c >= 'A' && c <= 'Z')
    {
        return (uint8_t)(c - 'A' + 11);
    }

    if (c >= 'a' && c <= 'z')
    {
        return (uint8_t)(c - 'a' + 11);
    }

    switch (c)
    {
        case '.':  return 37U;
        case '-':  return 38U;
        case ':':  return 39U;
        case '_':  return 40U;
        default:   return 42U;
    }
}

/* Draw one 5x7 character using a single SPI pixel-data transfer. */
static void ST7735_DrawChar(uint16_t x, uint16_t y,
                            char c, uint16_t colour,
                            uint16_t background)
{
    uint8_t index = ST7735_GetFontIndex(c);
    uint8_t pixel_data[5U * 7U * 2U];
    uint8_t column;
    uint8_t row;
    uint16_t offset = 0U;

    /* Build the complete character image in RGB565 format. */
    for (row = 0U; row < 7U; row++)
    {
        for (column = 0U; column < 5U; column++)
        {
            uint16_t pixel_colour =
                (font5x7[index][column] & (1U << row))
                ? colour : background;

            pixel_data[offset++] = (uint8_t)(pixel_colour >> 8);
            pixel_data[offset++] = (uint8_t)pixel_colour;
        }
    }

    /* Select the complete 5x7 character area. */
    ST7735_SetAddressWindow(x, y, x + 4U, y + 6U);

    /* Transmit all 35 pixels in one SPI transfer. */
    ST7735_WriteData(pixel_data, sizeof(pixel_data));
}

/* Draw a string using 6x8 character cells. */
void ST7735_WriteString(uint16_t x, uint16_t y,
                        const char *str,
                        uint16_t colour,
                        uint16_t background)
{
    uint16_t cursor_x = x;

    if (str == NULL)
    {
        return;
    }

    while (*str != '\0')
    {
        if (cursor_x + 6U > ST7735_WIDTH)
        {
            break;
        }

        ST7735_DrawChar(cursor_x, y, *str, colour, background);

        cursor_x += 6U;
        str++;
    }
}
