#include "app_dsp_link.h"

#include <string.h>

#include "app_critical.h"
#include "app_log.h"

#define APP_DSP_FRAME_TYPE_STATUS  0xA1U
#define APP_DSP_FRAME_TYPE_COMMAND 0x5AU
#define APP_DSP_FRAME_TYPE_ACK     0xACU

#define APP_DSP_STATUS_PERIOD_MS        100U
#define APP_DSP_TIMEOUT_MS              500U
#define APP_DSP_CRC_INJECT_PERIOD       40U
#define APP_DSP_COMMAND_INJECT_PERIOD   30U
#define APP_DSP_DROPOUT_PERIOD_MS       15000U
#define APP_DSP_DROPOUT_DURATION_MS     700U
#define APP_DSP_FRAME_PAYLOAD_OFFSET    2U
#define APP_DSP_FRAME_PAYLOAD_BYTES     16U
#define APP_DSP_FRAME_CRC_OFFSET        18U

static app_dsp_link_diag_t s_dsp_diag;
static uint32_t s_next_status_ms;
static uint32_t s_next_command_ms;
static uint8_t s_sequence;
static bool s_timeout_reported;
static bool s_last_online;

static uint16_t app_dsp_crc16_modbus(const uint8_t *data, uint32_t length);
static void app_dsp_write_u16_le(uint8_t *dst, uint16_t value);
static uint16_t app_dsp_read_u16_le(const uint8_t *src);
static void app_dsp_build_status_frame(uint32_t now_ms, uint8_t *frame, bool corrupt_crc);
static bool app_dsp_is_drop_window(uint32_t now_ms);
static void app_dsp_note_status_time(uint32_t now_ms);
static void app_dsp_note_command_time(uint32_t now_ms);
static void app_dsp_note_online_change(bool online);
static void app_dsp_publish_crc_error(uint32_t now_ms);
static void app_dsp_publish_timeout(uint32_t now_ms);

bool app_dsp_link_init(void)
{
    uint32_t primask = app_critical_enter();

    memset(&s_dsp_diag, 0, sizeof(s_dsp_diag));
    s_next_status_ms = 0U;
    s_next_command_ms = APP_DSP_STATUS_PERIOD_MS;
    s_sequence = 0U;
    s_timeout_reported = false;
    s_last_online = false;

    app_critical_exit(primask);
    app_log_event("Phase 13 DSP link ready");
    return true;
}

bool app_dsp_link_parse_frame(const uint8_t *frame, uint32_t length, app_comm_event_t *event)
{
    uint16_t received_crc;
    uint16_t calculated_crc;
    app_comm_event_t local_event;
    uint32_t primask;

    if ((frame == NULL) || (event == NULL) || (length != APP_DSP_LINK_FRAME_SIZE))
    {
        primask = app_critical_enter();
        s_dsp_diag.invalid_frame_count++;
        app_critical_exit(primask);
        return false;
    }

    primask = app_critical_enter();
    s_dsp_diag.rx_frame_count++;
    app_critical_exit(primask);

    received_crc = app_dsp_read_u16_le(&frame[APP_DSP_FRAME_CRC_OFFSET]);
    calculated_crc = app_dsp_crc16_modbus(frame, APP_DSP_FRAME_CRC_OFFSET);
    if (received_crc != calculated_crc)
    {
        primask = app_critical_enter();
        s_dsp_diag.crc_error_count++;
        app_critical_exit(primask);
        return false;
    }

    if (frame[0] == APP_DSP_FRAME_TYPE_STATUS)
    {
        uint32_t timestamp_ms = app_dsp_read_u16_le(&frame[APP_DSP_FRAME_PAYLOAD_OFFSET]);

        memset(&local_event, 0, sizeof(local_event));
        local_event.type = APP_COMM_EVENT_DSP_STATUS;
        local_event.timestamp_ms = timestamp_ms;
        local_event.source = APP_COMM_SOURCE_DSP_LINK;
        local_event.sequence = frame[1];
        local_event.data.dsp_status.online = true;
        local_event.data.dsp_status.bus_voltage_v_x10 = (int16_t)app_dsp_read_u16_le(&frame[4]);
        local_event.data.dsp_status.grid_voltage_v_x10 = (int16_t)app_dsp_read_u16_le(&frame[6]);
        local_event.data.dsp_status.output_current_a_x10 = (int16_t)app_dsp_read_u16_le(&frame[8]);
        local_event.data.dsp_status.temperature_c_x10 = (int16_t)app_dsp_read_u16_le(&frame[10]);
        local_event.data.dsp_status.run_state = app_dsp_read_u16_le(&frame[12]);
        local_event.data.dsp_status.warn_code = app_dsp_read_u16_le(&frame[14]);
        local_event.data.dsp_status.fault_code = app_dsp_read_u16_le(&frame[16]);
        local_event.data.dsp_status.firmware_version = 0x0201U;

        primask = app_critical_enter();
        s_dsp_diag.online = true;
        s_dsp_diag.valid_frame_count++;
        s_dsp_diag.last_rx_ms = timestamp_ms;
        s_dsp_diag.last_frame_id = frame[1];
        s_timeout_reported = false;
        app_critical_exit(primask);

        *event = local_event;
        app_dsp_note_online_change(true);
        return true;
    }

    if (frame[0] == APP_DSP_FRAME_TYPE_ACK)
    {
        memset(&local_event, 0, sizeof(local_event));
        local_event.type = APP_COMM_EVENT_DSP_ACK;
        local_event.timestamp_ms = app_dsp_read_u16_le(&frame[APP_DSP_FRAME_PAYLOAD_OFFSET]);
        local_event.source = APP_COMM_SOURCE_DSP_LINK;
        local_event.sequence = frame[1];
        local_event.data.error_code = app_dsp_read_u16_le(&frame[4]);

        primask = app_critical_enter();
        s_dsp_diag.ack_count++;
        s_dsp_diag.last_frame_id = frame[1];
        app_critical_exit(primask);

        *event = local_event;
        return true;
    }

    primask = app_critical_enter();
    s_dsp_diag.invalid_frame_count++;
    app_critical_exit(primask);
    return false;
}

bool app_dsp_link_build_command(uint8_t cmd,
                                const void *payload,
                                uint32_t payload_len,
                                uint8_t *frame,
                                uint32_t frame_size)
{
    const uint8_t *payload_bytes = (const uint8_t *)payload;
    uint16_t crc;
    uint32_t copy_len;
    uint32_t primask;

    if ((frame == NULL) ||
        (frame_size < APP_DSP_LINK_FRAME_SIZE) ||
        (payload_len > (APP_DSP_FRAME_PAYLOAD_BYTES - 1U)) ||
        ((payload == NULL) && (payload_len > 0U)))
    {
        return false;
    }

    memset(frame, 0, APP_DSP_LINK_FRAME_SIZE);
    frame[0] = APP_DSP_FRAME_TYPE_COMMAND;
    frame[1] = s_sequence++;
    frame[APP_DSP_FRAME_PAYLOAD_OFFSET] = cmd;

    copy_len = payload_len;
    if ((payload_bytes != NULL) && (copy_len > 0U))
    {
        memcpy(&frame[APP_DSP_FRAME_PAYLOAD_OFFSET + 1U], payload_bytes, copy_len);
    }

    crc = app_dsp_crc16_modbus(frame, APP_DSP_FRAME_CRC_OFFSET);
    app_dsp_write_u16_le(&frame[APP_DSP_FRAME_CRC_OFFSET], crc);

    primask = app_critical_enter();
    s_dsp_diag.command_tx_count++;
    s_dsp_diag.last_command = cmd;
    app_critical_exit(primask);

    return true;
}

void app_dsp_link_poll(uint32_t now_ms)
{
    uint8_t frame[APP_DSP_LINK_FRAME_SIZE];
    app_comm_event_t event;
    bool corrupt_crc;
    uint32_t primask;
    uint32_t last_rx_ms;
    bool online;
    bool timeout_reported;

    if ((int32_t)(now_ms - s_next_status_ms) >= 0)
    {
        if (!app_dsp_is_drop_window(now_ms))
        {
            corrupt_crc = ((s_sequence != 0U) && ((s_sequence % APP_DSP_CRC_INJECT_PERIOD) == 0U));
            app_dsp_build_status_frame(now_ms, frame, corrupt_crc);

            if (app_dsp_link_parse_frame(frame, APP_DSP_LINK_FRAME_SIZE, &event))
            {
                event.timestamp_ms = now_ms;
                app_dsp_note_status_time(now_ms);
                (void)app_comm_publish(&event, 0U);
            }
            else if (corrupt_crc)
            {
                app_dsp_publish_crc_error(now_ms);
            }
        }

        s_next_status_ms = now_ms + APP_DSP_STATUS_PERIOD_MS;
    }

    if ((int32_t)(now_ms - s_next_command_ms) >= 0)
    {
        uint16_t limit_w_x10 = 35000U;

        if (app_dsp_link_build_command(0x10U,
                                       &limit_w_x10,
                                       sizeof(limit_w_x10),
                                       frame,
                                       sizeof(frame)))
        {
            app_comm_event_t ack;

            app_dsp_note_command_time(now_ms);
            memset(frame, 0, sizeof(frame));
            frame[0] = APP_DSP_FRAME_TYPE_ACK;
            frame[1] = s_sequence++;
            app_dsp_write_u16_le(&frame[2], (uint16_t)(now_ms & 0xFFFFU));
            app_dsp_write_u16_le(&frame[4], 0U);
            app_dsp_write_u16_le(&frame[APP_DSP_FRAME_CRC_OFFSET],
                                 app_dsp_crc16_modbus(frame, APP_DSP_FRAME_CRC_OFFSET));

            if (app_dsp_link_parse_frame(frame, sizeof(frame), &ack))
            {
                ack.timestamp_ms = now_ms;
                (void)app_comm_publish(&ack, 0U);
            }
        }

        s_next_command_ms = now_ms + (APP_DSP_STATUS_PERIOD_MS * APP_DSP_COMMAND_INJECT_PERIOD);
    }

    primask = app_critical_enter();
    last_rx_ms = s_dsp_diag.last_rx_ms;
    online = s_dsp_diag.online;
    timeout_reported = s_timeout_reported;
    app_critical_exit(primask);

    if (online && !timeout_reported && ((now_ms - last_rx_ms) > APP_DSP_TIMEOUT_MS))
    {
        app_dsp_publish_timeout(now_ms);
    }
}

void app_dsp_link_get_diag(app_dsp_link_diag_t *diag)
{
    uint32_t primask;

    if (diag == NULL)
    {
        return;
    }

    primask = app_critical_enter();
    *diag = s_dsp_diag;
    app_critical_exit(primask);
}

static uint16_t app_dsp_crc16_modbus(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFFU;

    for (uint32_t i = 0; i < length; ++i)
    {
        crc ^= data[i];
        for (uint32_t bit = 0; bit < 8U; ++bit)
        {
            if ((crc & 0x0001U) != 0U)
            {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

static void app_dsp_write_u16_le(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xFFU);
    dst[1] = (uint8_t)((value >> 8U) & 0xFFU);
}

static uint16_t app_dsp_read_u16_le(const uint8_t *src)
{
    return (uint16_t)(((uint16_t)src[1] << 8U) | src[0]);
}

static void app_dsp_build_status_frame(uint32_t now_ms, uint8_t *frame, bool corrupt_crc)
{
    uint16_t seconds = (uint16_t)(now_ms / 1000U);
    uint16_t crc;

    memset(frame, 0, APP_DSP_LINK_FRAME_SIZE);
    frame[0] = APP_DSP_FRAME_TYPE_STATUS;
    frame[1] = s_sequence++;

    app_dsp_write_u16_le(&frame[2], (uint16_t)(now_ms & 0xFFFFU));
    app_dsp_write_u16_le(&frame[4], (uint16_t)(3800U + (seconds % 30U)));
    app_dsp_write_u16_le(&frame[6], (uint16_t)(2200U + (seconds % 10U)));
    app_dsp_write_u16_le(&frame[8], (uint16_t)(120U + ((seconds * 3U) % 40U)));
    app_dsp_write_u16_le(&frame[10], (uint16_t)(315U + ((seconds * 2U) % 35U)));
    app_dsp_write_u16_le(&frame[12], 1U);
    app_dsp_write_u16_le(&frame[14], ((seconds % 60U) == 0U) ? 0x0001U : 0U);
    app_dsp_write_u16_le(&frame[16], 0U);

    crc = app_dsp_crc16_modbus(frame, APP_DSP_FRAME_CRC_OFFSET);
    if (corrupt_crc)
    {
        crc ^= 0x0001U;
    }
    app_dsp_write_u16_le(&frame[APP_DSP_FRAME_CRC_OFFSET], crc);
}

static bool app_dsp_is_drop_window(uint32_t now_ms)
{
    if (now_ms < APP_DSP_DROPOUT_PERIOD_MS)
    {
        return false;
    }

    return ((now_ms % APP_DSP_DROPOUT_PERIOD_MS) < APP_DSP_DROPOUT_DURATION_MS);
}

static void app_dsp_note_status_time(uint32_t now_ms)
{
    uint32_t primask = app_critical_enter();
    s_dsp_diag.last_rx_ms = now_ms;
    app_critical_exit(primask);
}

static void app_dsp_note_command_time(uint32_t now_ms)
{
    uint32_t primask = app_critical_enter();
    s_dsp_diag.last_tx_ms = now_ms;
    app_critical_exit(primask);
}

static void app_dsp_note_online_change(bool online)
{
    if (s_last_online != online)
    {
        s_last_online = online;
        app_log_event(online ? "DSP online" : "DSP offline");
    }
}

static void app_dsp_publish_crc_error(uint32_t now_ms)
{
    app_comm_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = APP_COMM_EVENT_DSP_CRC_ERROR;
    event.timestamp_ms = now_ms;
    event.source = APP_COMM_SOURCE_DSP_LINK;
    event.data.error_code = 1U;
    (void)app_comm_publish(&event, 0U);
}

static void app_dsp_publish_timeout(uint32_t now_ms)
{
    app_comm_event_t event;
    uint32_t primask;

    primask = app_critical_enter();
    s_dsp_diag.online = false;
    s_dsp_diag.timeout_count++;
    s_timeout_reported = true;
    app_critical_exit(primask);

    memset(&event, 0, sizeof(event));
    event.type = APP_COMM_EVENT_DSP_TIMEOUT;
    event.timestamp_ms = now_ms;
    event.source = APP_COMM_SOURCE_DSP_LINK;
    event.data.error_code = APP_DSP_TIMEOUT_MS;
    (void)app_comm_publish(&event, 0U);

    app_dsp_note_online_change(false);
}
