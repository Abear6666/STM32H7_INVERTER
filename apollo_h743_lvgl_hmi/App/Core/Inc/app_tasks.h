#ifndef APP_TASKS_H
#define APP_TASKS_H

#include <stdint.h>

typedef struct
{
    uint32_t gui_stack_free_words;
    uint32_t storage_stack_free_words;
    uint32_t log_stack_free_words;
    uint32_t iap_stack_free_words;
    uint32_t idle_stack_free_words;
    uint32_t heap_free_bytes;
    uint32_t heap_min_free_bytes;
} app_task_runtime_status_t;

void app_tasks_start_scheduler(void);
void app_tasks_get_runtime_status(app_task_runtime_status_t *status);

#endif
