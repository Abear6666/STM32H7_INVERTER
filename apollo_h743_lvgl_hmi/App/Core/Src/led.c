#include "led.h"

void app_led_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = LED0_GPIO_PIN | LED1_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &gpio);

    app_led0_write(GPIO_PIN_SET);
    app_led1_write(GPIO_PIN_SET);
}

void app_led0_toggle(void)
{
    HAL_GPIO_TogglePin(LED0_GPIO_PORT, LED0_GPIO_PIN);
}

void app_led1_toggle(void)
{
    HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_GPIO_PIN);
}

void app_led0_write(GPIO_PinState state)
{
    HAL_GPIO_WritePin(LED0_GPIO_PORT, LED0_GPIO_PIN, state);
}

void app_led1_write(GPIO_PinState state)
{
    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, state);
}
