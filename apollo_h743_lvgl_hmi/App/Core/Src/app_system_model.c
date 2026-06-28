#include "app_system_model.h"

#include <string.h>

#include "app_critical.h"

#define APP_SYSTEM_DSP_TIMEOUT_MS 500U
#define APP_SYSTEM_BMS_TIMEOUT_MS 1000U

static app_system_snapshot_t s_system_snapshot;

void app_system_model_init(void)
{
    uint32_t primask = app_critical_enter();
    memset(&s_system_snapshot, 0, sizeof(s_system_snapshot));
    app_critical_exit(primask);
}

void app_system_model_process_event(const app_comm_event_t *event)
{
    uint32_t primask;

    if (event == NULL)
    {
        return;
    }

    primask = app_critical_enter();

    switch (event->type)
    {
    case APP_COMM_EVENT_DSP_STATUS:
        s_system_snapshot.dsp.online = event->data.dsp_status.online;
        s_system_snapshot.dsp.last_rx_ms = event->timestamp_ms;
        s_system_snapshot.dsp.frame_count++;
        s_system_snapshot.dsp.bus_voltage_v_x10 = event->data.dsp_status.bus_voltage_v_x10;
        s_system_snapshot.dsp.grid_voltage_v_x10 = event->data.dsp_status.grid_voltage_v_x10;
        s_system_snapshot.dsp.output_current_a_x10 = event->data.dsp_status.output_current_a_x10;
        s_system_snapshot.dsp.temperature_c_x10 = event->data.dsp_status.temperature_c_x10;
        s_system_snapshot.dsp.run_state = event->data.dsp_status.run_state;
        s_system_snapshot.dsp.warn_code = event->data.dsp_status.warn_code;
        s_system_snapshot.dsp.fault_code = event->data.dsp_status.fault_code;
        s_system_snapshot.dsp.firmware_version = event->data.dsp_status.firmware_version;
        break;

    case APP_COMM_EVENT_DSP_ACK:
        s_system_snapshot.dsp.ack_count++;
        s_system_snapshot.dsp.last_rx_ms = event->timestamp_ms;
        break;

    case APP_COMM_EVENT_DSP_TIMEOUT:
        s_system_snapshot.dsp.online = false;
        s_system_snapshot.dsp.timeout_count++;
        break;

    case APP_COMM_EVENT_DSP_CRC_ERROR:
        s_system_snapshot.dsp.crc_error_count++;
        break;

    case APP_COMM_EVENT_BMS_STATUS:
    case APP_COMM_EVENT_BMS_LIMITS:
    case APP_COMM_EVENT_BMS_ALARM:
        s_system_snapshot.bms.online = event->data.bms_status.online;
        s_system_snapshot.bms.last_rx_ms = event->timestamp_ms;
        s_system_snapshot.bms.frame_count++;
        s_system_snapshot.bms.soc_percent_x10 = event->data.bms_status.soc_percent_x10;
        s_system_snapshot.bms.soh_percent_x10 = event->data.bms_status.soh_percent_x10;
        s_system_snapshot.bms.pack_voltage_v_x10 = event->data.bms_status.pack_voltage_v_x10;
        s_system_snapshot.bms.pack_current_a_x10 = event->data.bms_status.pack_current_a_x10;
        s_system_snapshot.bms.max_temp_c_x10 = event->data.bms_status.max_temp_c_x10;
        s_system_snapshot.bms.charge_limit_a_x10 = event->data.bms_status.charge_limit_a_x10;
        s_system_snapshot.bms.discharge_limit_a_x10 = event->data.bms_status.discharge_limit_a_x10;
        s_system_snapshot.bms.alarm_flags = event->data.bms_status.alarm_flags;
        s_system_snapshot.bms.fault_flags = event->data.bms_status.fault_flags;
        break;

    case APP_COMM_EVENT_BMS_TIMEOUT:
        s_system_snapshot.bms.online = false;
        s_system_snapshot.bms.timeout_count++;
        break;

    case APP_COMM_EVENT_MODBUS_WRITE:
        s_system_snapshot.modbus.request_count++;
        s_system_snapshot.modbus.last_request_ms = event->timestamp_ms;
        break;

    case APP_COMM_EVENT_MODBUS_ERROR:
        s_system_snapshot.modbus.exception_count++;
        s_system_snapshot.modbus.last_request_ms = event->timestamp_ms;
        break;

    default:
        break;
    }

    app_critical_exit(primask);
}

void app_system_model_poll(uint32_t now_ms)
{
    uint32_t primask = app_critical_enter();

    if (s_system_snapshot.dsp.online &&
        ((now_ms - s_system_snapshot.dsp.last_rx_ms) > APP_SYSTEM_DSP_TIMEOUT_MS))
    {
        s_system_snapshot.dsp.online = false;
        s_system_snapshot.dsp.timeout_count++;
    }

    if (s_system_snapshot.bms.online &&
        ((now_ms - s_system_snapshot.bms.last_rx_ms) > APP_SYSTEM_BMS_TIMEOUT_MS))
    {
        s_system_snapshot.bms.online = false;
        s_system_snapshot.bms.timeout_count++;
    }

    app_critical_exit(primask);
}

void app_system_model_get_snapshot(app_system_snapshot_t *snapshot)
{
    uint32_t primask;

    if (snapshot == NULL)
    {
        return;
    }

    primask = app_critical_enter();
    *snapshot = s_system_snapshot;
    app_critical_exit(primask);
}
