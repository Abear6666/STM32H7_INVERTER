#ifndef APP_LCD_H
#define APP_LCD_H

#include <stdint.h>

#define APP_LCD_WIDTH 800UL
#define APP_LCD_HEIGHT 480UL
#define APP_LCD_BYTES_PER_PIXEL 2UL
#define APP_LCD_FRAMEBUFFER_ADDR 0xC0000000UL
#define APP_LCD_FRAMEBUFFER_SIZE_BYTES (APP_LCD_WIDTH * APP_LCD_HEIGHT * APP_LCD_BYTES_PER_PIXEL)

typedef struct
{
    uint16_t panel_id;
    uint16_t width;
    uint16_t height;
    uint16_t hsw;
    uint16_t hbp;
    uint16_t hfp;
    uint16_t vsw;
    uint16_t vbp;
    uint16_t vfp;
    uint32_t framebuffer_addr;
    uint32_t framebuffer_size_bytes;
} app_lcd_info_t;

void app_lcd_init(void);
void app_lcd_fill_test_pattern(void);
void app_lcd_fill_phase5_touch_background(void);
void app_lcd_draw_touch_mark(uint16_t x, uint16_t y);
void app_lcd_backlight_on(void);
void app_lcd_print_config(void);
app_lcd_info_t app_lcd_get_info(void);

#endif
