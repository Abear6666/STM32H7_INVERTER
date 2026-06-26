#include "app_iap_record.h"

#include <stddef.h>

#include "app_crc32.h"
#include "app_tag.h"

uint32_t app_iap_pending_crc(const app_iap_pending_t *pending)
{
    uint32_t crc = 0U;

    if (pending == NULL)
    {
        return 0U;
    }

    crc = app_crc32_update(crc, &pending->magic, sizeof(pending->magic));
    crc = app_crc32_update(crc, &pending->version, sizeof(pending->version));
    crc = app_crc32_update(crc, &pending->length, sizeof(pending->length));
    crc = app_crc32_update(crc, &pending->target_slot, sizeof(pending->target_slot));
    crc = app_crc32_update(crc, &pending->app_size, sizeof(pending->app_size));
    crc = app_crc32_update(crc, &pending->app_version, sizeof(pending->app_version));
    crc = app_crc32_update(crc, &pending->app_crc32, sizeof(pending->app_crc32));
    crc = app_crc32_update(crc, &pending->staging_addr, sizeof(pending->staging_addr));
    return crc;
}

bool app_iap_pending_valid(const app_iap_pending_t *pending)
{
    uint32_t staging_end;

    if (pending == NULL)
    {
        return false;
    }

    staging_end = pending->staging_addr + pending->app_size;
    if (staging_end < pending->staging_addr)
    {
        return false;
    }

    if (pending->app_size > APP_IAP_W25Q_STAGING_SIZE)
    {
        return false;
    }

    return ((pending->magic == APP_IAP_PENDING_MAGIC) &&
            (pending->version == APP_IAP_PENDING_VERSION) &&
            (pending->length == sizeof(*pending)) &&
            (pending->target_slot == APP_SLOT_B) &&
            (pending->app_size > 0U) &&
            (pending->staging_addr >= APP_IAP_W25Q_STAGING_ADDR) &&
            (staging_end <= APP_IAP_W25Q_END_ADDR) &&
            (pending->header_crc32 == app_iap_pending_crc(pending)));
}

uint32_t app_iap_boot_state_crc(const app_iap_boot_state_t *state)
{
    uint32_t crc = 0U;

    if (state == NULL)
    {
        return 0U;
    }

    crc = app_crc32_update(crc, &state->magic, sizeof(state->magic));
    crc = app_crc32_update(crc, &state->version, sizeof(state->version));
    crc = app_crc32_update(crc, &state->length, sizeof(state->length));
    crc = app_crc32_update(crc, &state->state, sizeof(state->state));
    crc = app_crc32_update(crc, &state->active_slot, sizeof(state->active_slot));
    crc = app_crc32_update(crc, &state->trial_slot, sizeof(state->trial_slot));
    crc = app_crc32_update(crc, &state->trial_version, sizeof(state->trial_version));
    crc = app_crc32_update(crc, &state->boot_attempts, sizeof(state->boot_attempts));
    crc = app_crc32_update(crc, &state->max_boot_attempts, sizeof(state->max_boot_attempts));
    crc = app_crc32_update(crc, &state->last_error, sizeof(state->last_error));
    return crc;
}

bool app_iap_boot_state_valid(const app_iap_boot_state_t *state)
{
    if (state == NULL)
    {
        return false;
    }

    if ((state->magic != APP_IAP_BOOT_STATE_MAGIC) ||
        (state->version != APP_IAP_BOOT_STATE_VERSION) ||
        (state->length != sizeof(*state)) ||
        (state->record_crc32 != app_iap_boot_state_crc(state)))
    {
        return false;
    }

    if ((state->active_slot != APP_SLOT_A) && (state->active_slot != APP_SLOT_B))
    {
        return false;
    }

    if ((state->trial_slot != APP_SLOT_A) && (state->trial_slot != APP_SLOT_B))
    {
        return false;
    }

    if ((state->state != APP_IAP_BOOT_STATE_TRIAL) &&
        (state->state != APP_IAP_BOOT_STATE_CONFIRMED) &&
        (state->state != APP_IAP_BOOT_STATE_ROLLBACK))
    {
        return false;
    }

    if ((state->max_boot_attempts == 0U) ||
        (state->max_boot_attempts > APP_IAP_BOOT_MAX_ATTEMPTS))
    {
        return false;
    }

    if (state->boot_attempts > state->max_boot_attempts)
    {
        return false;
    }

    return true;
}

const char *app_iap_boot_state_name(uint32_t state)
{
    switch (state)
    {
    case APP_IAP_BOOT_STATE_NONE:
        return "none";
    case APP_IAP_BOOT_STATE_TRIAL:
        return "trial";
    case APP_IAP_BOOT_STATE_CONFIRMED:
        return "confirmed";
    case APP_IAP_BOOT_STATE_ROLLBACK:
        return "rollback";
    default:
        return "invalid";
    }
}
