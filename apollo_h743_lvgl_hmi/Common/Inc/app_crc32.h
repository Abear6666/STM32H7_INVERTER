#ifndef APP_CRC32_H
#define APP_CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t app_crc32(const void *data, size_t length);
uint32_t app_crc32_update(uint32_t crc, const void *data, size_t length);

#endif
