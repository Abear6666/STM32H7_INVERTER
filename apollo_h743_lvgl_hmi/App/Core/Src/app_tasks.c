#include "app_tasks.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "app_iap.h"
#include "app_iap_record.h"
#include "app_log.h"
#include "app_settings.h"
#include "app_storage.h"
#include "app_ui_hmi.h"
#include "led.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "main.h"
#include "task.h"

#define TASK_GUI_STACK_WORDS     3072U
#define TASK_STORAGE_STACK_WORDS 1024U
#define TASK_LOG_STACK_WORDS     1024U
#define TASK_IAP_STACK_WORDS     2048U

static TaskHandle_t s_task_gui;
static TaskHandle_t s_task_storage;
static TaskHandle_t s_task_log;
static TaskHandle_t s_task_iap;

static void task_gui(void *argument);
static void task_storage(void *argument);
static void task_log(void *argument);
static void task_iap(void *argument);
static void app_tasks_print_stack_watermarks(void);

void app_tasks_start_scheduler(void)
{
    BaseType_t ok;

    ok = xTaskCreate(task_gui,
                     "task_gui",
                     TASK_GUI_STACK_WORDS,
                     NULL,
                     tskIDLE_PRIORITY + 3U,
                     &s_task_gui);
    configASSERT(ok == pdPASS);

    ok = xTaskCreate(task_storage,
                     "task_storage",
                     TASK_STORAGE_STACK_WORDS,
                     NULL,
                     tskIDLE_PRIORITY + 1U,
                     &s_task_storage);
    configASSERT(ok == pdPASS);

    ok = xTaskCreate(task_log,
                     "task_log",
                     TASK_LOG_STACK_WORDS,
                     NULL,
                     tskIDLE_PRIORITY + 1U,
                     &s_task_log);
    configASSERT(ok == pdPASS);

    ok = xTaskCreate(task_iap,
                     "task_iap",
                     TASK_IAP_STACK_WORDS,
                     NULL,
                     tskIDLE_PRIORITY + 1U,
                     &s_task_iap);
    configASSERT(ok == pdPASS);

    printf("FreeRTOS scheduler start\r\n");
    vTaskStartScheduler();

    Error_Handler();
}

void app_tasks_get_runtime_status(app_task_runtime_status_t *status)
{
    if (status == NULL)
    {
        return;
    }

    status->gui_stack_free_words = (s_task_gui != NULL) ? (uint32_t)uxTaskGetStackHighWaterMark(s_task_gui) : 0U;
    status->storage_stack_free_words = (s_task_storage != NULL) ? (uint32_t)uxTaskGetStackHighWaterMark(s_task_storage) : 0U;
    status->log_stack_free_words = (s_task_log != NULL) ? (uint32_t)uxTaskGetStackHighWaterMark(s_task_log) : 0U;
    status->iap_stack_free_words = (s_task_iap != NULL) ? (uint32_t)uxTaskGetStackHighWaterMark(s_task_iap) : 0U;
    status->idle_stack_free_words = (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) ?
                                    (uint32_t)uxTaskGetStackHighWaterMark(xTaskGetIdleTaskHandle()) :
                                    0U;
    status->heap_free_bytes = (uint32_t)xPortGetFreeHeapSize();
    status->heap_min_free_bytes = (uint32_t)xPortGetMinimumEverFreeHeapSize();
}

void vAssertCalled(const char *file, int line)
{
    printf("FreeRTOS assert: %s:%d\r\n", file, line);
    Error_Handler();
}

void vApplicationMallocFailedHook(void)
{
    printf("FreeRTOS malloc failed\r\n");
    Error_Handler();
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    printf("FreeRTOS stack overflow: %s\r\n", task_name);
    Error_Handler();
}

static void task_gui(void *argument)
{
    uint32_t last_lv_tick;

    (void)argument;

    printf("task_gui started\r\n");
    app_log_event("task_gui started");

    lv_init();
    last_lv_tick = HAL_GetTick();
    app_lvgl_port_init();
    app_lvgl_port_print_config();
    app_ui_hmi_create();
    printf("LVGL init done: HMI pages created in task_gui\r\n");
    app_log_event("HMI pages created");

    while (1)
    {
        uint32_t now = HAL_GetTick();
        uint32_t elapsed = now - last_lv_tick;
        uint32_t wait_ms;

        if (elapsed > 0U)
        {
            lv_tick_inc(elapsed);
            last_lv_tick = now;
        }

        wait_ms = lv_timer_handler();
        app_ui_hmi_tick(now);

        if ((wait_ms == LV_NO_TIMER_READY) || (wait_ms > 10U))
        {
            wait_ms = 10U;
        }
        if (wait_ms == 0U)
        {
            wait_ms = 1U;
        }

        vTaskDelay(pdMS_TO_TICKS(wait_ms));
    }
}

static void task_storage(void *argument)
{
    app_storage_task(argument);
}

static void task_log(void *argument)
{
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t seconds = 0;

    (void)argument;
    printf("task_log started\r\n");
    app_log_event("task_log started");

    while (1)
    {
        app_led0_toggle();
        seconds++;

        if ((seconds % 5U) == 0U)
        {
            app_tasks_print_stack_watermarks();
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1000U));
    }
}

static void task_iap(void *argument)
{
    app_iap_task(argument);
}

static void app_tasks_print_stack_watermarks(void)
{
    app_storage_status_t storage;
    app_settings_snapshot_t settings;
    app_iap_status_t iap;
    app_task_runtime_status_t runtime;

    app_storage_get_status(&storage);
    app_settings_get(&settings);
    app_iap_get_status(&iap);
    app_tasks_get_runtime_status(&runtime);

    printf("stack watermark words: gui=%lu storage=%lu log=%lu iap=%lu idle=%lu heap_free=%lu\r\n",
           (unsigned long)runtime.gui_stack_free_words,
           (unsigned long)runtime.storage_stack_free_words,
           (unsigned long)runtime.log_stack_free_words,
           (unsigned long)runtime.iap_stack_free_words,
           (unsigned long)runtime.idle_stack_free_words,
           (unsigned long)runtime.heap_free_bytes);
    printf("storage status: flash=%u id=0x%06lX load=%s save_ok=%lu save_fail=%lu dirty=%u\r\n",
           storage.flash_ready ? 1U : 0U,
           (unsigned long)storage.jedec_id,
           storage.load_result,
           (unsigned long)storage.save_ok_count,
           (unsigned long)storage.save_fail_count,
           app_settings_is_dirty() ? 1U : 0U);
    printf("settings snapshot: brightness=%lu threshold=%ld output=%lu language=%lu selftest=0x%08lX\r\n",
           (unsigned long)settings.brightness,
           (long)settings.threshold,
           (unsigned long)settings.output_enabled,
           (unsigned long)settings.language,
           (unsigned long)settings.save_selftest_cookie);
    printf("iap status: flash=%u pending=%u version=%lu size=%lu crc=0x%08lX recv=%u %lu/%lu state=%s\r\n",
           iap.flash_ready ? 1U : 0U,
           iap.pending_valid ? 1U : 0U,
           (unsigned long)iap.app_b_version,
           (unsigned long)iap.app_b_size,
           (unsigned long)iap.app_b_crc32,
           iap.recv_active ? 1U : 0U,
           (unsigned long)iap.recv_received,
           (unsigned long)iap.recv_expected,
           iap.status);
    printf("iap boot state: valid=%u state=%s active=%lu trial=%lu attempts=%lu/%lu err=%lu\r\n",
           iap.boot_state_valid ? 1U : 0U,
           app_iap_boot_state_name(iap.boot_state),
           (unsigned long)iap.boot_active_slot,
           (unsigned long)iap.boot_trial_slot,
           (unsigned long)iap.boot_attempts,
           (unsigned long)iap.boot_max_attempts,
           (unsigned long)iap.boot_last_error);
}
