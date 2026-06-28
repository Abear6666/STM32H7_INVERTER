#include "main.h"

#include <stdio.h>

#include "app_diag.h"
#include "app_log.h"
#include "app_comm_bus.h"
#include "app_iap.h"
#include "app_settings.h"
#include "app_storage.h"
#include "app_system_model.h"
#include "app_tasks.h"
#include "app_vector.h"
#include "flash_layout.h"
#include "lcd.h"
#include "led.h"
#include "sdram.h"
#include "system_clock.h"
#include "touch.h"
#include "uart.h"
#include "usb_device.h"

#ifndef APP_BUILD_SLOT_NAME
#define APP_BUILD_SLOT_NAME "AppA"
#endif

#ifndef APP_BUILD_RUN_ADDR
#define APP_BUILD_RUN_ADDR APP_A_RUN_ADDR
#endif

#ifndef APP_BUILD_VERSION
#define APP_BUILD_VERSION 1U
#endif

static void app_print_clock_info(void);
static void app_run_sdram_test(void);

int main(void)
{
    app_storage_status_t storage_status;

    SCB_EnableICache();
    app_sdram_mpu_config();

    HAL_Init();
    app_diag_early_init();
    SystemClock_Config();
    app_led_init();
    app_uart1_init(115200);
    app_uart_log_mutex_init();
    app_vector_relocate_to_ram();
    __set_BASEPRI(0U);
    if (HAL_PWREx_EnableUSBReg() != HAL_OK)
    {
        printf("USB regulator enable failed\r\n");
    }
    HAL_PWREx_EnableUSBVoltageDetector();
    printf("USB CDC early init start\r\n");
    if (app_usb_device_init())
    {
        printf("USB CDC early init OK\r\n");
    }
    else
    {
        printf("USB CDC early init FAIL\r\n");
    }

    printf("\r\nApollo H743 LVGL HMI minimal bring-up\r\n");
    printf("APP: %s start from 0x%08lX version=%lu\r\n",
           APP_BUILD_SLOT_NAME,
           (unsigned long)APP_BUILD_RUN_ADDR,
           (unsigned long)APP_BUILD_VERSION);
    printf("Phase 10-11 %s: FreeRTOS + W25Q256 settings + HMI pages + local IAP\r\n",
           APP_BUILD_SLOT_NAME);
    printf("FreeRTOS: enabled, LVGL only in task_gui\r\n");
    app_diag_print_boot();
    app_print_clock_info();
    app_run_sdram_test();
    app_lcd_print_config();
    printf("LCD init start\r\n");
    app_lcd_init();
    app_lcd_fill_phase5_touch_background();
    app_lcd_backlight_on();
    printf("LCD init done: SDRAM framebuffer ready\r\n");

    printf("Touch init start\r\n");
    if (app_touch_init())
    {
        app_touch_print_config();
        printf("Touch init done\r\n");
    }
    else
    {
        printf("Touch init fail: GT9xxx not detected on 0x14/0x5D\r\n");
    }

    app_log_init();
    if (app_comm_bus_init())
    {
        printf("Phase 13 comm bus init OK\r\n");
    }
    else
    {
        printf("Phase 13 comm bus init FAIL\r\n");
    }
    app_system_model_init();
    app_log_event("Phase 13 system model ready");
    app_settings_init();

    printf("Storage init start\r\n");
    if (app_storage_init())
    {
        printf("Storage init done: settings loaded from W25Q256\r\n");
    }
    else
    {
        printf("Storage init done: default settings active or flash unavailable\r\n");
    }
    app_storage_get_status(&storage_status);
    app_iap_init(storage_status.flash_ready);
    (void)app_iap_confirm_current_app();

    app_tasks_start_scheduler();
}

static void app_print_clock_info(void)
{
    app_clock_info_t clk = app_clock_get_info();

    printf("clock source: HSE=25MHz, PLL1 M=5 N=160 P=2 Q=4\r\n");
    printf("SYSCLK=%lu Hz\r\n", (unsigned long)clk.sysclk_hz);
    printf("HCLK  =%lu Hz\r\n", (unsigned long)clk.hclk_hz);
    printf("APB1  =%lu Hz\r\n", (unsigned long)clk.pclk1_hz);
    printf("APB2  =%lu Hz\r\n", (unsigned long)clk.pclk2_hz);
    printf("APB3  =%lu Hz\r\n", (unsigned long)clk.pclk3_hz);
    printf("APB4  =%lu Hz\r\n", (unsigned long)clk.pclk4_hz);
    printf("PLL2_R=%lu Hz\r\n", (unsigned long)clk.pll2_r_hz);
    printf("FMC   =%lu Hz\r\n", (unsigned long)clk.fmc_kernel_hz);
    printf("SDCLK =%lu Hz\r\n", (unsigned long)clk.sdram_hz);
    printf("PLL3_R=%lu Hz\r\n", (unsigned long)clk.pll3_r_hz);
    printf("LTDC  =%lu Hz\r\n", (unsigned long)clk.ltdc_pixel_hz);
    printf("QSPI  =D1HCLK/2 = 100000000 Hz\r\n");
    printf("USB   =HSI48 = 48000000 Hz\r\n");
    printf("SDMMC =PLL1_Q = %lu Hz\r\n", (unsigned long)clk.sdmmc_kernel_hz);
    printf("ICache=on, DCache=off\r\n");
}

static void app_run_sdram_test(void)
{
    app_sdram_test_result_t result;

    app_sdram_print_config();
    printf("SDRAM init start\r\n");
    app_sdram_init();
    printf("SDRAM init done\r\n");
    printf("SDRAM memtest start: %lu bytes\r\n", (unsigned long)APP_SDRAM_SIZE_BYTES);

    if (!app_sdram_memtest(APP_SDRAM_SIZE_BYTES, &result))
    {
        printf("SDRAM test fail: stage=%s addr=0x%08lX expected=0x%08lX actual=0x%08lX tested=%lu bytes\r\n",
               result.stage,
               (unsigned long)result.address,
               (unsigned long)result.expected,
               (unsigned long)result.actual,
               (unsigned long)result.tested_bytes);
        Error_Handler();
    }

    printf("SDRAM test pass: %lu bytes\r\n", (unsigned long)result.tested_bytes);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        app_led1_toggle();
        for (volatile uint32_t i = 0; i < 2000000UL; ++i)
        {
        }
    }
}
