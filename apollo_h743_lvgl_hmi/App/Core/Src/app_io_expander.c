#include "app_io_expander.h"

#include <stdint.h>

#include "app_critical.h"
#include "main.h"

#define APP_IOX_I2C_GPIO_PORT GPIOH
#define APP_IOX_I2C_SCL_PIN   GPIO_PIN_4
#define APP_IOX_I2C_SDA_PIN   GPIO_PIN_5

#define APP_IOX_PCF8574_ADDR_7BIT 0x20U
#define APP_IOX_RS485_RE_BIT      6U

static uint8_t s_pcf8574_shadow = 0xFFU;
static bool s_pcf8574_ready;

static void app_iox_i2c_init(void);
static void app_iox_i2c_delay(void);
static void app_iox_i2c_scl_write(GPIO_PinState state);
static void app_iox_i2c_sda_write(GPIO_PinState state);
static GPIO_PinState app_iox_i2c_sda_read(void);
static void app_iox_i2c_sda_output(void);
static void app_iox_i2c_sda_input(void);
static void app_iox_i2c_start(void);
static void app_iox_i2c_stop(void);
static bool app_iox_i2c_wait_ack(void);
static void app_iox_i2c_send_byte(uint8_t data);
static bool app_iox_pcf8574_write(uint8_t value);

bool app_io_expander_init(void)
{
    app_iox_i2c_init();
    s_pcf8574_shadow = 0xFFU;
    s_pcf8574_shadow &= (uint8_t)~(1U << APP_IOX_RS485_RE_BIT);
    s_pcf8574_ready = app_iox_pcf8574_write(s_pcf8574_shadow);
    return s_pcf8574_ready;
}

bool app_io_expander_set_rs485_tx(bool enable)
{
    uint32_t primask;
    uint8_t next;
    bool ok;

    if (!s_pcf8574_ready)
    {
        return false;
    }

    primask = app_critical_enter();
    next = s_pcf8574_shadow;
    if (enable)
    {
        next |= (uint8_t)(1U << APP_IOX_RS485_RE_BIT);
    }
    else
    {
        next &= (uint8_t)~(1U << APP_IOX_RS485_RE_BIT);
    }
    app_critical_exit(primask);

    ok = app_iox_pcf8574_write(next);
    if (ok)
    {
        primask = app_critical_enter();
        s_pcf8574_shadow = next;
        app_critical_exit(primask);
    }
    else
    {
        s_pcf8574_ready = false;
    }

    return ok;
}

static void app_iox_i2c_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio.Pin = APP_IOX_I2C_SCL_PIN | APP_IOX_I2C_SDA_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(APP_IOX_I2C_GPIO_PORT, &gpio);

    app_iox_i2c_scl_write(GPIO_PIN_SET);
    app_iox_i2c_sda_write(GPIO_PIN_SET);
}

static void app_iox_i2c_delay(void)
{
    for (volatile uint32_t i = 0; i < 80U; ++i)
    {
        __NOP();
    }
}

static void app_iox_i2c_scl_write(GPIO_PinState state)
{
    HAL_GPIO_WritePin(APP_IOX_I2C_GPIO_PORT, APP_IOX_I2C_SCL_PIN, state);
}

static void app_iox_i2c_sda_write(GPIO_PinState state)
{
    HAL_GPIO_WritePin(APP_IOX_I2C_GPIO_PORT, APP_IOX_I2C_SDA_PIN, state);
}

static GPIO_PinState app_iox_i2c_sda_read(void)
{
    return HAL_GPIO_ReadPin(APP_IOX_I2C_GPIO_PORT, APP_IOX_I2C_SDA_PIN);
}

static void app_iox_i2c_sda_output(void)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = APP_IOX_I2C_SDA_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(APP_IOX_I2C_GPIO_PORT, &gpio);
}

static void app_iox_i2c_sda_input(void)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = APP_IOX_I2C_SDA_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(APP_IOX_I2C_GPIO_PORT, &gpio);
}

static void app_iox_i2c_start(void)
{
    app_iox_i2c_sda_output();
    app_iox_i2c_sda_write(GPIO_PIN_SET);
    app_iox_i2c_scl_write(GPIO_PIN_SET);
    app_iox_i2c_delay();
    app_iox_i2c_sda_write(GPIO_PIN_RESET);
    app_iox_i2c_delay();
    app_iox_i2c_scl_write(GPIO_PIN_RESET);
}

static void app_iox_i2c_stop(void)
{
    app_iox_i2c_sda_output();
    app_iox_i2c_scl_write(GPIO_PIN_RESET);
    app_iox_i2c_sda_write(GPIO_PIN_RESET);
    app_iox_i2c_delay();
    app_iox_i2c_scl_write(GPIO_PIN_SET);
    app_iox_i2c_delay();
    app_iox_i2c_sda_write(GPIO_PIN_SET);
    app_iox_i2c_delay();
}

static bool app_iox_i2c_wait_ack(void)
{
    bool ack;

    app_iox_i2c_sda_input();
    app_iox_i2c_scl_write(GPIO_PIN_SET);
    app_iox_i2c_delay();
    ack = (app_iox_i2c_sda_read() == GPIO_PIN_RESET);
    app_iox_i2c_scl_write(GPIO_PIN_RESET);
    app_iox_i2c_sda_output();
    return ack;
}

static void app_iox_i2c_send_byte(uint8_t data)
{
    app_iox_i2c_sda_output();
    for (uint8_t bit = 0U; bit < 8U; ++bit)
    {
        app_iox_i2c_sda_write((data & 0x80U) != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET);
        app_iox_i2c_delay();
        app_iox_i2c_scl_write(GPIO_PIN_SET);
        app_iox_i2c_delay();
        app_iox_i2c_scl_write(GPIO_PIN_RESET);
        data <<= 1U;
    }
}

static bool app_iox_pcf8574_write(uint8_t value)
{
    bool ok;

    app_iox_i2c_start();
    app_iox_i2c_send_byte((uint8_t)(APP_IOX_PCF8574_ADDR_7BIT << 1U));
    ok = app_iox_i2c_wait_ack();
    if (ok)
    {
        app_iox_i2c_send_byte(value);
        ok = app_iox_i2c_wait_ack();
    }
    app_iox_i2c_stop();

    return ok;
}
