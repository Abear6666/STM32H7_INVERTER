#ifndef APP_LED_H
#define APP_LED_H

#include "stm32h7xx_hal.h"

#define LED0_GPIO_PORT GPIOB
#define LED0_GPIO_PIN GPIO_PIN_1
#define LED1_GPIO_PORT GPIOB
#define LED1_GPIO_PIN GPIO_PIN_0

void app_led_init(void);
void app_led0_toggle(void);
void app_led1_toggle(void);
void app_led0_write(GPIO_PinState state);
void app_led1_write(GPIO_PinState state);

#endif
