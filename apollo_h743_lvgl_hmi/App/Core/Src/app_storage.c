#include "app_storage.h"

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app_critical.h"
#include "app_log.h"
#include "app_settings.h"
#include "app_w25q256.h"
#include "task.h"

static app_storage_status_t s_storage_status;
static app_workset_block_t s_storage_block;
static app_workset_block_t s_storage_verify_block;

static void app_storage_set_status_text(const char *text);
static bool app_storage_load_settings(void);
static bool app_storage_save_block(const app_workset_block_t *block);

bool app_storage_init(void)
{
    memset(&s_storage_status, 0, sizeof(s_storage_status));
    app_storage_set_status_text("init");

    s_storage_status.flash_ready = app_w25q256_init();
    s_storage_status.jedec_id = app_w25q256_get_jedec_id();

    if (!s_storage_status.flash_ready)
    {
        (void)strncpy(s_storage_status.load_result, "flash_fail", sizeof(s_storage_status.load_result) - 1U);
        app_storage_set_status_text("flash init fail");
        app_log_event("W25Q init fail id=0x%06lX", (unsigned long)s_storage_status.jedec_id);
        return false;
    }

    (void)app_storage_load_settings();

    if (app_settings_run_save_selftest_once())
    {
        app_storage_set_status_text("selftest dirty");
        app_log_event("phase8 save selftest scheduled");
        printf("storage: phase8 save selftest scheduled, delayed_save=%ums\r\n",
               (unsigned int)APP_WORKSET_SAVE_DELAY_MS);
    }

    return s_storage_status.settings_loaded_from_flash;
}

void app_storage_task(void *argument)
{
    (void)argument;

    printf("task_storage started\r\n");
    app_log_event("task_storage started");

    while (1)
    {
        TickType_t now = xTaskGetTickCount();

        if (s_storage_status.flash_ready &&
            app_settings_get_pending_block(now, &s_storage_block))
        {
            printf("storage: save workset start\r\n");
            if (app_storage_save_block(&s_storage_block))
            {
                s_storage_status.save_ok_count++;
                s_storage_status.last_save_tick = (uint32_t)now;
                app_settings_on_save_result(true, now);
                app_storage_set_status_text("saved");
                printf("storage: save workset ok count=%lu\r\n",
                       (unsigned long)s_storage_status.save_ok_count);
                app_log_event("settings saved #%lu", (unsigned long)s_storage_status.save_ok_count);
            }
            else
            {
                s_storage_status.save_fail_count++;
                app_settings_on_save_result(false, now);
                app_storage_set_status_text("save fail");
                printf("storage: save workset fail count=%lu\r\n",
                       (unsigned long)s_storage_status.save_fail_count);
                app_log_event("settings save fail #%lu", (unsigned long)s_storage_status.save_fail_count);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200U));
    }
}

void app_storage_get_status(app_storage_status_t *status)
{
    if (status == NULL)
    {
        return;
    }

    uint32_t primask = app_critical_enter();
    *status = s_storage_status;
    app_critical_exit(primask);
}

static void app_storage_set_status_text(const char *text)
{
    uint32_t primask = app_critical_enter();
    (void)strncpy(s_storage_status.last_status, text, sizeof(s_storage_status.last_status) - 1U);
    s_storage_status.last_status[sizeof(s_storage_status.last_status) - 1U] = '\0';
    app_critical_exit(primask);
}

static bool app_storage_load_settings(void)
{
    app_settings_load_result_t load_result;

    if (!app_w25q256_read(APP_W25Q256_PARAM_BASE, &s_storage_block, sizeof(s_storage_block)))
    {
        load_result = APP_SETTINGS_LOAD_DEFAULT;
        (void)strncpy(s_storage_status.load_result, "read_fail", sizeof(s_storage_status.load_result) - 1U);
        app_storage_set_status_text("read fail");
        app_log_event("settings read fail");
        return false;
    }

    load_result = app_settings_import_block(&s_storage_block);
    s_storage_status.settings_loaded_from_flash = (load_result == APP_SETTINGS_LOAD_FLASH_OK);
    (void)strncpy(s_storage_status.load_result,
                  app_settings_load_result_name(load_result),
                  sizeof(s_storage_status.load_result) - 1U);
    s_storage_status.load_result[sizeof(s_storage_status.load_result) - 1U] = '\0';

    if (s_storage_status.settings_loaded_from_flash)
    {
        app_storage_set_status_text("loaded");
        app_log_event("settings loaded from W25Q");
    }
    else
    {
        app_settings_request_save_now();
        app_storage_set_status_text("default");
        app_log_event("settings default: %s", s_storage_status.load_result);
    }

    return s_storage_status.settings_loaded_from_flash;
}

static bool app_storage_save_block(const app_workset_block_t *block)
{
    if (block == NULL)
    {
        return false;
    }

    if (!app_w25q256_erase_sector(APP_W25Q256_PARAM_BASE))
    {
        return false;
    }

    if (!app_w25q256_write_erased(APP_W25Q256_PARAM_BASE, block, sizeof(*block)))
    {
        return false;
    }

    if (!app_w25q256_read(APP_W25Q256_PARAM_BASE, &s_storage_verify_block, sizeof(s_storage_verify_block)))
    {
        return false;
    }

    return (memcmp(block, &s_storage_verify_block, sizeof(*block)) == 0);
}
