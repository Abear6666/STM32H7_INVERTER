#include "touch.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "lcd.h"
#include "main.h"

#define APP_GT9XXX_ADDR_14_WR 0x28U
#define APP_GT9XXX_ADDR_14_RD 0x29U
#define APP_GT9XXX_ADDR_5D_WR 0xBAU
#define APP_GT9XXX_ADDR_5D_RD 0xBBU

#define APP_GT9XXX_CTRL_REG 0x8040U
#define APP_GT9XXX_CFGS_REG 0x8047U
#define APP_GT1X_CFGS_REG 0x8050U
#define APP_GT9XXX_PID_REG 0x8140U
#define APP_GT9XXX_GSTID_REG 0x814EU
#define APP_GT9XXX_TP1_REG 0x8150U

static uint8_t s_gt_cmd_wr = APP_GT9XXX_ADDR_14_WR;
static uint8_t s_gt_cmd_rd = APP_GT9XXX_ADDR_14_RD;
static uint16_t s_gt_cfg_reg = APP_GT9XXX_CFGS_REG;
static char s_gt_addr_name[] = "0x14";
static char s_gt_pid[5] = "----";
static uint8_t s_gt_touch_num = 5;
static bool s_touch_ready;
static bool s_was_pressed;
static uint16_t s_last_x = 0xFFFFU;
static uint16_t s_last_y = 0xFFFFU;
static uint32_t s_last_print_tick;

static void app_touch_gpio_init(void);
static void app_touch_i2c_init(void);
static void app_touch_i2c_delay(void);
static void app_touch_i2c_start(void);
static void app_touch_i2c_stop(void);
static bool app_touch_i2c_wait_ack(void);
static void app_touch_i2c_ack(void);
static void app_touch_i2c_nack(void);
static void app_touch_i2c_send_byte(uint8_t data);
static uint8_t app_touch_i2c_read_byte(bool ack);
static bool app_gt9xxx_write_reg(uint16_t reg, const uint8_t *buf, uint8_t len);
static bool app_gt9xxx_read_reg(uint16_t reg, uint8_t *buf, uint8_t len);
static void app_gt9xxx_reset(void);
static bool app_gt9xxx_try_address(uint8_t cmd_wr, uint8_t cmd_rd, const char *addr_name);
static bool app_gt9xxx_pid_is_valid(const uint8_t *pid);
static bool app_gt9xxx_scan(app_touch_sample_t *sample);
static void app_touch_map_to_screen(uint16_t raw_x, uint16_t raw_y, uint16_t *x, uint16_t *y);
static uint16_t app_touch_abs_diff(uint16_t a, uint16_t b);

bool app_touch_init(void)
{
    uint8_t ctrl;

    app_touch_gpio_init();
    app_touch_i2c_init();

    printf("Touch: reset GT9xxx\r\n");
    app_gt9xxx_reset();
    printf("Touch: read PID at address 0x14\r\n");
    if (!app_gt9xxx_try_address(APP_GT9XXX_ADDR_14_WR, APP_GT9XXX_ADDR_14_RD, "0x14"))
    {
        printf("Touch: PID read failed at address 0x14\r\n");
        s_touch_ready = false;
        return false;
    }
    printf("Touch: PID=%s\r\n", s_gt_pid);

    if ((strcmp(s_gt_pid, "1151") == 0) || (strcmp(s_gt_pid, "1158") == 0) ||
        (strcmp(s_gt_pid, "917S") == 0))
    {
        s_gt_cfg_reg = APP_GT1X_CFGS_REG;
    }
    else
    {
        s_gt_cfg_reg = APP_GT9XXX_CFGS_REG;
    }
    s_gt_touch_num = (strcmp(s_gt_pid, "9271") == 0) ? 10U : 5U;

    ctrl = 0x02U;
    printf("Touch: enter sleep/reconfig wait\r\n");
    (void)app_gt9xxx_write_reg(APP_GT9XXX_CTRL_REG, &ctrl, 1);
    HAL_Delay(10);
    ctrl = 0x00U;
    (void)app_gt9xxx_write_reg(APP_GT9XXX_CTRL_REG, &ctrl, 1);
    HAL_Delay(1500);
    printf("Touch: wake wait done\r\n");

    s_touch_ready = true;
    s_was_pressed = false;
    s_last_x = 0xFFFFU;
    s_last_y = 0xFFFFU;
    s_last_print_tick = HAL_GetTick();

    return true;
}

void app_touch_print_config(void)
{
    printf("Touch: GT9xxx over software IIC, SCL=PH6 SDA=PI3 INT=PH7 RST=PI8\r\n");
    printf("Touch: address=%s write=0x%02X read=0x%02X pid=%s\r\n",
           s_gt_addr_name,
           (unsigned int)s_gt_cmd_wr,
           (unsigned int)s_gt_cmd_rd,
           s_gt_pid);
    printf("Touch: config register=0x%04X max_points=%u\r\n",
           (unsigned int)s_gt_cfg_reg,
           (unsigned int)s_gt_touch_num);
    printf("Touch map: RGB4384 landscape, x=raw_x, y=raw_y, range=0..799/0..479\r\n");
}

bool app_touch_read(app_touch_sample_t *sample)
{
    if (!s_touch_ready)
    {
        return false;
    }

    return app_gt9xxx_scan(sample);
}

void app_touch_poll_and_print(void)
{
    app_touch_sample_t sample;
    uint32_t now = HAL_GetTick();

    if (!app_touch_read(&sample))
    {
        return;
    }

    if (sample.pressed)
    {
        bool moved = (app_touch_abs_diff(sample.x, s_last_x) >= 2U) ||
                     (app_touch_abs_diff(sample.y, s_last_y) >= 2U);
        bool due = (now - s_last_print_tick) >= 100U;

        app_lcd_draw_touch_mark(sample.x, sample.y);

        if (!s_was_pressed || moved || due)
        {
            printf("TP down x=%u y=%u raw_x=%u raw_y=%u points=%u\r\n",
                   (unsigned int)sample.x,
                   (unsigned int)sample.y,
                   (unsigned int)sample.raw_x,
                   (unsigned int)sample.raw_y,
                   (unsigned int)sample.points);
            s_was_pressed = true;
            s_last_x = sample.x;
            s_last_y = sample.y;
            s_last_print_tick = now;
        }
    }
    else if (s_was_pressed)
    {
        printf("TP up\r\n");
        s_was_pressed = false;
        s_last_x = 0xFFFFU;
        s_last_y = 0xFFFFU;
        s_last_print_tick = now;
    }
}

static void app_touch_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOI, &gpio);
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_8, GPIO_PIN_SET);

    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOH, &gpio);
}

static void app_touch_i2c_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = GPIO_PIN_6;
    HAL_GPIO_Init(GPIOH, &gpio);

    gpio.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOI, &gpio);

    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
}

static void app_touch_i2c_delay(void)
{
    for (volatile uint32_t i = 0; i < 180U; ++i)
    {
        __NOP();
    }
}

static void app_touch_i2c_start(void)
{
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
    app_touch_i2c_delay();
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_RESET);
    app_touch_i2c_delay();
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_RESET);
    app_touch_i2c_delay();
}

static void app_touch_i2c_stop(void)
{
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_RESET);
    app_touch_i2c_delay();
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
    app_touch_i2c_delay();
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);
    app_touch_i2c_delay();
}

static bool app_touch_i2c_wait_ack(void)
{
    uint16_t wait = 0;

    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);
    app_touch_i2c_delay();
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
    app_touch_i2c_delay();

    while (HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_3) == GPIO_PIN_SET)
    {
        if (++wait > 250U)
        {
            app_touch_i2c_stop();
            return false;
        }
        app_touch_i2c_delay();
    }

    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_RESET);
    app_touch_i2c_delay();
    return true;
}

static void app_touch_i2c_ack(void)
{
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_RESET);
    app_touch_i2c_delay();
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
    app_touch_i2c_delay();
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_RESET);
    app_touch_i2c_delay();
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);
    app_touch_i2c_delay();
}

static void app_touch_i2c_nack(void)
{
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);
    app_touch_i2c_delay();
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
    app_touch_i2c_delay();
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_RESET);
    app_touch_i2c_delay();
}

static void app_touch_i2c_send_byte(uint8_t data)
{
    for (uint8_t i = 0; i < 8U; ++i)
    {
        HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, ((data & 0x80U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        app_touch_i2c_delay();
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
        app_touch_i2c_delay();
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_RESET);
        data <<= 1;
    }

    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);
}

static uint8_t app_touch_i2c_read_byte(bool ack)
{
    uint8_t data = 0;

    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_3, GPIO_PIN_SET);

    for (uint8_t i = 0; i < 8U; ++i)
    {
        data <<= 1;
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_SET);
        app_touch_i2c_delay();
        if (HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_3) == GPIO_PIN_SET)
        {
            data++;
        }
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_6, GPIO_PIN_RESET);
        app_touch_i2c_delay();
    }

    if (ack)
    {
        app_touch_i2c_ack();
    }
    else
    {
        app_touch_i2c_nack();
    }

    return data;
}

static bool app_gt9xxx_write_reg(uint16_t reg, const uint8_t *buf, uint8_t len)
{
    app_touch_i2c_start();
    app_touch_i2c_send_byte(s_gt_cmd_wr);
    if (!app_touch_i2c_wait_ack())
    {
        return false;
    }
    app_touch_i2c_send_byte((uint8_t)(reg >> 8));
    if (!app_touch_i2c_wait_ack())
    {
        return false;
    }
    app_touch_i2c_send_byte((uint8_t)(reg & 0xFFU));
    if (!app_touch_i2c_wait_ack())
    {
        return false;
    }

    for (uint8_t i = 0; i < len; ++i)
    {
        app_touch_i2c_send_byte(buf[i]);
        if (!app_touch_i2c_wait_ack())
        {
            return false;
        }
    }

    app_touch_i2c_stop();
    return true;
}

static bool app_gt9xxx_read_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    app_touch_i2c_start();
    app_touch_i2c_send_byte(s_gt_cmd_wr);
    if (!app_touch_i2c_wait_ack())
    {
        return false;
    }
    app_touch_i2c_send_byte((uint8_t)(reg >> 8));
    if (!app_touch_i2c_wait_ack())
    {
        return false;
    }
    app_touch_i2c_send_byte((uint8_t)(reg & 0xFFU));
    if (!app_touch_i2c_wait_ack())
    {
        return false;
    }

    app_touch_i2c_start();
    app_touch_i2c_send_byte(s_gt_cmd_rd);
    if (!app_touch_i2c_wait_ack())
    {
        return false;
    }

    for (uint8_t i = 0; i < len; ++i)
    {
        buf[i] = app_touch_i2c_read_byte(i != (uint8_t)(len - 1U));
    }

    app_touch_i2c_stop();
    return true;
}

static void app_gt9xxx_reset(void)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOH, &gpio);

    gpio.Pin = GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOI, &gpio);

    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(10);

    gpio.Pin = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOH, &gpio);
    HAL_Delay(100);
}

static bool app_gt9xxx_try_address(uint8_t cmd_wr, uint8_t cmd_rd, const char *addr_name)
{
    uint8_t pid[4];

    s_gt_cmd_wr = cmd_wr;
    s_gt_cmd_rd = cmd_rd;

    if (!app_gt9xxx_read_reg(APP_GT9XXX_PID_REG, pid, sizeof(pid)))
    {
        return false;
    }

    if (!app_gt9xxx_pid_is_valid(pid))
    {
        return false;
    }

    for (uint8_t i = 0; i < 4U; ++i)
    {
        s_gt_pid[i] = (char)pid[i];
    }
    s_gt_pid[4] = '\0';
    (void)snprintf(s_gt_addr_name, sizeof(s_gt_addr_name), "%s", addr_name);

    return true;
}

static bool app_gt9xxx_pid_is_valid(const uint8_t *pid)
{
    bool has_valid = false;

    for (uint8_t i = 0; i < 4U; ++i)
    {
        if (pid[i] == 0x00U)
        {
            break;
        }

        if (!isalnum((int)pid[i]))
        {
            return false;
        }

        has_valid = true;
    }

    return has_valid;
}

static bool app_gt9xxx_scan(app_touch_sample_t *sample)
{
    uint8_t status = 0;
    uint8_t buf[4];
    uint8_t clear = 0;
    uint8_t points;

    if (sample == NULL)
    {
        return false;
    }

    if (!app_gt9xxx_read_reg(APP_GT9XXX_GSTID_REG, &status, 1))
    {
        return false;
    }

    points = status & 0x0FU;

    if (points > s_gt_touch_num)
    {
        if ((status & 0x80U) != 0U)
        {
            (void)app_gt9xxx_write_reg(APP_GT9XXX_GSTID_REG, &clear, 1);
        }
        return false;
    }

    if (points == 0U)
    {
        if ((status & 0x80U) == 0U)
        {
            return false;
        }

        sample->pressed = false;
        sample->points = 0U;
        sample->x = 0xFFFFU;
        sample->y = 0xFFFFU;
        sample->raw_x = 0xFFFFU;
        sample->raw_y = 0xFFFFU;
        (void)app_gt9xxx_write_reg(APP_GT9XXX_GSTID_REG, &clear, 1);
        return true;
    }

    if (!app_gt9xxx_read_reg(APP_GT9XXX_TP1_REG, buf, sizeof(buf)))
    {
        if ((status & 0x80U) != 0U)
        {
            (void)app_gt9xxx_write_reg(APP_GT9XXX_GSTID_REG, &clear, 1);
        }
        return false;
    }

    sample->pressed = true;
    sample->points = points;
    sample->raw_x = ((uint16_t)buf[1] << 8) | buf[0];
    sample->raw_y = ((uint16_t)buf[3] << 8) | buf[2];
    app_touch_map_to_screen(sample->raw_x, sample->raw_y, &sample->x, &sample->y);
    (void)app_gt9xxx_write_reg(APP_GT9XXX_GSTID_REG, &clear, 1);

    return true;
}

static void app_touch_map_to_screen(uint16_t raw_x, uint16_t raw_y, uint16_t *x, uint16_t *y)
{
    uint32_t mapped_x;
    uint32_t mapped_y;

    mapped_x = raw_x;
    mapped_y = raw_y;

    if (mapped_x >= APP_LCD_WIDTH)
    {
        mapped_x = APP_LCD_WIDTH - 1U;
    }

    if (mapped_y >= APP_LCD_HEIGHT)
    {
        mapped_y = APP_LCD_HEIGHT - 1U;
    }

    *x = (uint16_t)mapped_x;
    *y = (uint16_t)mapped_y;
}

static uint16_t app_touch_abs_diff(uint16_t a, uint16_t b)
{
    return (a > b) ? (uint16_t)(a - b) : (uint16_t)(b - a);
}
