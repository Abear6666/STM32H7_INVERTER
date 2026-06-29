#ifndef APP_RS485_MODBUS_H
#define APP_RS485_MODBUS_H

#include <stdbool.h>
#include <stdint.h>

#define APP_RS485_MODBUS_BAUDRATE 9600U

typedef struct
{
    bool ready;
    uint32_t rx_byte_count;
    uint32_t rx_frame_count;
    uint32_t tx_frame_count;
    uint32_t rx_overflow_count;
    uint32_t short_frame_count;
    uint32_t tx_error_count;
    uint32_t last_rx_ms;
    uint32_t last_tx_ms;
} app_rs485_modbus_diag_t;

bool app_rs485_modbus_init(void);
bool app_rs485_modbus_is_ready(void);
void app_rs485_modbus_poll(uint32_t now_ms);
void app_rs485_modbus_get_diag(app_rs485_modbus_diag_t *diag);

#endif
