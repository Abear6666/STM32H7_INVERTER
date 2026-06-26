#include "app_tag.h"

#include "app_crc32.h"

static const app_slot_info_t s_slots[] = {
    {
        .id = APP_SLOT_A,
        .name = "AppA",
        .slot_start_addr = APP_A_START_ADDR,
        .tag_addr = APP_A_TAG_ADDR,
        .run_addr = APP_A_RUN_ADDR,
        .max_size = APP_A_MAX_SIZE_BYTES,
    },
    {
        .id = APP_SLOT_B,
        .name = "AppB",
        .slot_start_addr = APP_B_START_ADDR,
        .tag_addr = APP_B_TAG_ADDR,
        .run_addr = APP_B_RUN_ADDR,
        .max_size = APP_B_MAX_SIZE_BYTES,
    },
};

const app_slot_info_t *app_get_slot_info(app_slot_id_t id)
{
    if ((uint32_t)id >= (sizeof(s_slots) / sizeof(s_slots[0])))
    {
        return NULL;
    }

    return &s_slots[(uint32_t)id];
}

const app_slot_info_t *app_get_slot_info_by_run_addr(uint32_t run_addr)
{
    for (uint32_t i = 0; i < (sizeof(s_slots) / sizeof(s_slots[0])); ++i)
    {
        if (s_slots[i].run_addr == run_addr)
        {
            return &s_slots[i];
        }
    }

    return NULL;
}

bool app_tag_basic_valid(const app_tag_t *tag, const app_slot_info_t *slot)
{
    if ((tag == NULL) || (slot == NULL))
    {
        return false;
    }

    if ((tag->magic != APP_TAG_MAGIC) ||
        (tag->tag_size != sizeof(app_tag_t)) ||
        (tag->app_run_addr != slot->run_addr) ||
        (tag->app_size == 0U) ||
        (tag->app_size > slot->max_size))
    {
        return false;
    }

    return true;
}

uint32_t app_crc32_memory(uint32_t address, uint32_t length)
{
    return app_crc32((const void *)address, length);
}

bool app_tag_crc_valid(const app_tag_t *tag, const app_slot_info_t *slot)
{
    if (!app_tag_basic_valid(tag, slot))
    {
        return false;
    }

    return (app_crc32_memory(tag->app_run_addr, tag->app_size) == tag->crc32);
}
