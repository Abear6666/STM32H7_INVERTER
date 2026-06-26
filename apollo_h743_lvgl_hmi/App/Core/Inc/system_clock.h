#ifndef APP_SYSTEM_CLOCK_H
#define APP_SYSTEM_CLOCK_H

#include <stdint.h>

typedef struct
{
    uint32_t sysclk_hz;
    uint32_t hclk_hz;
    uint32_t pclk1_hz;
    uint32_t pclk2_hz;
    uint32_t pclk3_hz;
    uint32_t pclk4_hz;
    uint32_t pll2_r_hz;
    uint32_t fmc_kernel_hz;
    uint32_t sdram_hz;
    uint32_t pll3_r_hz;
    uint32_t ltdc_pixel_hz;
    uint32_t sdmmc_kernel_hz;
} app_clock_info_t;

void SystemClock_Config(void);
app_clock_info_t app_clock_get_info(void);

#endif
