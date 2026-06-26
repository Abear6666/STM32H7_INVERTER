#ifndef APP_IAP_RECORD_H
#define APP_IAP_RECORD_H

#include <stdbool.h>
#include <stdint.h>

#define APP_IAP_W25Q_BASE            0x01A00000UL
#define APP_IAP_W25Q_SIZE            0x00600000UL
#define APP_IAP_W25Q_SECTOR_SIZE     4096UL
#define APP_IAP_W25Q_PENDING_ADDR    APP_IAP_W25Q_BASE
#define APP_IAP_W25Q_STATE_ADDR      (APP_IAP_W25Q_BASE + APP_IAP_W25Q_SECTOR_SIZE)
#define APP_IAP_W25Q_STAGING_ADDR    (APP_IAP_W25Q_BASE + (2UL * APP_IAP_W25Q_SECTOR_SIZE))
#define APP_IAP_W25Q_END_ADDR        (APP_IAP_W25Q_BASE + APP_IAP_W25Q_SIZE)
#define APP_IAP_W25Q_STAGING_SIZE    (APP_IAP_W25Q_SIZE - (2UL * APP_IAP_W25Q_SECTOR_SIZE))

#define APP_IAP_PENDING_MAGIC        0x50414950UL
#define APP_IAP_PENDING_VERSION      1UL

#define APP_IAP_BOOT_STATE_MAGIC     0x53504149UL
#define APP_IAP_BOOT_STATE_VERSION   1UL
#define APP_IAP_BOOT_MAX_ATTEMPTS    3UL

#define APP_IAP_BOOT_ERROR_NONE              0UL
#define APP_IAP_BOOT_ERROR_APPB_INVALID      1UL
#define APP_IAP_BOOT_ERROR_TRIAL_EXHAUSTED   2UL

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t length;
    uint32_t target_slot;
    uint32_t app_size;
    uint32_t app_version;
    uint32_t app_crc32;
    uint32_t staging_addr;
    uint32_t header_crc32;
    uint32_t reserved[7];
} app_iap_pending_t;

typedef enum
{
    APP_IAP_BOOT_STATE_NONE = 0,
    APP_IAP_BOOT_STATE_TRIAL = 1,
    APP_IAP_BOOT_STATE_CONFIRMED = 2,
    APP_IAP_BOOT_STATE_ROLLBACK = 3,
} app_iap_boot_state_id_t;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t length;
    uint32_t state;
    uint32_t active_slot;
    uint32_t trial_slot;
    uint32_t trial_version;
    uint32_t boot_attempts;
    uint32_t max_boot_attempts;
    uint32_t last_error;
    uint32_t record_crc32;
    uint32_t reserved[5];
} app_iap_boot_state_t;

uint32_t app_iap_pending_crc(const app_iap_pending_t *pending);
bool app_iap_pending_valid(const app_iap_pending_t *pending);
uint32_t app_iap_boot_state_crc(const app_iap_boot_state_t *state);
bool app_iap_boot_state_valid(const app_iap_boot_state_t *state);
const char *app_iap_boot_state_name(uint32_t state);

#endif
