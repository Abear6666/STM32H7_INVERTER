#ifndef APP_SD_H
#define APP_SD_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32h7xx_hal.h"

#define APP_SD_MOUNT_PATH "0:"

bool app_sd_mount(void);
void app_sd_unmount(void);
bool app_sd_initialize(void);
bool app_sd_is_mounted(void);
bool app_sd_is_card_ready(void);
bool app_sd_get_card_info(HAL_SD_CardInfoTypeDef *info);
uint32_t app_sd_get_last_error(void);
uint32_t app_sd_get_card_state(void);
bool app_sd_read_blocks(uint8_t *buffer, uint32_t sector, uint32_t count);
bool app_sd_write_blocks(const uint8_t *buffer, uint32_t sector, uint32_t count);

#endif
