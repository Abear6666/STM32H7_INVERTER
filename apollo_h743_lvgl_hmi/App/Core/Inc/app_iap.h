#ifndef APP_IAP_H
#define APP_IAP_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    APP_IAP_SOURCE_SERIAL = 0,
    APP_IAP_SOURCE_USB,
    APP_IAP_SOURCE_SD_CARD,
} app_iap_source_t;

typedef struct
{
    bool flash_ready;
    bool pending_valid;
    bool serial_ready;
    bool usb_ready;
    bool sd_ready;
    bool recv_active;
    bool boot_state_valid;
    uint32_t app_b_version;
    uint32_t app_b_size;
    uint32_t app_b_crc32;
    uint32_t boot_state;
    uint32_t boot_active_slot;
    uint32_t boot_trial_slot;
    uint32_t boot_attempts;
    uint32_t boot_max_attempts;
    uint32_t boot_last_error;
    uint32_t recv_received;
    uint32_t recv_expected;
    char status[64];
} app_iap_status_t;

void app_iap_init(bool flash_ready);
bool app_iap_confirm_current_app(void);
void app_iap_task(void *argument);
void app_iap_get_status(app_iap_status_t *status);
void app_iap_request_demo_stage(app_iap_source_t source);
void app_iap_request_sd_stage(void);

#endif
