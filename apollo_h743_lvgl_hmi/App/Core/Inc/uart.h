#ifndef APP_UART_H
#define APP_UART_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32h7xx_hal.h"

extern UART_HandleTypeDef huart1;

void app_uart1_init(uint32_t baudrate);
void app_uart_log_mutex_init(void);
void app_uart1_rx_start_it(void);
bool app_uart1_rx_read_byte(uint8_t *byte);

#endif
