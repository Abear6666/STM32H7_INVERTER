#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"

#define APP_WORKSET_MAGIC        0x48534D48UL
#define APP_WORKSET_VERSION      1UL
#define APP_WORKSET_PAYLOAD_SIZE 512U
#define APP_WORKSET_SAVE_DELAY_MS 3000U
#define APP_WORKSET_SELFTEST_COOKIE 0x53385431UL

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t length;
    uint32_t crc32;
    uint8_t payload[APP_WORKSET_PAYLOAD_SIZE];
} app_workset_block_t;

typedef struct
{
    uint32_t language;
    uint32_t brightness;
    int32_t threshold;
    uint32_t output_enabled;
    uint32_t save_selftest_cookie;
} app_settings_snapshot_t;

typedef enum
{
    APP_SETTINGS_LOAD_DEFAULT = 0,
    APP_SETTINGS_LOAD_FLASH_OK,
    APP_SETTINGS_LOAD_BAD_MAGIC,
    APP_SETTINGS_LOAD_BAD_VERSION,
    APP_SETTINGS_LOAD_BAD_LENGTH,
    APP_SETTINGS_LOAD_BAD_CRC,
} app_settings_load_result_t;

void app_settings_init(void);
void app_settings_get(app_settings_snapshot_t *settings);
app_settings_load_result_t app_settings_import_block(const app_workset_block_t *block);
void app_settings_export_block(app_workset_block_t *block);
const char *app_settings_load_result_name(app_settings_load_result_t result);

bool app_settings_set_brightness(uint32_t brightness);
bool app_settings_set_threshold(int32_t threshold);
bool app_settings_set_output_enabled(bool enabled);
bool app_settings_set_language(uint32_t language);
bool app_settings_run_save_selftest_once(void);
void app_settings_request_save_now(void);
bool app_settings_get_pending_block(TickType_t now, app_workset_block_t *block);
void app_settings_on_save_result(bool ok, TickType_t now);
bool app_settings_is_dirty(void);

#endif
