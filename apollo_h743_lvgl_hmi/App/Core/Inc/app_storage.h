#ifndef APP_STORAGE_H
#define APP_STORAGE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool flash_ready;
    bool settings_loaded_from_flash;
    uint32_t jedec_id;
    uint32_t save_ok_count;
    uint32_t save_fail_count;
    uint32_t last_save_tick;
    char load_result[20];
    char last_status[48];
} app_storage_status_t;

bool app_storage_init(void);
void app_storage_task(void *argument);
void app_storage_get_status(app_storage_status_t *status);

#endif
