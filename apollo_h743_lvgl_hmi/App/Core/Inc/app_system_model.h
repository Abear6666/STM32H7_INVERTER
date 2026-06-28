#ifndef APP_SYSTEM_MODEL_H
#define APP_SYSTEM_MODEL_H

#include <stdbool.h>
#include <stdint.h>

#include "app_comm_bus.h"

typedef struct
{
    bool online;
    uint32_t last_rx_ms;
    uint32_t frame_count;
    uint32_t crc_error_count;
    uint32_t timeout_count;
    int16_t bus_voltage_v_x10;
    int16_t grid_voltage_v_x10;
    int16_t output_current_a_x10;
    int16_t temperature_c_x10;
    uint16_t run_state;
    uint16_t warn_code;
    uint16_t fault_code;
    uint16_t firmware_version;
} app_dsp_snapshot_t;

typedef struct
{
    bool online;
    uint32_t last_rx_ms;
    uint32_t frame_count;
    uint32_t timeout_count;
    uint16_t soc_percent_x10;
    uint16_t soh_percent_x10;
    int16_t pack_voltage_v_x10;
    int16_t pack_current_a_x10;
    int16_t max_temp_c_x10;
    uint16_t charge_limit_a_x10;
    uint16_t discharge_limit_a_x10;
    uint32_t alarm_flags;
    uint32_t fault_flags;
} app_bms_snapshot_t;

typedef struct
{
    uint32_t request_count;
    uint32_t exception_count;
    uint32_t crc_error_count;
    uint32_t last_request_ms;
} app_modbus_snapshot_t;

typedef struct
{
    app_dsp_snapshot_t dsp;
    app_bms_snapshot_t bms;
    app_modbus_snapshot_t modbus;
} app_system_snapshot_t;

void app_system_model_init(void);
void app_system_model_process_event(const app_comm_event_t *event);
void app_system_model_poll(uint32_t now_ms);
void app_system_model_get_snapshot(app_system_snapshot_t *snapshot);

#endif
