#include "app_diag.h"

#include <stdio.h>
#include <string.h>

#include "stm32h7xx_hal.h"

#define APP_DIAG_MAGIC   0xAD1A6001UL
#define APP_DIAG_VERSION 1UL

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t boot_count;
    uint32_t fault_valid;
    uint32_t fault_type;
    uint32_t fault_count;
    uint32_t stacked_r0;
    uint32_t stacked_r1;
    uint32_t stacked_r2;
    uint32_t stacked_r3;
    uint32_t stacked_r12;
    uint32_t stacked_lr;
    uint32_t stacked_pc;
    uint32_t stacked_xpsr;
    uint32_t exc_return;
    uint32_t msp;
    uint32_t psp;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t dfsr;
    uint32_t afsr;
    uint32_t mmfar;
    uint32_t bfar;
} app_diag_persistent_t;

__attribute__((section(".noinit.app_diag"), used))
static app_diag_persistent_t s_diag_persistent;

static app_diag_snapshot_t s_diag_snapshot;

static void app_diag_reset_persistent(void);
static void app_diag_build_reset_reason(uint32_t rsr, char *buffer, size_t buffer_len);
static void app_diag_append_reason(char *buffer, size_t buffer_len, const char *reason);

void app_diag_early_init(void)
{
    uint32_t rsr = RCC->RSR;
    bool cold_reset = ((rsr & (RCC_RSR_PORRSTF | RCC_RSR_BORRSTF)) != 0U);

    if ((s_diag_persistent.magic != APP_DIAG_MAGIC) ||
        (s_diag_persistent.version != APP_DIAG_VERSION) ||
        cold_reset)
    {
        app_diag_reset_persistent();
    }

    s_diag_persistent.boot_count++;

    memset(&s_diag_snapshot, 0, sizeof(s_diag_snapshot));
    s_diag_snapshot.magic = APP_DIAG_MAGIC;
    s_diag_snapshot.version = APP_DIAG_VERSION;
    s_diag_snapshot.boot_count = s_diag_persistent.boot_count;
    s_diag_snapshot.reset_rsr = rsr;
    app_diag_build_reset_reason(rsr, s_diag_snapshot.reset_reason, sizeof(s_diag_snapshot.reset_reason));
    s_diag_snapshot.fault_valid = (s_diag_persistent.fault_valid != 0U);
    s_diag_snapshot.fault_type = s_diag_persistent.fault_type;
    s_diag_snapshot.fault_count = s_diag_persistent.fault_count;
    s_diag_snapshot.stacked_r0 = s_diag_persistent.stacked_r0;
    s_diag_snapshot.stacked_r1 = s_diag_persistent.stacked_r1;
    s_diag_snapshot.stacked_r2 = s_diag_persistent.stacked_r2;
    s_diag_snapshot.stacked_r3 = s_diag_persistent.stacked_r3;
    s_diag_snapshot.stacked_r12 = s_diag_persistent.stacked_r12;
    s_diag_snapshot.stacked_lr = s_diag_persistent.stacked_lr;
    s_diag_snapshot.stacked_pc = s_diag_persistent.stacked_pc;
    s_diag_snapshot.stacked_xpsr = s_diag_persistent.stacked_xpsr;
    s_diag_snapshot.exc_return = s_diag_persistent.exc_return;
    s_diag_snapshot.msp = s_diag_persistent.msp;
    s_diag_snapshot.psp = s_diag_persistent.psp;
    s_diag_snapshot.cfsr = s_diag_persistent.cfsr;
    s_diag_snapshot.hfsr = s_diag_persistent.hfsr;
    s_diag_snapshot.dfsr = s_diag_persistent.dfsr;
    s_diag_snapshot.afsr = s_diag_persistent.afsr;
    s_diag_snapshot.mmfar = s_diag_persistent.mmfar;
    s_diag_snapshot.bfar = s_diag_persistent.bfar;

    __HAL_RCC_CLEAR_RESET_FLAGS();
}

void app_diag_print_boot(void)
{
    printf("diag: boot_count=%lu reset_rsr=0x%08lX reason=%s\r\n",
           (unsigned long)s_diag_snapshot.boot_count,
           (unsigned long)s_diag_snapshot.reset_rsr,
           s_diag_snapshot.reset_reason);

    if (s_diag_snapshot.fault_valid)
    {
        printf("diag: last_fault=%s count=%lu pc=0x%08lX lr=0x%08lX cfsr=0x%08lX hfsr=0x%08lX bfar=0x%08lX mmfar=0x%08lX\r\n",
               app_diag_fault_name(s_diag_snapshot.fault_type),
               (unsigned long)s_diag_snapshot.fault_count,
               (unsigned long)s_diag_snapshot.stacked_pc,
               (unsigned long)s_diag_snapshot.stacked_lr,
               (unsigned long)s_diag_snapshot.cfsr,
               (unsigned long)s_diag_snapshot.hfsr,
               (unsigned long)s_diag_snapshot.bfar,
               (unsigned long)s_diag_snapshot.mmfar);
    }
    else
    {
        printf("diag: last_fault=none\r\n");
    }
}

void app_diag_get_snapshot(app_diag_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    *snapshot = s_diag_snapshot;
}

void app_diag_record_fault(app_diag_fault_type_t type, uint32_t *stacked_sp, uint32_t exc_return)
{
    s_diag_persistent.magic = APP_DIAG_MAGIC;
    s_diag_persistent.version = APP_DIAG_VERSION;
    s_diag_persistent.fault_valid = 1U;
    s_diag_persistent.fault_type = (uint32_t)type;
    s_diag_persistent.fault_count++;

    if (stacked_sp != NULL)
    {
        s_diag_persistent.stacked_r0 = stacked_sp[0];
        s_diag_persistent.stacked_r1 = stacked_sp[1];
        s_diag_persistent.stacked_r2 = stacked_sp[2];
        s_diag_persistent.stacked_r3 = stacked_sp[3];
        s_diag_persistent.stacked_r12 = stacked_sp[4];
        s_diag_persistent.stacked_lr = stacked_sp[5];
        s_diag_persistent.stacked_pc = stacked_sp[6];
        s_diag_persistent.stacked_xpsr = stacked_sp[7];
    }

    s_diag_persistent.exc_return = exc_return;
    s_diag_persistent.msp = __get_MSP();
    s_diag_persistent.psp = __get_PSP();
    s_diag_persistent.cfsr = SCB->CFSR;
    s_diag_persistent.hfsr = SCB->HFSR;
    s_diag_persistent.dfsr = SCB->DFSR;
    s_diag_persistent.afsr = SCB->AFSR;
    s_diag_persistent.mmfar = SCB->MMFAR;
    s_diag_persistent.bfar = SCB->BFAR;

    __DSB();
    __ISB();
}

const char *app_diag_fault_name(uint32_t type)
{
    switch ((app_diag_fault_type_t)type)
    {
    case APP_DIAG_FAULT_HARDFAULT:
        return "HardFault";
    case APP_DIAG_FAULT_MEMMANAGE:
        return "MemManage";
    case APP_DIAG_FAULT_BUSFAULT:
        return "BusFault";
    case APP_DIAG_FAULT_USAGEFAULT:
        return "UsageFault";
    case APP_DIAG_FAULT_NONE:
    default:
        return "none";
    }
}

static void app_diag_reset_persistent(void)
{
    memset(&s_diag_persistent, 0, sizeof(s_diag_persistent));
    s_diag_persistent.magic = APP_DIAG_MAGIC;
    s_diag_persistent.version = APP_DIAG_VERSION;
}

static void app_diag_build_reset_reason(uint32_t rsr, char *buffer, size_t buffer_len)
{
    if ((buffer == NULL) || (buffer_len == 0U))
    {
        return;
    }

    buffer[0] = '\0';

    if ((rsr & RCC_RSR_CPURSTF) != 0U)
    {
        app_diag_append_reason(buffer, buffer_len, "CPU");
    }
    if ((rsr & RCC_RSR_D1RSTF) != 0U)
    {
        app_diag_append_reason(buffer, buffer_len, "D1");
    }
    if ((rsr & RCC_RSR_D2RSTF) != 0U)
    {
        app_diag_append_reason(buffer, buffer_len, "D2");
    }
    if ((rsr & RCC_RSR_BORRSTF) != 0U)
    {
        app_diag_append_reason(buffer, buffer_len, "BOR");
    }
    if ((rsr & RCC_RSR_PINRSTF) != 0U)
    {
        app_diag_append_reason(buffer, buffer_len, "PIN");
    }
    if ((rsr & RCC_RSR_PORRSTF) != 0U)
    {
        app_diag_append_reason(buffer, buffer_len, "POR");
    }
    if ((rsr & RCC_RSR_SFTRSTF) != 0U)
    {
        app_diag_append_reason(buffer, buffer_len, "SOFT");
    }
    if ((rsr & RCC_RSR_IWDG1RSTF) != 0U)
    {
        app_diag_append_reason(buffer, buffer_len, "IWDG");
    }
    if ((rsr & RCC_RSR_WWDG1RSTF) != 0U)
    {
        app_diag_append_reason(buffer, buffer_len, "WWDG");
    }
    if ((rsr & RCC_RSR_LPWRRSTF) != 0U)
    {
        app_diag_append_reason(buffer, buffer_len, "LPWR");
    }

    if (buffer[0] == '\0')
    {
        (void)snprintf(buffer, buffer_len, "unknown");
    }
}

static void app_diag_append_reason(char *buffer, size_t buffer_len, const char *reason)
{
    size_t used;

    if ((buffer == NULL) || (reason == NULL) || (buffer_len == 0U))
    {
        return;
    }

    used = strlen(buffer);
    if (used >= buffer_len)
    {
        return;
    }

    if (used == 0U)
    {
        (void)snprintf(buffer, buffer_len, "%s", reason);
    }
    else
    {
        (void)snprintf(&buffer[used], buffer_len - used, "|%s", reason);
    }
}
