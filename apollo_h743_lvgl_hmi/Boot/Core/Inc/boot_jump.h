#ifndef BOOT_JUMP_H
#define BOOT_JUMP_H

#include <stdbool.h>
#include <stdint.h>

bool boot_jump_to_app(uint32_t app_run_addr);

#endif
