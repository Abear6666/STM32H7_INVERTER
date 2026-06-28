#ifndef APP_COMM_BUS_H
#define APP_COMM_BUS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    APP_COMM_EVENT_DSP_STATUS = 0,
    APP_COMM_EVENT_DSP_ACK,
    APP_COMM_EVENT_DSP_TIMEOUT,
    APP_COMM_EVENT_DSP_CRC_ERROR,
    APP_COMM_EVENT_BMS_STATUS,
    APP_COMM_EVENT_BMS_LIMITS,
    APP_COMM_EVENT_BMS_ALARM,
    APP_COMM_EVENT_BMS_TIMEOUT,
    APP_COMM_EVENT_MODBUS_WRITE,
    APP_COMM_EVENT_MODBUS_ERROR,
} app_comm_event_type_t;

typedef struct
{
    bool online;
    int16_t bus_voltage_v_x10;
    int16_t grid_voltage_v_x10;
    int16_t output_current_a_x10;
    int16_t temperature_c_x10;
    uint16_t run_state;
    uint16_t warn_code;
    uint16_t fault_code;
    uint16_t firmware_version;
} app_dsp_status_t;

typedef struct
{
    bool online;
    uint16_t soc_percent_x10;
    uint16_t soh_percent_x10;
    int16_t pack_voltage_v_x10;
    int16_t pack_current_a_x10;
    int16_t max_temp_c_x10;
    uint16_t charge_limit_a_x10;
    uint16_t discharge_limit_a_x10;
    uint32_t alarm_flags;
    uint32_t fault_flags;
} app_bms_status_t;

typedef struct
{
    uint16_t reg;
    uint16_t value;
} app_modbus_write_t;

typedef struct
{
    app_comm_event_type_t type;
    uint32_t timestamp_ms;
    uint32_t source;
    uint32_t sequence;
    union
    {
        app_dsp_status_t dsp_status;
        app_bms_status_t bms_status;
        app_modbus_write_t modbus_write;
        uint32_t error_code;
    } data;
} app_comm_event_t;

bool app_comm_bus_init(void);
bool app_comm_publish(const app_comm_event_t *event, uint32_t timeout_ms);
bool app_comm_receive(app_comm_event_t *event, uint32_t timeout_ms);
uint32_t app_comm_bus_dropped_count(void);

#endif
