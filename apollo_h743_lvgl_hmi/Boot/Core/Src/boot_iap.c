#include "boot_iap.h"

#include <stdio.h>
#include <string.h>

#include "app_crc32.h"
#include "app_iap_record.h"
#include "app_tag.h"
#include "app_w25q256.h"
#include "flash_layout.h"
#include "main.h"

#define BOOT_IAP_COPY_CHUNK       512U
#define BOOT_IAP_FLASH_WORD_BYTES (FLASH_NB_32BITWORD_IN_FLASHWORD * sizeof(uint32_t))

static uint8_t s_boot_iap_buffer[BOOT_IAP_COPY_CHUNK];
static uint8_t s_boot_iap_flash_word[BOOT_IAP_FLASH_WORD_BYTES] __attribute__((aligned(4)));
static bool s_boot_iap_flash_ready;

static bool boot_iap_verify_staging_crc(const app_iap_pending_t *pending);
static bool boot_iap_read_staged_tag(const app_iap_pending_t *pending, app_tag_t *tag);
static bool boot_iap_staged_package_valid(const app_iap_pending_t *pending, const app_tag_t *tag);
static bool boot_iap_erase_app_b(void);
static bool boot_iap_program_app_b(const app_iap_pending_t *pending);
static bool boot_iap_verify_flash_bytes(uint32_t flash_addr, uint32_t staging_addr, uint32_t length);
static bool boot_iap_compare_flash_chunk(uint32_t flash_addr,
                                         const uint8_t *expected,
                                         uint32_t length,
                                         uint32_t *mismatch_offset,
                                         uint8_t *actual,
                                         uint8_t *want);
static void boot_iap_sync_flash_view(void);
static bool boot_iap_verify_app_b_tag(void);
static bool boot_iap_clear_pending(void);
static bool boot_iap_read_state(app_iap_boot_state_t *state);
static bool boot_iap_write_state(app_iap_boot_state_t *state);
static bool boot_iap_write_trial_state(uint32_t version);
static bool boot_iap_write_rollback_state(uint32_t error);
static bool boot_iap_app_b_basic_valid(void);

bool boot_iap_try_apply_pending(void)
{
    app_iap_pending_t pending;
    app_tag_t staged_tag;

    printf("BOOT IAP: check W25Q pending flag at 0x%08lX\r\n",
           (unsigned long)APP_IAP_W25Q_PENDING_ADDR);
    s_boot_iap_flash_ready = false;

    if (!app_w25q256_init())
    {
        printf("BOOT IAP: W25Q256 unavailable, id=0x%06lX\r\n",
               (unsigned long)app_w25q256_get_jedec_id());
        printf("BOOT IAP: continue with existing internal Apps, AppA untouched\r\n");
        return false;
    }
    s_boot_iap_flash_ready = true;

    if (!app_w25q256_read(APP_IAP_W25Q_PENDING_ADDR, &pending, sizeof(pending)))
    {
        printf("BOOT IAP: pending flag read failed\r\n");
        printf("BOOT IAP: continue with existing internal Apps, AppA untouched\r\n");
        return false;
    }

    printf("BOOT IAP: pending magic=0x%08lX version=%lu target=%lu size=%lu app_ver=%lu crc=0x%08lX staging=0x%08lX\r\n",
           (unsigned long)pending.magic,
           (unsigned long)pending.version,
           (unsigned long)pending.target_slot,
           (unsigned long)pending.app_size,
           (unsigned long)pending.app_version,
           (unsigned long)pending.app_crc32,
           (unsigned long)pending.staging_addr);

    if (!app_iap_pending_valid(&pending))
    {
        printf("BOOT IAP: no valid pending package\r\n");
        printf("BOOT IAP: continue with existing internal Apps, AppA untouched\r\n");
        return false;
    }

    if (!boot_iap_verify_staging_crc(&pending))
    {
        printf("BOOT IAP: staged package CRC failed\r\n");
        if (!boot_iap_clear_pending())
        {
            printf("BOOT IAP: warning, bad pending clear failed\r\n");
        }
        printf("BOOT IAP: continue with existing internal Apps, AppA untouched\r\n");
        return false;
    }

    if (!boot_iap_read_staged_tag(&pending, &staged_tag) ||
        !boot_iap_staged_package_valid(&pending, &staged_tag))
    {
        printf("BOOT IAP: staged package tag invalid, AppB not changed\r\n");
        if (!boot_iap_clear_pending())
        {
            printf("BOOT IAP: warning, bad pending clear failed\r\n");
        }
        printf("BOOT IAP: continue with existing internal Apps, AppA untouched\r\n");
        return false;
    }

    printf("BOOT IAP: staged AppB package CRC OK, version=%lu app_size=%lu\r\n",
           (unsigned long)staged_tag.version,
           (unsigned long)staged_tag.app_size);
    printf("BOOT IAP: erase/program internal AppB only, AppA untouched\r\n");

    if (!boot_iap_erase_app_b())
    {
        printf("BOOT IAP: AppB erase failed, AppA untouched\r\n");
        return false;
    }

    if (!boot_iap_program_app_b(&pending))
    {
        printf("BOOT IAP: AppB program failed, AppA untouched\r\n");
        return false;
    }

    boot_iap_sync_flash_view();
    if (!boot_iap_verify_flash_bytes(APP_B_START_ADDR, pending.staging_addr, pending.app_size))
    {
        printf("BOOT IAP: AppB flash readback mismatch, AppA untouched\r\n");
        return false;
    }

    if (!boot_iap_verify_app_b_tag())
    {
        printf("BOOT IAP: AppB final tag/crc invalid, AppA untouched\r\n");
        return false;
    }

    if (!boot_iap_clear_pending())
    {
        printf("BOOT IAP: warning, pending flag clear failed; AppB is valid\r\n");
    }

    if (!boot_iap_write_trial_state(staged_tag.version))
    {
        printf("BOOT IAP: warning, trial state write failed; AppA remains fallback\r\n");
    }

    printf("BOOT IAP: AppB update applied and verified, trial pending, AppA untouched\r\n");
    return true;
}

app_slot_id_t boot_iap_select_boot_slot(void)
{
    app_iap_boot_state_t state;

    if (!boot_iap_read_state(&state))
    {
        printf("BOOT IAP: boot state unavailable, select AppA fallback\r\n");
        return APP_SLOT_A;
    }

    printf("BOOT IAP: boot state=%s active=%lu trial=%lu version=%lu attempts=%lu/%lu err=%lu\r\n",
           app_iap_boot_state_name(state.state),
           (unsigned long)state.active_slot,
           (unsigned long)state.trial_slot,
           (unsigned long)state.trial_version,
           (unsigned long)state.boot_attempts,
           (unsigned long)state.max_boot_attempts,
           (unsigned long)state.last_error);

    if ((state.state == APP_IAP_BOOT_STATE_CONFIRMED) &&
        (state.active_slot == APP_SLOT_B))
    {
        printf("BOOT IAP: confirmed AppB selected\r\n");
        return APP_SLOT_B;
    }

    if ((state.state == APP_IAP_BOOT_STATE_TRIAL) &&
        (state.trial_slot == APP_SLOT_B))
    {
        if (!boot_iap_app_b_basic_valid())
        {
            printf("BOOT IAP: trial AppB tag invalid before jump, rollback to AppA\r\n");
            (void)boot_iap_write_rollback_state(APP_IAP_BOOT_ERROR_APPB_INVALID);
            return APP_SLOT_A;
        }

        if (state.boot_attempts < state.max_boot_attempts)
        {
            state.boot_attempts++;
            state.record_crc32 = app_iap_boot_state_crc(&state);
            if (!boot_iap_write_state(&state))
            {
                printf("BOOT IAP: warning, trial attempt write failed\r\n");
            }
            printf("BOOT IAP: trial AppB attempt %lu/%lu selected\r\n",
                   (unsigned long)state.boot_attempts,
                   (unsigned long)state.max_boot_attempts);
            return APP_SLOT_B;
        }

        printf("BOOT IAP: trial attempts exhausted, rollback to AppA\r\n");
        (void)boot_iap_write_rollback_state(APP_IAP_BOOT_ERROR_TRIAL_EXHAUSTED);
        return APP_SLOT_A;
    }

    printf("BOOT IAP: state does not allow AppB, select AppA fallback\r\n");
    return APP_SLOT_A;
}

void boot_iap_report_slot_invalid(app_slot_id_t slot_id)
{
    if (slot_id == APP_SLOT_B)
    {
        printf("BOOT IAP: selected AppB failed validation, rollback to AppA\r\n");
        (void)boot_iap_write_rollback_state(APP_IAP_BOOT_ERROR_APPB_INVALID);
    }
}

static bool boot_iap_verify_staging_crc(const app_iap_pending_t *pending)
{
    uint32_t crc = 0U;
    uint32_t address;
    uint32_t remaining;

    if (pending == NULL)
    {
        return false;
    }

    address = pending->staging_addr;
    remaining = pending->app_size;
    while (remaining > 0U)
    {
        uint32_t chunk = (remaining > sizeof(s_boot_iap_buffer)) ? sizeof(s_boot_iap_buffer) : remaining;

        if (!app_w25q256_read(address, s_boot_iap_buffer, chunk))
        {
            return false;
        }

        crc = app_crc32_update(crc, s_boot_iap_buffer, chunk);
        address += chunk;
        remaining -= chunk;
    }

    printf("BOOT IAP: staging crc calc=0x%08lX expect=0x%08lX\r\n",
           (unsigned long)crc,
           (unsigned long)pending->app_crc32);
    return (crc == pending->app_crc32);
}

static bool boot_iap_read_staged_tag(const app_iap_pending_t *pending, app_tag_t *tag)
{
    if ((pending == NULL) || (tag == NULL) || (pending->app_size < APP_SLOT_HEADER_SIZE))
    {
        return false;
    }

    return app_w25q256_read(pending->staging_addr + APP_SLOT_HEADER_SIZE - APP_TAG_SIZE_BYTES,
                            tag,
                            sizeof(*tag));
}

static bool boot_iap_staged_package_valid(const app_iap_pending_t *pending, const app_tag_t *tag)
{
    const app_slot_info_t *slot = app_get_slot_info(APP_SLOT_B);

    if ((pending == NULL) || (tag == NULL) || (slot == NULL))
    {
        return false;
    }

    printf("BOOT IAP: staged tag magic=0x%08lX size=%lu run=0x%08lX app_size=%lu version=%lu crc=0x%08lX\r\n",
           (unsigned long)tag->magic,
           (unsigned long)tag->tag_size,
           (unsigned long)tag->app_run_addr,
           (unsigned long)tag->app_size,
           (unsigned long)tag->version,
           (unsigned long)tag->crc32);

    if (!app_tag_basic_valid(tag, slot))
    {
        return false;
    }

    if (pending->app_size != (APP_SLOT_HEADER_SIZE + tag->app_size))
    {
        printf("BOOT IAP: package size mismatch total=%lu expected=%lu\r\n",
               (unsigned long)pending->app_size,
               (unsigned long)(APP_SLOT_HEADER_SIZE + tag->app_size));
        return false;
    }

    if (pending->app_version != tag->version)
    {
        printf("BOOT IAP: pending version=%lu does not match tag version=%lu\r\n",
               (unsigned long)pending->app_version,
               (unsigned long)tag->version);
        return false;
    }

    return true;
}

static bool boot_iap_erase_app_b(void)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_error = 0xFFFFFFFFUL;

    memset(&erase, 0, sizeof(erase));
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Banks = FLASH_BANK_2;
    erase.Sector = 0U;
    erase.NbSectors = APP_B_SIZE_BYTES / STM32H743_FLASH_SECTOR_SIZE;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_4;

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return false;
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);
    if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK)
    {
        printf("BOOT IAP: HAL erase AppB failed sector=%lu err=0x%08lX\r\n",
               (unsigned long)sector_error,
               (unsigned long)HAL_FLASH_GetError());
        (void)HAL_FLASH_Lock();
        return false;
    }

    (void)HAL_FLASH_Lock();
    printf("BOOT IAP: AppB erase OK sectors=%lu\r\n", (unsigned long)erase.NbSectors);
    return true;
}

static bool boot_iap_program_app_b(const app_iap_pending_t *pending)
{
    uint32_t source_addr;
    uint32_t flash_addr;
    uint32_t remaining;
    uint32_t programmed = 0U;

    if ((pending == NULL) ||
        (pending->app_size == 0U) ||
        (pending->app_size > APP_B_SIZE_BYTES))
    {
        return false;
    }

    source_addr = pending->staging_addr;
    flash_addr = APP_B_START_ADDR;
    remaining = pending->app_size;

    if (HAL_FLASH_Unlock() != HAL_OK)
    {
        return false;
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK2);

    while (remaining > 0U)
    {
        uint32_t chunk = (remaining > BOOT_IAP_FLASH_WORD_BYTES) ? BOOT_IAP_FLASH_WORD_BYTES : remaining;

        memset(s_boot_iap_flash_word, 0xFF, sizeof(s_boot_iap_flash_word));
        if (!app_w25q256_read(source_addr, s_boot_iap_flash_word, chunk))
        {
            (void)HAL_FLASH_Lock();
            return false;
        }

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                              flash_addr,
                              (uint32_t)s_boot_iap_flash_word) != HAL_OK)
        {
            printf("BOOT IAP: HAL program failed addr=0x%08lX err=0x%08lX\r\n",
                   (unsigned long)flash_addr,
                   (unsigned long)HAL_FLASH_GetError());
            (void)HAL_FLASH_Lock();
            return false;
        }

        source_addr += chunk;
        flash_addr += BOOT_IAP_FLASH_WORD_BYTES;
        remaining -= chunk;
        programmed += chunk;

        if (((programmed % (128UL * 1024UL)) == 0U) || (remaining == 0U))
        {
            printf("BOOT IAP: AppB program progress %lu/%lu\r\n",
                   (unsigned long)programmed,
                   (unsigned long)pending->app_size);
        }
    }

    (void)HAL_FLASH_Lock();
    return true;
}

static bool boot_iap_verify_flash_bytes(uint32_t flash_addr, uint32_t staging_addr, uint32_t length)
{
    uint32_t remaining = length;
    uint32_t verified = 0U;
    uint32_t verify_addr = flash_addr;
    uint32_t staging_read_addr = staging_addr;

    while (remaining > 0U)
    {
        uint32_t chunk = (remaining > sizeof(s_boot_iap_buffer)) ? sizeof(s_boot_iap_buffer) : remaining;
        bool match = false;

        for (uint32_t attempt = 0U; attempt < 3U; ++attempt)
        {
            uint32_t mismatch_offset = 0U;
            uint8_t actual = 0U;
            uint8_t want = 0U;

            if (!app_w25q256_read(staging_read_addr, s_boot_iap_buffer, chunk))
            {
                return false;
            }

            if (boot_iap_compare_flash_chunk(verify_addr,
                                             s_boot_iap_buffer,
                                             chunk,
                                             &mismatch_offset,
                                             &actual,
                                             &want))
            {
                if (attempt > 0U)
                {
                    printf("BOOT IAP: verify retry recovered at flash=0x%08lX attempt=%lu\r\n",
                           (unsigned long)(verify_addr + mismatch_offset),
                           (unsigned long)(attempt + 1U));
                }
                match = true;
                break;
            }

            printf("BOOT IAP: verify mismatch at flash=0x%08lX actual=0x%02X expect=0x%02X attempt=%lu\r\n",
                   (unsigned long)(verify_addr + mismatch_offset),
                   actual,
                   want,
                   (unsigned long)(attempt + 1U));
            boot_iap_sync_flash_view();
            HAL_Delay(2U);
        }

        if (!match)
        {
            return false;
        }

        verify_addr += chunk;
        staging_read_addr += chunk;
        remaining -= chunk;
        verified += chunk;
    }

    printf("BOOT IAP: AppB flash readback OK bytes=%lu\r\n", (unsigned long)verified);
    return true;
}

static bool boot_iap_compare_flash_chunk(uint32_t flash_addr,
                                         const uint8_t *expected,
                                         uint32_t length,
                                         uint32_t *mismatch_offset,
                                         uint8_t *actual,
                                         uint8_t *want)
{
    volatile const uint8_t *flash = (volatile const uint8_t *)flash_addr;

    if ((expected == NULL) || (mismatch_offset == NULL) || (actual == NULL) || (want == NULL))
    {
        return false;
    }

    for (uint32_t i = 0U; i < length; ++i)
    {
        uint8_t flash_byte = flash[i];
        if (flash_byte != expected[i])
        {
            *mismatch_offset = i;
            *actual = flash_byte;
            *want = expected[i];
            return false;
        }
    }

    return true;
}

static void boot_iap_sync_flash_view(void)
{
    __DSB();
    __ISB();
    SCB_InvalidateICache();
    __DSB();
    __ISB();
}

static bool boot_iap_verify_app_b_tag(void)
{
    const app_slot_info_t *slot = app_get_slot_info(APP_SLOT_B);
    const app_tag_t *tag = (const app_tag_t *)APP_B_TAG_ADDR;
    uint32_t calc_crc;

    if ((slot == NULL) || !app_tag_basic_valid(tag, slot))
    {
        return false;
    }

    calc_crc = app_crc32_memory(tag->app_run_addr, tag->app_size);
    printf("BOOT IAP: AppB crc calc=0x%08lX expect=0x%08lX\r\n",
           (unsigned long)calc_crc,
           (unsigned long)tag->crc32);
    return (calc_crc == tag->crc32);
}

static bool boot_iap_clear_pending(void)
{
    if (!app_w25q256_erase_sector(APP_IAP_W25Q_PENDING_ADDR))
    {
        return false;
    }

    printf("BOOT IAP: pending flag cleared\r\n");
    return true;
}

static bool boot_iap_read_state(app_iap_boot_state_t *state)
{
    if (state == NULL)
    {
        return false;
    }

    if (!s_boot_iap_flash_ready)
    {
        printf("BOOT IAP: boot state skipped, W25Q256 not ready\r\n");
        return false;
    }

    if (!app_w25q256_read(APP_IAP_W25Q_STATE_ADDR, state, sizeof(*state)))
    {
        printf("BOOT IAP: boot state read failed\r\n");
        return false;
    }

    if (!app_iap_boot_state_valid(state))
    {
        printf("BOOT IAP: no valid boot state at 0x%08lX\r\n",
               (unsigned long)APP_IAP_W25Q_STATE_ADDR);
        return false;
    }

    return true;
}

static bool boot_iap_write_state(app_iap_boot_state_t *state)
{
    if (state == NULL)
    {
        return false;
    }

    if (!s_boot_iap_flash_ready)
    {
        return false;
    }

    state->magic = APP_IAP_BOOT_STATE_MAGIC;
    state->version = APP_IAP_BOOT_STATE_VERSION;
    state->length = sizeof(*state);
    state->record_crc32 = app_iap_boot_state_crc(state);

    if (!app_iap_boot_state_valid(state))
    {
        return false;
    }

    if (!app_w25q256_erase_sector(APP_IAP_W25Q_STATE_ADDR))
    {
        return false;
    }

    return app_w25q256_write_erased(APP_IAP_W25Q_STATE_ADDR, state, sizeof(*state));
}

static bool boot_iap_write_trial_state(uint32_t version)
{
    app_iap_boot_state_t state;

    memset(&state, 0, sizeof(state));
    state.state = APP_IAP_BOOT_STATE_TRIAL;
    state.active_slot = APP_SLOT_A;
    state.trial_slot = APP_SLOT_B;
    state.trial_version = version;
    state.boot_attempts = 0U;
    state.max_boot_attempts = APP_IAP_BOOT_MAX_ATTEMPTS;
    state.last_error = APP_IAP_BOOT_ERROR_NONE;

    if (!boot_iap_write_state(&state))
    {
        return false;
    }

    printf("BOOT IAP: boot state written: trial AppB version=%lu max_attempts=%lu\r\n",
           (unsigned long)version,
           (unsigned long)APP_IAP_BOOT_MAX_ATTEMPTS);
    return true;
}

static bool boot_iap_write_rollback_state(uint32_t error)
{
    app_iap_boot_state_t state;

    memset(&state, 0, sizeof(state));
    state.state = APP_IAP_BOOT_STATE_ROLLBACK;
    state.active_slot = APP_SLOT_A;
    state.trial_slot = APP_SLOT_B;
    state.trial_version = 0U;
    state.boot_attempts = APP_IAP_BOOT_MAX_ATTEMPTS;
    state.max_boot_attempts = APP_IAP_BOOT_MAX_ATTEMPTS;
    state.last_error = error;

    if (!boot_iap_write_state(&state))
    {
        printf("BOOT IAP: rollback state write failed\r\n");
        return false;
    }

    printf("BOOT IAP: rollback state written error=%lu, AppA selected\r\n",
           (unsigned long)error);
    return true;
}

static bool boot_iap_app_b_basic_valid(void)
{
    const app_slot_info_t *slot = app_get_slot_info(APP_SLOT_B);
    const app_tag_t *tag = (const app_tag_t *)APP_B_TAG_ADDR;

    return app_tag_basic_valid(tag, slot);
}
