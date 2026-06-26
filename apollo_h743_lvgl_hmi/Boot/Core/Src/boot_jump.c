#include "boot_jump.h"

#include <stdio.h>

#include "main.h"

typedef void (*boot_app_entry_t)(void);

static bool boot_stack_pointer_valid(uint32_t sp)
{
    return (((sp >= 0x20000000UL) && (sp <= 0x20020000UL)) ||
            ((sp >= 0x24000000UL) && (sp <= 0x24080000UL)) ||
            ((sp >= 0x30000000UL) && (sp <= 0x30048000UL)) ||
            ((sp >= 0x38000000UL) && (sp <= 0x38010000UL)));
}

static bool boot_reset_handler_valid(uint32_t reset)
{
    return ((reset >= 0x08000000UL) && (reset <= 0x081FFFFFUL) && ((reset & 1UL) == 1UL));
}

bool boot_jump_to_app(uint32_t app_run_addr)
{
    uint32_t app_msp = *(volatile uint32_t *)(app_run_addr + 0U);
    uint32_t app_reset = *(volatile uint32_t *)(app_run_addr + 4U);

    printf("BOOT: vector msp=0x%08lX reset=0x%08lX\r\n",
           (unsigned long)app_msp,
           (unsigned long)app_reset);

    if (!boot_stack_pointer_valid(app_msp) || !boot_reset_handler_valid(app_reset))
    {
        printf("BOOT: invalid App vector, stay in Boot\r\n");
        return false;
    }

    printf("BOOT: jump to App at 0x%08lX\r\n", (unsigned long)app_run_addr);
    HAL_Delay(20);

    __disable_irq();
    HAL_RCC_DeInit();
    HAL_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    for (uint32_t i = 0; i < 8U; ++i)
    {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }

    SCB->VTOR = app_run_addr;
    __DSB();
    __ISB();
    __set_BASEPRI(0U);
    __set_FAULTMASK(0U);
    __set_CONTROL(0U);
    __ISB();
    __set_MSP(app_msp);
    __enable_irq();

    ((boot_app_entry_t)app_reset)();
    return true;
}
