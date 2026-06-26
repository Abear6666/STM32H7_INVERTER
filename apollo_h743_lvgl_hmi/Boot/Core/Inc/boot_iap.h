#ifndef BOOT_IAP_H
#define BOOT_IAP_H

#include <stdbool.h>

#include "app_tag.h"

bool boot_iap_try_apply_pending(void);
app_slot_id_t boot_iap_select_boot_slot(void);
void boot_iap_report_slot_invalid(app_slot_id_t slot_id);

#endif
