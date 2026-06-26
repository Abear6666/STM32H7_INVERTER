#include "stm32h7xx_it.h"

#include "FreeRTOS.h"
#include "app_diag.h"
#include "stm32h7xx_hal.h"
#include "task.h"

void xPortSysTickHandler(void);
extern PCD_HandleTypeDef hpcd_USB_FS;

void app_diag_fault_entry(uint32_t *stacked_sp, uint32_t exc_return, uint32_t fault_type);

void NMI_Handler(void)
{
    while (1)
    {
    }
}

__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "movs r2, %[fault_type]\n"
        "b app_diag_fault_entry\n"
        :
        : [fault_type] "I"(APP_DIAG_FAULT_HARDFAULT)
        : "r0", "r1", "r2");
}

__attribute__((naked)) void MemManage_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "movs r2, %[fault_type]\n"
        "b app_diag_fault_entry\n"
        :
        : [fault_type] "I"(APP_DIAG_FAULT_MEMMANAGE)
        : "r0", "r1", "r2");
}

__attribute__((naked)) void BusFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "movs r2, %[fault_type]\n"
        "b app_diag_fault_entry\n"
        :
        : [fault_type] "I"(APP_DIAG_FAULT_BUSFAULT)
        : "r0", "r1", "r2");
}

__attribute__((naked)) void UsageFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "mov r1, lr\n"
        "movs r2, %[fault_type]\n"
        "b app_diag_fault_entry\n"
        :
        : [fault_type] "I"(APP_DIAG_FAULT_USAGEFAULT)
        : "r0", "r1", "r2");
}

void DebugMon_Handler(void)
{
}

void SysTick_Handler(void)
{
    HAL_IncTick();

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
}

void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

void app_diag_fault_entry(uint32_t *stacked_sp, uint32_t exc_return, uint32_t fault_type)
{
    app_diag_record_fault((app_diag_fault_type_t)fault_type, stacked_sp, exc_return);
    NVIC_SystemReset();

    while (1)
    {
    }
}
