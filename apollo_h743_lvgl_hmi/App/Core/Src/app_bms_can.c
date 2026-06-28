#include "app_bms_can.h"

#include <stdio.h>
#include <string.h>

#include "app_critical.h"
#include "app_log.h"

#define APP_BMS_CAN_ID_STATUS 0x351U
#define APP_BMS_CAN_ID_LIMITS 0x355U
#define APP_BMS_CAN_ID_ALARM  0x359U

#define APP_BMS_CAN_PERIOD_MS       250U
#define APP_BMS_CAN_TIMEOUT_MS      1000U
#define APP_BMS_DROPOUT_PERIOD_MS   20000U
#define APP_BMS_DROPOUT_DURATION_MS 1300U

static app_bms_can_diag_t s_bms_diag;
static app_bms_status_t s_bms_status;
static uint32_t s_next_frame_ms;
static bool s_timeout_reported;
static bool s_last_online;

static void app_bms_build_status_frame(uint32_t now_ms, app_can_frame_t *frame);
static void app_bms_build_limits_frame(uint32_t now_ms, app_can_frame_t *frame);
static void app_bms_build_alarm_frame(uint32_t now_ms, app_can_frame_t *frame);
static bool app_bms_is_drop_window(uint32_t now_ms);
static uint16_t app_bms_read_u16_le(const uint8_t *src);
static int16_t app_bms_read_i16_le(const uint8_t *src);
static void app_bms_write_u16_le(uint8_t *dst, uint16_t value);
static void app_bms_publish_timeout(uint32_t now_ms);
static void app_bms_note_online_change(bool online);
static void app_bms_log_alarm_change(uint32_t alarm_flags, uint32_t fault_flags);
static void app_bms_copy_status(app_bms_status_t *status);

bool app_bms_can_init(void)
{
    uint32_t primask = app_critical_enter();

    memset(&s_bms_diag, 0, sizeof(s_bms_diag));
    memset(&s_bms_status, 0, sizeof(s_bms_status));
    s_next_frame_ms = 0U;
    s_timeout_reported = false;
    s_last_online = false;

    app_critical_exit(primask);
    app_log_event("Phase 13 BMS CAN ready");
    return true;
}

bool app_bms_can_parse_frame(const app_can_frame_t *frame, app_comm_event_t *event)
{
    app_comm_event_t local_event;
    uint32_t primask;
    uint32_t old_alarm_flags;
    uint32_t old_fault_flags;

    if ((frame == NULL) || (event == NULL) || (frame->dlc != 8U))
    {
        primask = app_critical_enter();
        s_bms_diag.invalid_frame_count++;
        app_critical_exit(primask);
        return false;
    }

    memset(&local_event, 0, sizeof(local_event));
    local_event.source = APP_COMM_SOURCE_BMS_CAN;
    local_event.sequence = frame->id;

    primask = app_critical_enter();
    s_bms_diag.rx_frame_count++;
    s_bms_diag.last_can_id = frame->id;
    old_alarm_flags = s_bms_status.alarm_flags;
    old_fault_flags = s_bms_status.fault_flags;
    app_critical_exit(primask);

    switch (frame->id)
    {
    case APP_BMS_CAN_ID_STATUS:
        primask = app_critical_enter();
        s_bms_status.online = true;
        s_bms_status.soc_percent_x10 = app_bms_read_u16_le(&frame->data[0]);
        s_bms_status.soh_percent_x10 = app_bms_read_u16_le(&frame->data[2]);
        s_bms_status.pack_voltage_v_x10 = app_bms_read_i16_le(&frame->data[4]);
        s_bms_status.pack_current_a_x10 = app_bms_read_i16_le(&frame->data[6]);
        s_bms_diag.status_frame_count++;
        app_critical_exit(primask);
        local_event.type = APP_COMM_EVENT_BMS_STATUS;
        break;

    case APP_BMS_CAN_ID_LIMITS:
        primask = app_critical_enter();
        s_bms_status.online = true;
        s_bms_status.max_temp_c_x10 = app_bms_read_i16_le(&frame->data[0]);
        s_bms_status.charge_limit_a_x10 = app_bms_read_u16_le(&frame->data[2]);
        s_bms_status.discharge_limit_a_x10 = app_bms_read_u16_le(&frame->data[4]);
        s_bms_diag.limits_frame_count++;
        app_critical_exit(primask);
        local_event.type = APP_COMM_EVENT_BMS_LIMITS;
        break;

    case APP_BMS_CAN_ID_ALARM:
        primask = app_critical_enter();
        s_bms_status.online = true;
        s_bms_status.alarm_flags = (uint32_t)app_bms_read_u16_le(&frame->data[0]);
        s_bms_status.fault_flags = (uint32_t)app_bms_read_u16_le(&frame->data[2]);
        s_bms_diag.alarm_frame_count++;
        app_critical_exit(primask);
        local_event.type = APP_COMM_EVENT_BMS_ALARM;
        app_bms_log_alarm_change(s_bms_status.alarm_flags, s_bms_status.fault_flags);
        break;

    default:
        primask = app_critical_enter();
        s_bms_diag.invalid_frame_count++;
        app_critical_exit(primask);
        return false;
    }

    primask = app_critical_enter();
    s_bms_diag.online = true;
    s_timeout_reported = false;
    app_critical_exit(primask);

    if ((old_alarm_flags != s_bms_status.alarm_flags) || (old_fault_flags != s_bms_status.fault_flags))
    {
        primask = app_critical_enter();
        s_bms_diag.alarm_change_count++;
        app_critical_exit(primask);
    }

    app_bms_copy_status(&local_event.data.bms_status);
    *event = local_event;
    app_bms_note_online_change(true);
    return true;
}

void app_bms_can_poll(uint32_t now_ms)
{
    app_can_frame_t frames[3];
    app_comm_event_t event;
    uint32_t primask;
    uint32_t last_rx_ms;
    bool online;
    bool timeout_reported;

    if ((int32_t)(now_ms - s_next_frame_ms) >= 0)
    {
        if (!app_bms_is_drop_window(now_ms))
        {
            app_bms_build_status_frame(now_ms, &frames[0]);
            app_bms_build_limits_frame(now_ms, &frames[1]);
            app_bms_build_alarm_frame(now_ms, &frames[2]);

            for (uint32_t i = 0; i < 3U; ++i)
            {
                if (app_bms_can_parse_frame(&frames[i], &event))
                {
                    event.timestamp_ms = now_ms;
                    primask = app_critical_enter();
                    s_bms_diag.last_rx_ms = now_ms;
                    app_critical_exit(primask);
                    (void)app_comm_publish(&event, 0U);
                }
            }
        }

        s_next_frame_ms = now_ms + APP_BMS_CAN_PERIOD_MS;
    }

    primask = app_critical_enter();
    last_rx_ms = s_bms_diag.last_rx_ms;
    online = s_bms_diag.online;
    timeout_reported = s_timeout_reported;
    app_critical_exit(primask);

    if (online && !timeout_reported && ((now_ms - last_rx_ms) > APP_BMS_CAN_TIMEOUT_MS))
    {
        app_bms_publish_timeout(now_ms);
    }
}

void app_bms_can_get_diag(app_bms_can_diag_t *diag)
{
    uint32_t primask;

    if (diag == NULL)
    {
        return;
    }

    primask = app_critical_enter();
    *diag = s_bms_diag;
    app_critical_exit(primask);
}

static void app_bms_build_status_frame(uint32_t now_ms, app_can_frame_t *frame)
{
    uint16_t seconds = (uint16_t)(now_ms / 1000U);
    int16_t current_a_x10;

    memset(frame, 0, sizeof(*frame));
    frame->id = APP_BMS_CAN_ID_STATUS;
    frame->dlc = 8U;

    current_a_x10 = (int16_t)(-120 + ((int16_t)(seconds % 40U) * 3));
    app_bms_write_u16_le(&frame->data[0], (uint16_t)(760U + (seconds % 30U)));
    app_bms_write_u16_le(&frame->data[2], 985U);
    app_bms_write_u16_le(&frame->data[4], (uint16_t)(5120U + (seconds % 25U)));
    app_bms_write_u16_le(&frame->data[6], (uint16_t)current_a_x10);
}

static void app_bms_build_limits_frame(uint32_t now_ms, app_can_frame_t *frame)
{
    uint16_t seconds = (uint16_t)(now_ms / 1000U);

    memset(frame, 0, sizeof(*frame));
    frame->id = APP_BMS_CAN_ID_LIMITS;
    frame->dlc = 8U;

    app_bms_write_u16_le(&frame->data[0], (uint16_t)(286U + ((seconds * 2U) % 35U)));
    app_bms_write_u16_le(&frame->data[2], 500U);
    app_bms_write_u16_le(&frame->data[4], 600U);
}

static void app_bms_build_alarm_frame(uint32_t now_ms, app_can_frame_t *frame)
{
    uint16_t seconds = (uint16_t)(now_ms / 1000U);
    uint16_t alarm_flags = 0U;
    uint16_t fault_flags = 0U;

    memset(frame, 0, sizeof(*frame));
    frame->id = APP_BMS_CAN_ID_ALARM;
    frame->dlc = 8U;

    if (((seconds / 12U) % 2U) != 0U)
    {
        alarm_flags |= 0x0001U;
    }
    if ((seconds % 45U) >= 42U)
    {
        alarm_flags |= 0x0002U;
    }
    if ((seconds % 90U) >= 88U)
    {
        fault_flags |= 0x0001U;
    }

    app_bms_write_u16_le(&frame->data[0], alarm_flags);
    app_bms_write_u16_le(&frame->data[2], fault_flags);
}

static bool app_bms_is_drop_window(uint32_t now_ms)
{
    if (now_ms < APP_BMS_DROPOUT_PERIOD_MS)
    {
        return false;
    }

    return ((now_ms % APP_BMS_DROPOUT_PERIOD_MS) < APP_BMS_DROPOUT_DURATION_MS);
}

static uint16_t app_bms_read_u16_le(const uint8_t *src)
{
    return (uint16_t)(((uint16_t)src[1] << 8U) | src[0]);
}

static int16_t app_bms_read_i16_le(const uint8_t *src)
{
    return (int16_t)app_bms_read_u16_le(src);
}

static void app_bms_write_u16_le(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xFFU);
    dst[1] = (uint8_t)((value >> 8U) & 0xFFU);
}

static void app_bms_publish_timeout(uint32_t now_ms)
{
    app_comm_event_t event;
    uint32_t primask;

    primask = app_critical_enter();
    s_bms_diag.online = false;
    s_bms_diag.timeout_count++;
    s_bms_status.online = false;
    s_timeout_reported = true;
    app_critical_exit(primask);

    memset(&event, 0, sizeof(event));
    event.type = APP_COMM_EVENT_BMS_TIMEOUT;
    event.timestamp_ms = now_ms;
    event.source = APP_COMM_SOURCE_BMS_CAN;
    event.data.error_code = APP_BMS_CAN_TIMEOUT_MS;
    (void)app_comm_publish(&event, 0U);

    app_bms_note_online_change(false);
}

static void app_bms_note_online_change(bool online)
{
    if (s_last_online != online)
    {
        s_last_online = online;
        app_log_event(online ? "BMS online" : "BMS offline");
    }
}

static void app_bms_log_alarm_change(uint32_t alarm_flags, uint32_t fault_flags)
{
    char message[96];
    uint32_t primask;

    if ((s_bms_diag.last_alarm_flags == alarm_flags) &&
        (s_bms_diag.last_fault_flags == fault_flags))
    {
        return;
    }

    primask = app_critical_enter();
    s_bms_diag.last_alarm_flags = alarm_flags;
    s_bms_diag.last_fault_flags = fault_flags;
    app_critical_exit(primask);

    snprintf(message,
             sizeof(message),
             "BMS alarm=0x%08lX fault=0x%08lX",
             (unsigned long)alarm_flags,
             (unsigned long)fault_flags);
    app_log_event(message);
}

static void app_bms_copy_status(app_bms_status_t *status)
{
    uint32_t primask;

    if (status == NULL)
    {
        return;
    }

    primask = app_critical_enter();
    *status = s_bms_status;
    app_critical_exit(primask);
}
