#ifndef APP_UART_H
#define APP_UART_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32h7xx_hal.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

void app_uart1_init(uint32_t baudrate);
void app_uart_log_mutex_init(void);
void app_uart1_rx_start_it(void);
bool app_uart1_rx_read_byte(uint8_t *byte);
void app_uart2_init(uint32_t baudrate);
bool app_uart2_rx_start_dma(uint8_t *buffer, uint16_t size);
void app_uart2_rx_on_idle_from_isr(uint16_t size);
void app_uart2_rx_on_error_from_isr(uint32_t error_code);

#endif
