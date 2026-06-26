#ifndef APP_DIAG_H
#define APP_DIAG_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    APP_DIAG_FAULT_NONE = 0,
    APP_DIAG_FAULT_HARDFAULT,
    APP_DIAG_FAULT_MEMMANAGE,
    APP_DIAG_FAULT_BUSFAULT,
    APP_DIAG_FAULT_USAGEFAULT,
} app_diag_fault_type_t;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t boot_count;
    uint32_t reset_rsr;
    char reset_reason[80];

    bool fault_valid;
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
} app_diag_snapshot_t;

void app_diag_early_init(void);
void app_diag_print_boot(void);
void app_diag_get_snapshot(app_diag_snapshot_t *snapshot);
void app_diag_record_fault(app_diag_fault_type_t type, uint32_t *stacked_sp, uint32_t exc_return);
const char *app_diag_fault_name(uint32_t type);

#endif
