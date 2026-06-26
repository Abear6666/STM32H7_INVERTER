#ifndef APP_TAG_H
#define APP_TAG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "flash_layout.h"

#define APP_TAG_MAGIC        0x41505447UL
#define APP_TAG_VERSION_NONE 0UL

#define APP_IAP_HEADER_MAGIC 0x31495041UL

typedef struct
{
    uint32_t magic;
    uint32_t tag_size;
    uint32_t app_run_addr;
    uint32_t app_size;
    uint32_t version;
    uint32_t crc32;
    uint8_t sha256[32];
    uint32_t reserved[2];
} app_tag_t;

typedef enum
{
    APP_SLOT_A = 0,
    APP_SLOT_B = 1,
} app_slot_id_t;

typedef struct
{
    app_slot_id_t id;
    const char *name;
    uint32_t slot_start_addr;
    uint32_t tag_addr;
    uint32_t run_addr;
    uint32_t max_size;
} app_slot_info_t;

typedef struct
{
    uint32_t magic;
    uint32_t target_slot;
    uint32_t app_size;
    uint32_t version;
    uint32_t crc32;
    uint32_t reserved[3];
} app_iap_header_t;

const app_slot_info_t *app_get_slot_info(app_slot_id_t id);
const app_slot_info_t *app_get_slot_info_by_run_addr(uint32_t run_addr);
bool app_tag_basic_valid(const app_tag_t *tag, const app_slot_info_t *slot);
uint32_t app_crc32_memory(uint32_t address, uint32_t length);
bool app_tag_crc_valid(const app_tag_t *tag, const app_slot_info_t *slot);

#endif
