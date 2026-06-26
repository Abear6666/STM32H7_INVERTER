#include "app_settings.h"

#include <string.h>

#include "app_crc32.h"
#include "semphr.h"
#include "task.h"

typedef struct
{
    uint32_t language;
    uint32_t brightness;
    int32_t threshold;
    uint32_t output_enabled;
    uint32_t save_selftest_cookie;
    uint32_t reserved[11];
} app_workset_payload_t;

static app_settings_snapshot_t s_settings;
static SemaphoreHandle_t s_settings_mutex;
static bool s_dirty;
static TickType_t s_save_due_tick;

static void app_settings_set_defaults(void);
static uint32_t app_settings_block_crc(const app_workset_block_t *block);
static void app_settings_lock(void);
static void app_settings_unlock(void);
static TickType_t app_settings_now(void);

void app_settings_init(void)
{
    s_settings_mutex = xSemaphoreCreateMutex();
    app_settings_set_defaults();
    s_dirty = false;
    s_save_due_tick = 0;
}

void app_settings_get(app_settings_snapshot_t *settings)
{
    if (settings == NULL)
    {
        return;
    }

    app_settings_lock();
    *settings = s_settings;
    app_settings_unlock();
}

app_settings_load_result_t app_settings_import_block(const app_workset_block_t *block)
{
    app_workset_payload_t payload;
    uint32_t crc;

    if (block == NULL)
    {
        app_settings_set_defaults();
        return APP_SETTINGS_LOAD_DEFAULT;
    }

    if (block->magic != APP_WORKSET_MAGIC)
    {
        app_settings_set_defaults();
        return APP_SETTINGS_LOAD_BAD_MAGIC;
    }

    if (block->version != APP_WORKSET_VERSION)
    {
        app_settings_set_defaults();
        return APP_SETTINGS_LOAD_BAD_VERSION;
    }

    if (block->length != sizeof(app_workset_payload_t))
    {
        app_settings_set_defaults();
        return APP_SETTINGS_LOAD_BAD_LENGTH;
    }

    crc = app_settings_block_crc(block);
    if (crc != block->crc32)
    {
        app_settings_set_defaults();
        return APP_SETTINGS_LOAD_BAD_CRC;
    }

    memcpy(&payload, block->payload, sizeof(payload));

    app_settings_lock();
    s_settings.language = payload.language;
    s_settings.brightness = (payload.brightness <= 100U) ? payload.brightness : 70U;
    s_settings.threshold = payload.threshold;
    s_settings.output_enabled = (payload.output_enabled != 0U) ? 1U : 0U;
    s_settings.save_selftest_cookie = payload.save_selftest_cookie;
    s_dirty = false;
    s_save_due_tick = 0;
    app_settings_unlock();

    return APP_SETTINGS_LOAD_FLASH_OK;
}

void app_settings_export_block(app_workset_block_t *block)
{
    app_workset_payload_t payload;

    if (block == NULL)
    {
        return;
    }

    app_settings_lock();
    payload.language = s_settings.language;
    payload.brightness = s_settings.brightness;
    payload.threshold = s_settings.threshold;
    payload.output_enabled = s_settings.output_enabled;
    payload.save_selftest_cookie = s_settings.save_selftest_cookie;
    memset(payload.reserved, 0, sizeof(payload.reserved));
    app_settings_unlock();

    memset(block, 0xFF, sizeof(*block));
    block->magic = APP_WORKSET_MAGIC;
    block->version = APP_WORKSET_VERSION;
    block->length = sizeof(payload);
    memcpy(block->payload, &payload, sizeof(payload));
    memset(&block->payload[sizeof(payload)], 0, APP_WORKSET_PAYLOAD_SIZE - sizeof(payload));
    block->crc32 = app_settings_block_crc(block);
}

const char *app_settings_load_result_name(app_settings_load_result_t result)
{
    switch (result)
    {
    case APP_SETTINGS_LOAD_DEFAULT:
        return "default";
    case APP_SETTINGS_LOAD_FLASH_OK:
        return "flash_ok";
    case APP_SETTINGS_LOAD_BAD_MAGIC:
        return "bad_magic";
    case APP_SETTINGS_LOAD_BAD_VERSION:
        return "bad_version";
    case APP_SETTINGS_LOAD_BAD_LENGTH:
        return "bad_length";
    case APP_SETTINGS_LOAD_BAD_CRC:
        return "bad_crc";
    default:
        return "unknown";
    }
}

bool app_settings_set_brightness(uint32_t brightness)
{
    bool changed = false;

    if (brightness > 100U)
    {
        brightness = 100U;
    }

    app_settings_lock();
    if (s_settings.brightness != brightness)
    {
        s_settings.brightness = brightness;
        s_dirty = true;
        s_save_due_tick = app_settings_now() + pdMS_TO_TICKS(APP_WORKSET_SAVE_DELAY_MS);
        changed = true;
    }
    app_settings_unlock();

    return changed;
}

bool app_settings_set_threshold(int32_t threshold)
{
    bool changed = false;

    if (threshold < 0)
    {
        threshold = 0;
    }
    if (threshold > 100)
    {
        threshold = 100;
    }

    app_settings_lock();
    if (s_settings.threshold != threshold)
    {
        s_settings.threshold = threshold;
        s_dirty = true;
        s_save_due_tick = app_settings_now() + pdMS_TO_TICKS(APP_WORKSET_SAVE_DELAY_MS);
        changed = true;
    }
    app_settings_unlock();

    return changed;
}

bool app_settings_set_output_enabled(bool enabled)
{
    uint32_t value = enabled ? 1U : 0U;
    bool changed = false;

    app_settings_lock();
    if (s_settings.output_enabled != value)
    {
        s_settings.output_enabled = value;
        s_dirty = true;
        s_save_due_tick = app_settings_now() + pdMS_TO_TICKS(APP_WORKSET_SAVE_DELAY_MS);
        changed = true;
    }
    app_settings_unlock();

    return changed;
}

bool app_settings_set_language(uint32_t language)
{
    bool changed = false;

    if (language > 1U)
    {
        language = 1U;
    }

    app_settings_lock();
    if (s_settings.language != language)
    {
        s_settings.language = language;
        s_dirty = true;
        s_save_due_tick = app_settings_now() + pdMS_TO_TICKS(APP_WORKSET_SAVE_DELAY_MS);
        changed = true;
    }
    app_settings_unlock();

    return changed;
}

bool app_settings_run_save_selftest_once(void)
{
    bool changed = false;

    app_settings_lock();
    if (s_settings.save_selftest_cookie != APP_WORKSET_SELFTEST_COOKIE)
    {
        s_settings.brightness = (s_settings.brightness < 95U) ? (s_settings.brightness + 1U) : 70U;
        s_settings.save_selftest_cookie = APP_WORKSET_SELFTEST_COOKIE;
        s_dirty = true;
        s_save_due_tick = app_settings_now() + pdMS_TO_TICKS(APP_WORKSET_SAVE_DELAY_MS);
        changed = true;
    }
    app_settings_unlock();

    return changed;
}

void app_settings_request_save_now(void)
{
    app_settings_lock();
    s_dirty = true;
    s_save_due_tick = app_settings_now();
    app_settings_unlock();
}

bool app_settings_get_pending_block(TickType_t now, app_workset_block_t *block)
{
    bool due = false;

    if (block == NULL)
    {
        return false;
    }

    app_settings_lock();
    if (s_dirty && ((int32_t)(now - s_save_due_tick) >= 0))
    {
        due = true;
    }
    app_settings_unlock();

    if (!due)
    {
        return false;
    }

    app_settings_export_block(block);
    return true;
}

void app_settings_on_save_result(bool ok, TickType_t now)
{
    app_settings_lock();
    if (ok)
    {
        s_dirty = false;
        s_save_due_tick = 0;
    }
    else
    {
        s_dirty = true;
        s_save_due_tick = now + pdMS_TO_TICKS(5000U);
    }
    app_settings_unlock();
}

bool app_settings_is_dirty(void)
{
    bool dirty;

    app_settings_lock();
    dirty = s_dirty;
    app_settings_unlock();

    return dirty;
}

static void app_settings_set_defaults(void)
{
    app_settings_lock();
    s_settings.language = 0;
    s_settings.brightness = 70;
    s_settings.threshold = 55;
    s_settings.output_enabled = 1;
    s_settings.save_selftest_cookie = 0;
    s_dirty = false;
    s_save_due_tick = 0;
    app_settings_unlock();
}

static uint32_t app_settings_block_crc(const app_workset_block_t *block)
{
    uint32_t crc = 0U;

    crc = app_crc32_update(crc, &block->version, sizeof(block->version));
    crc = app_crc32_update(crc, &block->length, sizeof(block->length));
    crc = app_crc32_update(crc, block->payload, block->length);
    return crc;
}

static void app_settings_lock(void)
{
    if (s_settings_mutex != NULL)
    {
        (void)xSemaphoreTake(s_settings_mutex, portMAX_DELAY);
    }
}

static void app_settings_unlock(void)
{
    if (s_settings_mutex != NULL)
    {
        (void)xSemaphoreGive(s_settings_mutex);
    }
}

static TickType_t app_settings_now(void)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        return 0;
    }

    return xTaskGetTickCount();
}
