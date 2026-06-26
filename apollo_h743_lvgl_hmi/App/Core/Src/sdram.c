#include "sdram.h"

#include <stdio.h>

#include "main.h"

#define APP_SDRAM_REFRESH_COUNT 839UL

#define SDRAM_MODEREG_BURST_LENGTH_1 ((uint16_t)0x0000U)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL ((uint16_t)0x0000U)
#define SDRAM_MODEREG_CAS_LATENCY_2 ((uint16_t)0x0020U)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD ((uint16_t)0x0000U)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE ((uint16_t)0x0200U)

static SDRAM_HandleTypeDef s_sdram;

static void app_sdram_gpio_init(void);
static void app_sdram_initialization_sequence(void);
static void app_sdram_send_command(uint32_t mode, uint32_t refresh, uint32_t mode_register);
static bool app_sdram_fill_verify(volatile uint32_t *mem,
                                  uint32_t words,
                                  uint32_t pattern,
                                  const char *stage,
                                  app_sdram_test_result_t *result);
static bool app_sdram_address_test(volatile uint32_t *mem,
                                   uint32_t words,
                                   app_sdram_test_result_t *result);
static bool app_sdram_walking_bit_test(volatile uint32_t *mem,
                                       app_sdram_test_result_t *result);
static void app_sdram_set_failure(app_sdram_test_result_t *result,
                                  const char *stage,
                                  uintptr_t address,
                                  uint32_t expected,
                                  uint32_t actual,
                                  uint32_t tested_bytes);

void app_sdram_mpu_config(void)
{
    MPU_Region_InitTypeDef region = {0};

    HAL_MPU_Disable();

    region.Enable = MPU_REGION_ENABLE;
    region.Number = MPU_REGION_NUMBER6;
    region.BaseAddress = APP_SDRAM_BASE_ADDR;
    region.Size = MPU_REGION_SIZE_32MB;
    region.SubRegionDisable = 0x00;
    region.TypeExtField = MPU_TEX_LEVEL0;
    region.AccessPermission = MPU_REGION_FULL_ACCESS;
    region.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    region.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    region.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    region.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&region);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void app_sdram_init(void)
{
    FMC_SDRAM_TimingTypeDef timing = {0};

    s_sdram.Instance = FMC_SDRAM_DEVICE;
    s_sdram.Init.SDBank = FMC_SDRAM_BANK1;
    s_sdram.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_9;
    s_sdram.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;
    s_sdram.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
    s_sdram.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
    s_sdram.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_2;
    s_sdram.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    s_sdram.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
    s_sdram.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
    s_sdram.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1;

    timing.LoadToActiveDelay = 2;
    timing.ExitSelfRefreshDelay = 8;
    timing.SelfRefreshTime = 7;
    timing.RowCycleDelay = 7;
    timing.WriteRecoveryTime = 2;
    timing.RPDelay = 2;
    timing.RCDDelay = 2;

    if (HAL_SDRAM_Init(&s_sdram, &timing) != HAL_OK)
    {
        Error_Handler();
    }

    app_sdram_initialization_sequence();

    if (HAL_SDRAM_ProgramRefreshRate(&s_sdram, APP_SDRAM_REFRESH_COUNT) != HAL_OK)
    {
        Error_Handler();
    }
}

bool app_sdram_memtest(size_t bytes, app_sdram_test_result_t *result)
{
    volatile uint32_t *mem = (volatile uint32_t *)APP_SDRAM_BASE_ADDR;
    uint32_t words = (uint32_t)(bytes / sizeof(uint32_t));

    if (result != NULL)
    {
        result->passed = false;
        result->stage = "not-started";
        result->address = APP_SDRAM_BASE_ADDR;
        result->expected = 0;
        result->actual = 0;
        result->tested_bytes = (uint32_t)(words * sizeof(uint32_t));
    }

    if (words == 0U)
    {
        app_sdram_set_failure(result, "size-check", APP_SDRAM_BASE_ADDR, 1U, 0U, 0U);
        return false;
    }

    if (!app_sdram_walking_bit_test(mem, result))
    {
        return false;
    }

    if (!app_sdram_fill_verify(mem, words, 0x55555555UL, "pattern-0x55555555", result))
    {
        return false;
    }

    if (!app_sdram_fill_verify(mem, words, 0xAAAAAAAAUL, "pattern-0xAAAAAAAA", result))
    {
        return false;
    }

    if (!app_sdram_address_test(mem, words, result))
    {
        return false;
    }

    if (result != NULL)
    {
        result->passed = true;
        result->stage = "pass";
        result->address = APP_SDRAM_BASE_ADDR;
        result->expected = 0;
        result->actual = 0;
        result->tested_bytes = (uint32_t)(words * sizeof(uint32_t));
    }

    return true;
}

void app_sdram_print_config(void)
{
    printf("SDRAM base=0x%08lX size=%lu bytes\r\n",
           (unsigned long)APP_SDRAM_BASE_ADDR,
           (unsigned long)APP_SDRAM_SIZE_BYTES);
    printf("SDRAM FMC: bank1, 16-bit, col=9, row=13, banks=4, CAS=2\r\n");
    printf("SDRAM clock: FMC kernel=220000000 Hz, SDCLK=110000000 Hz, refresh=%lu\r\n",
           (unsigned long)APP_SDRAM_REFRESH_COUNT);
    printf("SDRAM MPU: 32MB non-cacheable, non-bufferable, execute-never\r\n");
}

void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef *hsdram)
{
    if (hsdram->Instance == FMC_SDRAM_DEVICE)
    {
        app_sdram_gpio_init();
    }
}

static void app_sdram_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_FMC_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF12_FMC;

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3;
    HAL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 |
               GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 |
               GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
               GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 |
               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
               GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOG, &gpio);
}

static void app_sdram_initialization_sequence(void)
{
    uint32_t mode_register = 0;

    app_sdram_send_command(FMC_SDRAM_CMD_CLK_ENABLE, 1, 0);
    HAL_Delay(1);
    app_sdram_send_command(FMC_SDRAM_CMD_PALL, 1, 0);
    app_sdram_send_command(FMC_SDRAM_CMD_AUTOREFRESH_MODE, 8, 0);

    mode_register = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1 |
                    (uint32_t)SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |
                    (uint32_t)SDRAM_MODEREG_CAS_LATENCY_2 |
                    (uint32_t)SDRAM_MODEREG_OPERATING_MODE_STANDARD |
                    (uint32_t)SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    app_sdram_send_command(FMC_SDRAM_CMD_LOAD_MODE, 1, mode_register);
}

static void app_sdram_send_command(uint32_t mode, uint32_t refresh, uint32_t mode_register)
{
    FMC_SDRAM_CommandTypeDef command = {0};

    command.CommandMode = mode;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = refresh;
    command.ModeRegisterDefinition = mode_register;

    if (HAL_SDRAM_SendCommand(&s_sdram, &command, 0x1000U) != HAL_OK)
    {
        Error_Handler();
    }
}

static bool app_sdram_fill_verify(volatile uint32_t *mem,
                                  uint32_t words,
                                  uint32_t pattern,
                                  const char *stage,
                                  app_sdram_test_result_t *result)
{
    for (uint32_t i = 0; i < words; ++i)
    {
        mem[i] = pattern;
    }

    __DSB();

    for (uint32_t i = 0; i < words; ++i)
    {
        uint32_t actual = mem[i];

        if (actual != pattern)
        {
            app_sdram_set_failure(result,
                                  stage,
                                  APP_SDRAM_BASE_ADDR + ((uintptr_t)i * sizeof(uint32_t)),
                                  pattern,
                                  actual,
                                  (uint32_t)(words * sizeof(uint32_t)));
            return false;
        }
    }

    return true;
}

static bool app_sdram_address_test(volatile uint32_t *mem,
                                   uint32_t words,
                                   app_sdram_test_result_t *result)
{
    for (uint32_t i = 0; i < words; ++i)
    {
        mem[i] = APP_SDRAM_BASE_ADDR + (i * sizeof(uint32_t));
    }

    __DSB();

    for (uint32_t i = 0; i < words; ++i)
    {
        uint32_t expected = APP_SDRAM_BASE_ADDR + (i * sizeof(uint32_t));
        uint32_t actual = mem[i];

        if (actual != expected)
        {
            app_sdram_set_failure(result,
                                  "address",
                                  APP_SDRAM_BASE_ADDR + ((uintptr_t)i * sizeof(uint32_t)),
                                  expected,
                                  actual,
                                  (uint32_t)(words * sizeof(uint32_t)));
            return false;
        }
    }

    return true;
}

static bool app_sdram_walking_bit_test(volatile uint32_t *mem,
                                       app_sdram_test_result_t *result)
{
    for (uint32_t bit = 0; bit < 32U; ++bit)
    {
        uint32_t pattern = 1UL << bit;
        mem[bit] = pattern;
    }

    __DSB();

    for (uint32_t bit = 0; bit < 32U; ++bit)
    {
        uint32_t expected = 1UL << bit;
        uint32_t actual = mem[bit];

        if (actual != expected)
        {
            app_sdram_set_failure(result,
                                  "walking-bit",
                                  APP_SDRAM_BASE_ADDR + ((uintptr_t)bit * sizeof(uint32_t)),
                                  expected,
                                  actual,
                                  32U * sizeof(uint32_t));
            return false;
        }
    }

    return true;
}

static void app_sdram_set_failure(app_sdram_test_result_t *result,
                                  const char *stage,
                                  uintptr_t address,
                                  uint32_t expected,
                                  uint32_t actual,
                                  uint32_t tested_bytes)
{
    if (result != NULL)
    {
        result->passed = false;
        result->stage = stage;
        result->address = address;
        result->expected = expected;
        result->actual = actual;
        result->tested_bytes = tested_bytes;
    }
}
