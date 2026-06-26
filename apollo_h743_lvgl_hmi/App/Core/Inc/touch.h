#ifndef APP_TOUCH_H
#define APP_TOUCH_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool pressed;
    uint8_t points;
    uint16_t x;
    uint16_t y;
    uint16_t raw_x;
    uint16_t raw_y;
} app_touch_sample_t;

bool app_touch_init(void);
void app_touch_print_config(void);
bool app_touch_read(app_touch_sample_t *sample);
void app_touch_poll_and_print(void);

#endif
