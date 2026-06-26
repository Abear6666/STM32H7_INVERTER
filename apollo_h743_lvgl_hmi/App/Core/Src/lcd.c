#include "lcd.h"

#include <stdio.h>

#include "main.h"

#define APP_LCD_PANEL_ID_RGB4384 0x4384U
#define APP_LCD_PIXEL_CLOCK_HZ 33333333UL

static LTDC_HandleTypeDef s_ltdc;

static void app_lcd_gpio_init(void);
static void app_lcd_backlight_gpio_init(void);
static uint16_t app_lcd_rgb565(uint8_t r, uint8_t g, uint8_t b);
static void app_lcd_fill_rect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color);

void app_lcd_init(void)
{
    LTDC_LayerCfgTypeDef layer = {0};

    s_ltdc.Instance = LTDC;
    s_ltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    s_ltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    s_ltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    s_ltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    s_ltdc.Init.HorizontalSync = 48U - 1U;
    s_ltdc.Init.VerticalSync = 3U - 1U;
    s_ltdc.Init.AccumulatedHBP = 48U + 88U - 1U;
    s_ltdc.Init.AccumulatedVBP = 3U + 32U - 1U;
    s_ltdc.Init.AccumulatedActiveW = 48U + 88U + APP_LCD_WIDTH - 1U;
    s_ltdc.Init.AccumulatedActiveH = 3U + 32U + APP_LCD_HEIGHT - 1U;
    s_ltdc.Init.TotalWidth = 48U + 88U + APP_LCD_WIDTH + 40U - 1U;
    s_ltdc.Init.TotalHeigh = 3U + 32U + APP_LCD_HEIGHT + 13U - 1U;
    s_ltdc.Init.Backcolor.Red = 0;
    s_ltdc.Init.Backcolor.Green = 0;
    s_ltdc.Init.Backcolor.Blue = 0;

    if (HAL_LTDC_Init(&s_ltdc) != HAL_OK)
    {
        Error_Handler();
    }

    layer.WindowX0 = 0;
    layer.WindowX1 = APP_LCD_WIDTH;
    layer.WindowY0 = 0;
    layer.WindowY1 = APP_LCD_HEIGHT;
    layer.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
    layer.Alpha = 255;
    layer.Alpha0 = 0;
    layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    layer.FBStartAdress = APP_LCD_FRAMEBUFFER_ADDR;
    layer.ImageWidth = APP_LCD_WIDTH;
    layer.ImageHeight = APP_LCD_HEIGHT;
    layer.Backcolor.Red = 0;
    layer.Backcolor.Green = 0;
    layer.Backcolor.Blue = 0;

    if (HAL_LTDC_ConfigLayer(&s_ltdc, &layer, 0) != HAL_OK)
    {
        Error_Handler();
    }
}

void app_lcd_fill_test_pattern(void)
{
    const uint16_t colors[] = {
        0xF800U,
        0x07E0U,
        0x001FU,
        0xFFE0U,
        0xF81FU,
        0x07FFU,
        0xFFFFU,
        0x0000U,
    };
    uint16_t stripe_width = (uint16_t)(APP_LCD_WIDTH / (sizeof(colors) / sizeof(colors[0])));

    for (uint16_t i = 0; i < (sizeof(colors) / sizeof(colors[0])); ++i)
    {
        uint16_t x = (uint16_t)(i * stripe_width);
        uint16_t w = (i == ((sizeof(colors) / sizeof(colors[0])) - 1U)) ? (uint16_t)(APP_LCD_WIDTH - x) : stripe_width;

        app_lcd_fill_rect(x, 0, w, APP_LCD_HEIGHT, colors[i]);
    }

    app_lcd_fill_rect(20, 20, 120, 80, app_lcd_rgb565(255, 255, 255));
    app_lcd_fill_rect(40, 40, 80, 40, app_lcd_rgb565(0, 0, 0));
    __DSB();
}

void app_lcd_fill_phase5_touch_background(void)
{
    app_lcd_fill_rect(0, 0, APP_LCD_WIDTH, APP_LCD_HEIGHT, app_lcd_rgb565(255, 255, 255));
    app_lcd_fill_rect(0, 0, 24, 20, app_lcd_rgb565(0, 0, 0));
    __DSB();
}

void app_lcd_draw_touch_mark(uint16_t x, uint16_t y)
{
    const uint16_t color = 0xF800U;
    uint16_t x0 = (x > 2U) ? (uint16_t)(x - 2U) : 0U;
    uint16_t y0 = (y > 2U) ? (uint16_t)(y - 2U) : 0U;
    uint16_t x1 = ((uint32_t)x + 2U < APP_LCD_WIDTH) ? (uint16_t)(x + 2U) : (uint16_t)(APP_LCD_WIDTH - 1U);
    uint16_t y1 = ((uint32_t)y + 2U < APP_LCD_HEIGHT) ? (uint16_t)(y + 2U) : (uint16_t)(APP_LCD_HEIGHT - 1U);

    app_lcd_fill_rect(x0, y0, (uint16_t)(x1 - x0 + 1U), (uint16_t)(y1 - y0 + 1U), color);
    __DSB();
}

void app_lcd_backlight_on(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
}

void app_lcd_print_config(void)
{
    app_lcd_info_t info = app_lcd_get_info();

    printf("LCD panel id=0x%04X, RGB565, %ux%u\r\n",
           (unsigned int)info.panel_id,
           (unsigned int)info.width,
           (unsigned int)info.height);
    printf("LTDC timing: HSW=%u HBP=%u HFP=%u VSW=%u VBP=%u VFP=%u\r\n",
           (unsigned int)info.hsw,
           (unsigned int)info.hbp,
           (unsigned int)info.hfp,
           (unsigned int)info.vsw,
           (unsigned int)info.vbp,
           (unsigned int)info.vfp);
    printf("LTDC pixel clock: PLL3 M=25 N=300 R=9, %lu Hz\r\n",
           (unsigned long)APP_LCD_PIXEL_CLOCK_HZ);
    printf("LCD framebuffer: 0x%08lX, %lu bytes\r\n",
           (unsigned long)info.framebuffer_addr,
           (unsigned long)info.framebuffer_size_bytes);
    printf("LCD backlight: PB5 high\r\n");
}

app_lcd_info_t app_lcd_get_info(void)
{
    app_lcd_info_t info;

    info.panel_id = APP_LCD_PANEL_ID_RGB4384;
    info.width = APP_LCD_WIDTH;
    info.height = APP_LCD_HEIGHT;
    info.hsw = 48;
    info.hbp = 88;
    info.hfp = 40;
    info.vsw = 3;
    info.vbp = 32;
    info.vfp = 13;
    info.framebuffer_addr = APP_LCD_FRAMEBUFFER_ADDR;
    info.framebuffer_size_bytes = APP_LCD_FRAMEBUFFER_SIZE_BYTES;

    return info;
}

void HAL_LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{
    if (hltdc->Instance == LTDC)
    {
        __HAL_RCC_LTDC_CLK_ENABLE();
        app_lcd_backlight_gpio_init();
        app_lcd_gpio_init();
    }
}

static void app_lcd_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF14_LTDC;

    gpio.Pin = GPIO_PIN_10;
    HAL_GPIO_Init(GPIOF, &gpio);

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOG, &gpio);

    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOH, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
               GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_9 |
               GPIO_PIN_10;
    HAL_GPIO_Init(GPIOI, &gpio);
}

static void app_lcd_backlight_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
}

static uint16_t app_lcd_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)((((uint16_t)r & 0xF8U) << 8) |
                      (((uint16_t)g & 0xFCU) << 3) |
                      (((uint16_t)b & 0xF8U) >> 3));
}

static void app_lcd_fill_rect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color)
{
    volatile uint16_t *fb = (volatile uint16_t *)APP_LCD_FRAMEBUFFER_ADDR;

    for (uint16_t y = y0; y < (uint16_t)(y0 + h); ++y)
    {
        for (uint16_t x = x0; x < (uint16_t)(x0 + w); ++x)
        {
            fb[((uint32_t)y * APP_LCD_WIDTH) + x] = color;
        }
    }
}
