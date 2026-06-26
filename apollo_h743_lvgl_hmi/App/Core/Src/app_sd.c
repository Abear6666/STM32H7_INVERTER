#include "app_sd.h"

#include <stdio.h>

#include "ff.h"

#define APP_SD_TIMEOUT_MS       5000U
#define APP_SD_CLOCK_DIV        SDMMC_NSPEED_CLK_DIV

static SD_HandleTypeDef s_sd_handle;
static HAL_SD_CardInfoTypeDef s_sd_card_info;
static FATFS s_sd_fs;
static bool s_sd_card_initialized;
static bool s_sd_mounted;

static bool app_sd_init_card(void);
static bool app_sd_wait_transfer_ready(void);

bool app_sd_mount(void)
{
    FRESULT res;

    if (s_sd_mounted)
    {
        return true;
    }

    res = f_mount(&s_sd_fs, APP_SD_MOUNT_PATH, 1);
    if (res != FR_OK)
    {
        printf("SD: mount failed, fresult=%u\r\n", (unsigned int)res);
        return false;
    }

    s_sd_mounted = true;
    printf("SD: mounted at %s, blocks=%lu block_size=%lu capacity=%lu MB\r\n",
           APP_SD_MOUNT_PATH,
           (unsigned long)s_sd_card_info.LogBlockNbr,
           (unsigned long)s_sd_card_info.LogBlockSize,
           (unsigned long)(((uint64_t)s_sd_card_info.LogBlockNbr * s_sd_card_info.LogBlockSize) >> 20));
    return true;
}

void app_sd_unmount(void)
{
    (void)f_mount(NULL, APP_SD_MOUNT_PATH, 0);
    s_sd_mounted = false;
}

bool app_sd_initialize(void)
{
    return app_sd_init_card();
}

bool app_sd_is_mounted(void)
{
    return s_sd_mounted;
}

bool app_sd_is_card_ready(void)
{
    if (!s_sd_card_initialized)
    {
        return false;
    }

    return HAL_SD_GetCardState(&s_sd_handle) == HAL_SD_CARD_TRANSFER;
}

bool app_sd_get_card_info(HAL_SD_CardInfoTypeDef *info)
{
    if ((info == NULL) || !s_sd_card_initialized)
    {
        return false;
    }

    *info = s_sd_card_info;
    return true;
}

bool app_sd_read_blocks(uint8_t *buffer, uint32_t sector, uint32_t count)
{
    if ((buffer == NULL) || (count == 0U) || !app_sd_init_card())
    {
        return false;
    }

    if (HAL_SD_ReadBlocks(&s_sd_handle, buffer, sector, count, APP_SD_TIMEOUT_MS) != HAL_OK)
    {
        return false;
    }

    return app_sd_wait_transfer_ready();
}

bool app_sd_write_blocks(const uint8_t *buffer, uint32_t sector, uint32_t count)
{
    if ((buffer == NULL) || (count == 0U) || !app_sd_init_card())
    {
        return false;
    }

    if (HAL_SD_WriteBlocks(&s_sd_handle, (uint8_t *)buffer, sector, count, APP_SD_TIMEOUT_MS) != HAL_OK)
    {
        return false;
    }

    return app_sd_wait_transfer_ready();
}

static bool app_sd_init_card(void)
{
    if (s_sd_card_initialized)
    {
        return true;
    }

    s_sd_handle.Instance = SDMMC1;
    s_sd_handle.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    s_sd_handle.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    s_sd_handle.Init.BusWide = SDMMC_BUS_WIDE_4B;
    s_sd_handle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
    s_sd_handle.Init.ClockDiv = APP_SD_CLOCK_DIV;

    if (HAL_SD_Init(&s_sd_handle) != HAL_OK)
    {
        printf("SD: HAL_SD_Init failed err=0x%08lX\r\n", (unsigned long)HAL_SD_GetError(&s_sd_handle));
        return false;
    }

    if (HAL_SD_GetCardInfo(&s_sd_handle, &s_sd_card_info) != HAL_OK)
    {
        printf("SD: HAL_SD_GetCardInfo failed err=0x%08lX\r\n", (unsigned long)HAL_SD_GetError(&s_sd_handle));
        return false;
    }

    s_sd_card_initialized = true;
    return true;
}

static bool app_sd_wait_transfer_ready(void)
{
    uint32_t start = HAL_GetTick();

    while (HAL_SD_GetCardState(&s_sd_handle) != HAL_SD_CARD_TRANSFER)
    {
        if ((HAL_GetTick() - start) > APP_SD_TIMEOUT_MS)
        {
            return false;
        }
    }

    return true;
}

void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
    GPIO_InitTypeDef gpio = {0};

    if (hsd->Instance != SDMMC1)
    {
        return;
    }

    __HAL_RCC_SDMMC1_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF12_SDIO1;

    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    HAL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &gpio);
}
