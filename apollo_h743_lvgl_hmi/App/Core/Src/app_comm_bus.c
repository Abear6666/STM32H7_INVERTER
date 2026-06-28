#include "app_comm_bus.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define APP_COMM_QUEUE_LENGTH 16U

static QueueHandle_t s_comm_queue;
static uint32_t s_dropped_count;

bool app_comm_bus_init(void)
{
    if (s_comm_queue == NULL)
    {
        s_comm_queue = xQueueCreate(APP_COMM_QUEUE_LENGTH, sizeof(app_comm_event_t));
    }

    s_dropped_count = 0U;
    return s_comm_queue != NULL;
}

bool app_comm_publish(const app_comm_event_t *event, uint32_t timeout_ms)
{
    TickType_t timeout_ticks = 0;

    if ((s_comm_queue == NULL) || (event == NULL))
    {
        s_dropped_count++;
        return false;
    }

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    }

    if (xQueueSend(s_comm_queue, event, timeout_ticks) != pdPASS)
    {
        s_dropped_count++;
        return false;
    }

    return true;
}

bool app_comm_receive(app_comm_event_t *event, uint32_t timeout_ms)
{
    TickType_t timeout_ticks = 0;

    if ((s_comm_queue == NULL) || (event == NULL))
    {
        return false;
    }

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    }

    return xQueueReceive(s_comm_queue, event, timeout_ticks) == pdPASS;
}

uint32_t app_comm_bus_dropped_count(void)
{
    return s_dropped_count;
}
