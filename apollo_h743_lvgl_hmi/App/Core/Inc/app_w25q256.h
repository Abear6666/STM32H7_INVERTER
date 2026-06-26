#ifndef APP_W25Q256_H
#define APP_W25Q256_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APP_W25Q256_SIZE_BYTES      (32UL * 1024UL * 1024UL)
#define APP_W25Q256_SECTOR_SIZE     4096UL
#define APP_W25Q256_PAGE_SIZE       256UL
#define APP_W25Q256_PARAM_BASE      0x00000000UL
#define APP_W25Q256_PARAM_SIZE      0x00010000UL

bool app_w25q256_init(void);
uint32_t app_w25q256_get_jedec_id(void);
bool app_w25q256_read(uint32_t address, void *data, size_t length);
bool app_w25q256_erase_sector(uint32_t address);
bool app_w25q256_page_program(uint32_t address, const void *data, size_t length);
bool app_w25q256_write_erased(uint32_t address, const void *data, size_t length);

#endif
