#include "app_critical.h"

#include "FreeRTOS.h"
#include "main.h"
#include "task.h"

uint32_t app_critical_enter(void)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        taskENTER_CRITICAL();
        return UINT32_MAX;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void app_critical_exit(uint32_t primask)
{
    if (primask == UINT32_MAX)
    {
        taskEXIT_CRITICAL();
        return;
    }

    if ((primask & 1U) == 0U)
    {
        __enable_irq();
    }
}
