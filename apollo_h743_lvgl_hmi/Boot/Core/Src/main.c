#include "main.h"

#include <stdio.h>

#include "app_tag.h"
#include "boot_iap.h"
#include "boot_jump.h"
#include "led.h"
#include "system_clock.h"
#include "uart.h"

static void boot_print_clock_info(void);
static bool boot_validate_and_jump_slot(app_slot_id_t slot_id);

int main(void)
{
    app_slot_id_t selected_slot;

    SCB_EnableICache();

    HAL_Init();
    SystemClock_Config();
    app_led_init();
    app_uart1_init(115200);

    printf("\r\nBOOT: Apollo H743 bootloader start\r\n");
    printf("BOOT: Phase 10-11 CRC32 Boot/App/IAP build\r\n");
    boot_print_clock_info();
    printf("BOOT: partition Boot=0x%08lX..0x%08lX AppA=0x%08lX run=0x%08lX AppB=0x%08lX run=0x%08lX\r\n",
           (unsigned long)BOOT_START_ADDR,
           (unsigned long)BOOT_END_ADDR,
           (unsigned long)APP_A_START_ADDR,
           (unsigned long)APP_A_RUN_ADDR,
           (unsigned long)APP_B_START_ADDR,
           (unsigned long)APP_B_RUN_ADDR);

    if (boot_iap_try_apply_pending())
    {
        printf("BOOT: pending package applied to AppB\r\n");
    }

    selected_slot = boot_iap_select_boot_slot();
    printf("BOOT: selected slot=%s\r\n", selected_slot == APP_SLOT_B ? "AppB" : "AppA");

    if (boot_validate_and_jump_slot(selected_slot))
    {
        while (1)
        {
        }
    }

    boot_iap_report_slot_invalid(selected_slot);

    if (selected_slot == APP_SLOT_A)
    {
        printf("BOOT: AppA unavailable; AppB will not be used without confirmed/trial state\r\n");
    }
    else if (selected_slot == APP_SLOT_B)
    {
        printf("BOOT: AppB unavailable, try AppA fallback\r\n");
        if (boot_validate_and_jump_slot(APP_SLOT_A))
        {
            while (1)
            {
            }
        }
    }
    else
    {
        printf("BOOT: invalid selected slot\r\n");
    }

    printf("BOOT: no valid App, stay in bootloader\r\n");
    while (1)
    {
        app_led1_toggle();
        HAL_Delay(250);
    }
}

static void boot_print_clock_info(void)
{
    app_clock_info_t clk = app_clock_get_info();

    printf("BOOT: SYSCLK=%lu HCLK=%lu APB1=%lu APB2=%lu\r\n",
           (unsigned long)clk.sysclk_hz,
           (unsigned long)clk.hclk_hz,
           (unsigned long)clk.pclk1_hz,
           (unsigned long)clk.pclk2_hz);
}

static bool boot_validate_and_jump_slot(app_slot_id_t slot_id)
{
    const app_slot_info_t *slot = app_get_slot_info(slot_id);
    const app_tag_t *tag;
    uint32_t calc_crc;

    if (slot == NULL)
    {
        return false;
    }

    tag = (const app_tag_t *)slot->tag_addr;
    printf("BOOT: check %s tag at 0x%08lX\r\n",
           slot->name,
           (unsigned long)slot->tag_addr);
    printf("BOOT: tag magic=0x%08lX size=%lu run=0x%08lX app_size=%lu version=%lu crc=0x%08lX\r\n",
           (unsigned long)tag->magic,
           (unsigned long)tag->tag_size,
           (unsigned long)tag->app_run_addr,
           (unsigned long)tag->app_size,
           (unsigned long)tag->version,
           (unsigned long)tag->crc32);

    if (!app_tag_basic_valid(tag, slot))
    {
        printf("BOOT: %s tag invalid\r\n", slot->name);
        return false;
    }

    calc_crc = app_crc32_memory(tag->app_run_addr, tag->app_size);
    printf("BOOT: %s crc calc=0x%08lX expect=0x%08lX\r\n",
           slot->name,
           (unsigned long)calc_crc,
           (unsigned long)tag->crc32);
    if (calc_crc != tag->crc32)
    {
        printf("BOOT: %s crc mismatch\r\n", slot->name);
        return false;
    }

    printf("BOOT: %s valid, version=%lu\r\n",
           slot->name,
           (unsigned long)tag->version);
    return boot_jump_to_app(slot->run_addr);
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
