#ifndef APP_LOG_H
#define APP_LOG_H

#include <stddef.h>
#include <stdint.h>

#define APP_LOG_LINE_COUNT 8U
#define APP_LOG_LINE_LEN   72U

void app_log_init(void);
void app_log_event(const char *fmt, ...);
uint32_t app_log_copy_recent(char lines[][APP_LOG_LINE_LEN], uint32_t max_lines);

#endif
