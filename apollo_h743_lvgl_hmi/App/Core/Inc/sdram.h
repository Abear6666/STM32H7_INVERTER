#ifndef APP_SDRAM_H
#define APP_SDRAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APP_SDRAM_BASE_ADDR 0xC0000000UL
#define APP_SDRAM_SIZE_BYTES (32UL * 1024UL * 1024UL)
#define APP_SDRAM_WORDS (APP_SDRAM_SIZE_BYTES / sizeof(uint32_t))

typedef struct
{
    bool passed;
    const char *stage;
    uintptr_t address;
    uint32_t expected;
    uint32_t actual;
    uint32_t tested_bytes;
} app_sdram_test_result_t;

void app_sdram_mpu_config(void);
void app_sdram_init(void);
bool app_sdram_memtest(size_t bytes, app_sdram_test_result_t *result);
void app_sdram_print_config(void);

#endif
