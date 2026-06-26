#include "system_clock.h"

#include "main.h"

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    RCC_PeriphCLKInitTypeDef periph_clk = {0};

    MODIFY_REG(PWR->CR3, PWR_CR3_SCUEN, 0U);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while ((PWR->D3CR & PWR_D3CR_VOSRDY) != PWR_D3CR_VOSRDY)
    {
    }

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE
#ifdef APP_USB_CDC_ENABLED
                        | RCC_OSCILLATORTYPE_HSI48
#endif
                        ;
    osc.HSEState = RCC_HSE_ON;
    osc.HSI48State = RCC_HSI48_ON;
    osc.HSIState = RCC_HSI_OFF;
    osc.CSIState = RCC_CSI_OFF;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 5;
    osc.PLL.PLLN = 160;
    osc.PLL.PLLP = 2;
    osc.PLL.PLLQ = 4;
    osc.PLL.PLLR = 2;
    osc.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    osc.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    osc.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK
                  | RCC_CLOCKTYPE_HCLK
                  | RCC_CLOCKTYPE_PCLK1
                  | RCC_CLOCKTYPE_PCLK2
                  | RCC_CLOCKTYPE_D1PCLK1
                  | RCC_CLOCKTYPE_D3PCLK1;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.SYSCLKDivider = RCC_SYSCLK_DIV1;
    clk.AHBCLKDivider = RCC_HCLK_DIV2;
    clk.APB1CLKDivider = RCC_APB1_DIV2;
    clk.APB2CLKDivider = RCC_APB2_DIV2;
    clk.APB3CLKDivider = RCC_APB3_DIV2;
    clk.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }

    periph_clk.PeriphClockSelection = RCC_PERIPHCLK_FMC
                                    | RCC_PERIPHCLK_LTDC
                                    | RCC_PERIPHCLK_QSPI
                                    | RCC_PERIPHCLK_SDMMC
#ifdef APP_USB_CDC_ENABLED
                                    | RCC_PERIPHCLK_USB
#endif
                                    ;
    periph_clk.PLL2.PLL2M = 25;
    periph_clk.PLL2.PLL2N = 440;
    periph_clk.PLL2.PLL2P = 2;
    periph_clk.PLL2.PLL2Q = 2;
    periph_clk.PLL2.PLL2R = 2;
    periph_clk.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_0;
    periph_clk.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
    periph_clk.PLL2.PLL2FRACN = 0;
    periph_clk.PLL3.PLL3M = 25;
    periph_clk.PLL3.PLL3N = 300;
    periph_clk.PLL3.PLL3P = 2;
    periph_clk.PLL3.PLL3Q = 2;
    periph_clk.PLL3.PLL3R = 9;
    periph_clk.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_0;
    periph_clk.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
    periph_clk.PLL3.PLL3FRACN = 0;
    periph_clk.FmcClockSelection = RCC_FMCCLKSOURCE_PLL2;
    periph_clk.QspiClockSelection = RCC_QSPICLKSOURCE_D1HCLK;
    periph_clk.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL;
#ifdef APP_USB_CDC_ENABLED
    periph_clk.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
#endif
    if (HAL_RCCEx_PeriphCLKConfig(&periph_clk) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_RCC_CSI_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    HAL_EnableCompensationCell();
}

app_clock_info_t app_clock_get_info(void)
{
    app_clock_info_t info;

    info.sysclk_hz = HAL_RCC_GetSysClockFreq();
    info.hclk_hz = HAL_RCC_GetHCLKFreq();
    info.pclk1_hz = HAL_RCC_GetPCLK1Freq();
    info.pclk2_hz = HAL_RCC_GetPCLK2Freq();
    info.pclk3_hz = HAL_RCCEx_GetD1PCLK1Freq();
    info.pclk4_hz = HAL_RCCEx_GetD3PCLK1Freq();
    info.pll2_r_hz = 220000000UL;
    info.fmc_kernel_hz = info.pll2_r_hz;
    info.sdram_hz = info.fmc_kernel_hz / 2UL;
    info.pll3_r_hz = 33333333UL;
    info.ltdc_pixel_hz = info.pll3_r_hz;
    info.sdmmc_kernel_hz = 200000000UL;

    return info;
}
