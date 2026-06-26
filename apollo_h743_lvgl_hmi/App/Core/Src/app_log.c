#include "app_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app_critical.h"
#include "task.h"

static char s_log_lines[APP_LOG_LINE_COUNT][APP_LOG_LINE_LEN];
static uint32_t s_log_head;
static uint32_t s_log_count;

void app_log_init(void)
{
    uint32_t primask = app_critical_enter();
    memset(s_log_lines, 0, sizeof(s_log_lines));
    s_log_head = 0;
    s_log_count = 0;
    app_critical_exit(primask);
}

void app_log_event(const char *fmt, ...)
{
    char line[APP_LOG_LINE_LEN];
    va_list args;

    va_start(args, fmt);
    (void)vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    line[sizeof(line) - 1U] = '\0';

    uint32_t primask = app_critical_enter();
    (void)strncpy(s_log_lines[s_log_head], line, APP_LOG_LINE_LEN - 1U);
    s_log_lines[s_log_head][APP_LOG_LINE_LEN - 1U] = '\0';
    s_log_head = (s_log_head + 1U) % APP_LOG_LINE_COUNT;
    if (s_log_count < APP_LOG_LINE_COUNT)
    {
        s_log_count++;
    }
    app_critical_exit(primask);
}

uint32_t app_log_copy_recent(char lines[][APP_LOG_LINE_LEN], uint32_t max_lines)
{
    uint32_t copied = 0;

    if ((lines == NULL) || (max_lines == 0U))
    {
        return 0;
    }

    uint32_t primask = app_critical_enter();
    copied = (s_log_count < max_lines) ? s_log_count : max_lines;
    for (uint32_t i = 0; i < copied; ++i)
    {
        uint32_t index = (s_log_head + APP_LOG_LINE_COUNT - copied + i) % APP_LOG_LINE_COUNT;
        (void)strncpy(lines[i], s_log_lines[index], APP_LOG_LINE_LEN);
        lines[i][APP_LOG_LINE_LEN - 1U] = '\0';
    }
    app_critical_exit(primask);

    return copied;
}
