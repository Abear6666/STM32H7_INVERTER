#ifndef APP_CRITICAL_H
#define APP_CRITICAL_H

#include <stdint.h>

uint32_t app_critical_enter(void);
void app_critical_exit(uint32_t primask);

#endif
