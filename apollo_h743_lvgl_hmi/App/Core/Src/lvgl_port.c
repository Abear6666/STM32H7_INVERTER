#include "lvgl_port.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lcd.h"
#include "lvgl.h"
#include "main.h"
#include "touch.h"

static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;
static lv_indev_drv_t s_touch_drv;
static lv_disp_t *s_disp;
static LV_ATTRIBUTE_MEM_ALIGN lv_color_t s_draw_buf_mem[APP_LVGL_DRAW_BUF_PIXELS];

static uint16_t s_last_touch_x;
static uint16_t s_last_touch_y;
static bool s_last_touch_pressed;
static bool s_logged_touch_pressed;

static void app_lvgl_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
static void app_lvgl_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

void app_lvgl_port_init(void)
{
    lv_disp_draw_buf_init(&s_draw_buf,
                          s_draw_buf_mem,
                          NULL,
                          (uint32_t)APP_LVGL_DRAW_BUF_PIXELS);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = (lv_coord_t)APP_LCD_WIDTH;
    s_disp_drv.ver_res = (lv_coord_t)APP_LCD_HEIGHT;
    s_disp_drv.draw_buf = &s_draw_buf;
    s_disp_drv.flush_cb = app_lvgl_disp_flush;
    s_disp_drv.antialiasing = 1;
    s_disp = lv_disp_drv_register(&s_disp_drv);

    lv_indev_drv_init(&s_touch_drv);
    s_touch_drv.type = LV_INDEV_TYPE_POINTER;
    s_touch_drv.disp = s_disp;
    s_touch_drv.read_cb = app_lvgl_touch_read;
    (void)lv_indev_drv_register(&s_touch_drv);
}

void app_lvgl_port_print_config(void)
{
    printf("LVGL version: %d.%d.%d\r\n",
           lv_version_major(),
           lv_version_minor(),
           lv_version_patch());
    printf("LVGL color: RGB565, LV_COLOR_DEPTH=%u, LV_COLOR_16_SWAP=%u\r\n",
           (unsigned int)LV_COLOR_DEPTH,
           (unsigned int)LV_COLOR_16_SWAP);
    printf("LVGL draw buffer: %lux%lu pixels, %lu bytes, single buffer\r\n",
           (unsigned long)APP_LCD_WIDTH,
           (unsigned long)APP_LVGL_DRAW_BUF_LINES,
           (unsigned long)(APP_LVGL_DRAW_BUF_PIXELS * sizeof(lv_color_t)));
    printf("LVGL framebuffer: SDRAM 0x%08lX, %lu bytes\r\n",
           (unsigned long)APP_LCD_FRAMEBUFFER_ADDR,
           (unsigned long)APP_LCD_FRAMEBUFFER_SIZE_BYTES);
    printf("LVGL tick: lv_tick_inc(elapsed) in task_gui\r\n");
    printf("LVGL loop: lv_timer_handler() in task_gui, FreeRTOS enabled\r\n");
}

static void app_lvgl_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    int32_t x1 = area->x1;
    int32_t x2 = area->x2;
    int32_t y1 = area->y1;
    int32_t y2 = area->y2;

    if ((x2 < 0) || (y2 < 0) ||
        (x1 >= (int32_t)APP_LCD_WIDTH) ||
        (y1 >= (int32_t)APP_LCD_HEIGHT))
    {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    if (x1 < 0)
    {
        x1 = 0;
    }
    if (y1 < 0)
    {
        y1 = 0;
    }
    if (x2 >= (int32_t)APP_LCD_WIDTH)
    {
        x2 = (int32_t)APP_LCD_WIDTH - 1;
    }
    if (y2 >= (int32_t)APP_LCD_HEIGHT)
    {
        y2 = (int32_t)APP_LCD_HEIGHT - 1;
    }

    const int32_t src_area_w = area->x2 - area->x1 + 1;
    const int32_t copy_w = x2 - x1 + 1;
    uint16_t *fb = (uint16_t *)APP_LCD_FRAMEBUFFER_ADDR;

    for (int32_t y = y1; y <= y2; ++y)
    {
        const lv_color_t *src = color_p + ((y - area->y1) * src_area_w) + (x1 - area->x1);
        uint16_t *dst = &fb[((uint32_t)y * APP_LCD_WIDTH) + (uint32_t)x1];
        memcpy(dst, src, (size_t)copy_w * sizeof(lv_color_t));
    }

    __DSB();
    lv_disp_flush_ready(disp_drv);
}

static void app_lvgl_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    app_touch_sample_t sample;

    (void)indev_drv;

    if (app_touch_read(&sample))
    {
        if (sample.pressed)
        {
            s_last_touch_x = sample.x;
            s_last_touch_y = sample.y;
            s_last_touch_pressed = true;
        }
        else
        {
            s_last_touch_pressed = false;
        }
    }

    data->point.x = (lv_coord_t)s_last_touch_x;
    data->point.y = (lv_coord_t)s_last_touch_y;
    data->state = s_last_touch_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

    if (s_last_touch_pressed && !s_logged_touch_pressed)
    {
        printf("LVGL TP down x=%u y=%u\r\n",
               (unsigned int)s_last_touch_x,
               (unsigned int)s_last_touch_y);
        s_logged_touch_pressed = true;
    }
    else if (!s_last_touch_pressed && s_logged_touch_pressed)
    {
        printf("LVGL TP up\r\n");
        s_logged_touch_pressed = false;
    }
}
