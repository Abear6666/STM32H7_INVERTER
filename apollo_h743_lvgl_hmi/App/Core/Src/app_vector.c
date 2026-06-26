#include "app_vector.h"

#include <stdint.h>
#include <stdio.h>

#include "flash_layout.h"
#include "main.h"

#ifndef APP_BUILD_RUN_ADDR
#define APP_BUILD_RUN_ADDR APP_A_RUN_ADDR
#endif

extern uint32_t g_pfnVectors[];
extern uint32_t _sram_vector;
extern uint32_t _eram_vector;

void app_vector_relocate_to_ram(void)
{
    uint32_t *dst = &_sram_vector;
    const uint32_t *src = g_pfnVectors;
    uint32_t words = ((uint32_t)(&_eram_vector - &_sram_vector));

    if (words > (APP_VECTOR_TABLE_SIZE / sizeof(uint32_t)))
    {
        words = APP_VECTOR_TABLE_SIZE / sizeof(uint32_t);
    }

    for (uint32_t i = 0; i < words; ++i)
    {
        dst[i] = src[i];
    }

    SCB->VTOR = (uint32_t)dst;
    __DSB();
    __ISB();

    printf("APP: vector copied to RAM src=0x%08lX dst=0x%08lX bytes=%lu\r\n",
           (unsigned long)APP_BUILD_RUN_ADDR,
           (unsigned long)dst,
           (unsigned long)(words * sizeof(uint32_t)));
}
