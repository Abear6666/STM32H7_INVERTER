#ifndef APP_MODBUS_RTU_H
#define APP_MODBUS_RTU_H

#include <stdbool.h>
#include <stdint.h>

#define APP_MODBUS_MAX_FRAME_BYTES 64U

typedef enum
{
    APP_MODBUS_EXCEPTION_NONE = 0,
    APP_MODBUS_EXCEPTION_ILLEGAL_FUNCTION = 1,
    APP_MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS = 2,
    APP_MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE = 3,
} app_modbus_exception_t;

typedef struct
{
    uint8_t slave_addr;
    uint8_t function;
    uint16_t start_reg;
    uint16_t quantity;
    uint16_t value;
    uint32_t timestamp_ms;
    app_modbus_exception_t exception;
} app_modbus_request_t;

typedef struct
{
    uint32_t request_count;
    uint32_t response_count;
    uint32_t exception_count;
    uint32_t crc_error_count;
    uint32_t write_count;
    uint32_t last_request_ms;
    uint16_t last_reg;
    uint16_t last_value;
    uint8_t last_function;
    uint8_t last_exception;
} app_modbus_diag_t;

bool app_modbus_rtu_init(uint8_t slave_addr);
bool app_modbus_rtu_parse_request(const uint8_t *rx, uint32_t rx_len, app_modbus_request_t *request);
bool app_modbus_rtu_build_response(const app_modbus_request_t *request,
                                   uint8_t *tx,
                                   uint32_t tx_size,
                                   uint32_t *tx_len);
bool app_modbus_map_read(uint16_t reg, uint16_t *value);
bool app_modbus_map_write(uint16_t reg, uint16_t value);
void app_modbus_rtu_poll(uint32_t now_ms);
void app_modbus_get_diag(app_modbus_diag_t *diag);

#endif
