#ifndef APP_LVGL_PORT_H
#define APP_LVGL_PORT_H

#include <stdint.h>

#define APP_LVGL_DRAW_BUF_LINES 40UL
#define APP_LVGL_DRAW_BUF_PIXELS (800UL * APP_LVGL_DRAW_BUF_LINES)

void app_lvgl_port_init(void);
void app_lvgl_port_print_config(void);

#endif
