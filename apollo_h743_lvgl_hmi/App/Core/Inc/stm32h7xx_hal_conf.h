#ifndef STM32H7XX_HAL_CONF_H
#define STM32H7XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_LTDC_MODULE_ENABLED
#define HAL_MDMA_MODULE_ENABLED
#define HAL_PCD_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_QSPI_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_SDRAM_MODULE_ENABLED
#define HAL_SD_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

#if !defined(HSE_VALUE)
#define HSE_VALUE (25000000UL)
#endif

#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT (100UL)
#endif

#if !defined(CSI_VALUE)
#define CSI_VALUE (4000000UL)
#endif

#if !defined(HSI_VALUE)
#define HSI_VALUE (64000000UL)
#endif

#if !defined(LSE_VALUE)
#define LSE_VALUE (32768UL)
#endif

#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT (5000UL)
#endif

#if !defined(LSI_VALUE)
#define LSI_VALUE (32000UL)
#endif

#if !defined(EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE (12288000UL)
#endif

#define VDD_VALUE (3300UL)
#define TICK_INT_PRIORITY (15UL)
#define USE_RTOS 0U
#define USE_SD_TRANSCEIVER 0U
#define USE_SPI_CRC 0U

#define USE_HAL_UART_REGISTER_CALLBACKS 0U
#define USE_HAL_PCD_REGISTER_CALLBACKS 0U
#define USE_HAL_SD_REGISTER_CALLBACKS 0U

#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32h7xx_hal_rcc.h"
#include "stm32h7xx_hal_rcc_ex.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32h7xx_hal_gpio.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32h7xx_hal_flash.h"
#include "stm32h7xx_hal_flash_ex.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32h7xx_hal_dma.h"
#include "stm32h7xx_hal_dma_ex.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32h7xx_hal_cortex.h"
#endif

#ifdef HAL_MDMA_MODULE_ENABLED
#include "stm32h7xx_hal_mdma.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32h7xx_hal_pwr.h"
#include "stm32h7xx_hal_pwr_ex.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32h7xx_hal_uart.h"
#include "stm32h7xx_hal_uart_ex.h"
#endif

#ifdef HAL_SDRAM_MODULE_ENABLED
#include "stm32h7xx_hal_sdram.h"
#endif

#ifdef HAL_SD_MODULE_ENABLED
#include "stm32h7xx_hal_sd.h"
#include "stm32h7xx_hal_sd_ex.h"
#endif

#ifdef HAL_LTDC_MODULE_ENABLED
#include "stm32h7xx_hal_ltdc.h"
#include "stm32h7xx_hal_ltdc_ex.h"
#endif

#ifdef HAL_QSPI_MODULE_ENABLED
#include "stm32h7xx_hal_qspi.h"
#endif

#ifdef HAL_PCD_MODULE_ENABLED
#include "stm32h7xx_hal_pcd.h"
#include "stm32h7xx_hal_pcd_ex.h"
#endif

#define assert_param(expr) ((void)0U)

#ifdef __cplusplus
}
#endif

#endif
