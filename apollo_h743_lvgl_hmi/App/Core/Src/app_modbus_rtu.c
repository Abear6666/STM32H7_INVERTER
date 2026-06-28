#include "app_modbus_rtu.h"

#include <string.h>

#include "app_comm_bus.h"
#include "app_critical.h"
#include "app_system_model.h"

#define APP_MODBUS_FUNC_READ_HOLDING 0x03U
#define APP_MODBUS_FUNC_READ_INPUT   0x04U
#define APP_MODBUS_FUNC_WRITE_SINGLE 0x06U

#define APP_MODBUS_POLL_PERIOD_MS 1000U

static uint8_t s_slave_addr;
static app_modbus_diag_t s_modbus_diag;
static uint32_t s_next_poll_ms;
static uint8_t s_virtual_step;

static uint16_t app_modbus_crc16(const uint8_t *data, uint32_t length);
static void app_modbus_write_u16_be(uint8_t *dst, uint16_t value);
static uint16_t app_modbus_read_u16_be(const uint8_t *src);
static bool app_modbus_build_read_request(uint8_t function,
                                          uint16_t start_reg,
                                          uint16_t quantity,
                                          uint8_t *tx,
                                          uint32_t tx_size,
                                          uint32_t *tx_len);
static bool app_modbus_build_write_request(uint16_t reg,
                                           uint16_t value,
                                           uint8_t *tx,
                                           uint32_t tx_size,
                                           uint32_t *tx_len);
static void app_modbus_corrupt_crc(uint8_t *frame, uint32_t frame_len);
static void app_modbus_note_request(const app_modbus_request_t *request, uint32_t now_ms);
static void app_modbus_note_response(void);
static void app_modbus_note_crc_error(uint32_t now_ms);
static void app_modbus_note_exception(app_modbus_exception_t exception, uint32_t now_ms);
static void app_modbus_publish_event(app_comm_event_type_t type, uint32_t now_ms, uint32_t code);

bool app_modbus_rtu_init(uint8_t slave_addr)
{
    uint32_t primask = app_critical_enter();

    s_slave_addr = slave_addr;
    memset(&s_modbus_diag, 0, sizeof(s_modbus_diag));
    s_next_poll_ms = 0U;
    s_virtual_step = 0U;

    app_critical_exit(primask);
    return true;
}

bool app_modbus_rtu_parse_request(const uint8_t *rx, uint32_t rx_len, app_modbus_request_t *request)
{
    uint16_t received_crc;
    uint16_t calculated_crc;

    if ((rx == NULL) || (request == NULL) || (rx_len < 8U))
    {
        return false;
    }

    memset(request, 0, sizeof(*request));
    request->slave_addr = rx[0];
    request->function = rx[1];

    received_crc = (uint16_t)(((uint16_t)rx[rx_len - 1U] << 8U) | rx[rx_len - 2U]);
    calculated_crc = app_modbus_crc16(rx, rx_len - 2U);
    if (received_crc != calculated_crc)
    {
        request->exception = APP_MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE;
        return false;
    }

    if (request->slave_addr != s_slave_addr)
    {
        request->exception = APP_MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
        return false;
    }

    switch (request->function)
    {
    case APP_MODBUS_FUNC_READ_HOLDING:
    case APP_MODBUS_FUNC_READ_INPUT:
        request->start_reg = app_modbus_read_u16_be(&rx[2]);
        request->quantity = app_modbus_read_u16_be(&rx[4]);
        if ((request->quantity == 0U) || (request->quantity > 16U))
        {
            request->exception = APP_MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE;
            return false;
        }
        break;

    case APP_MODBUS_FUNC_WRITE_SINGLE:
        request->start_reg = app_modbus_read_u16_be(&rx[2]);
        request->quantity = 1U;
        request->value = app_modbus_read_u16_be(&rx[4]);
        break;

    default:
        request->exception = APP_MODBUS_EXCEPTION_ILLEGAL_FUNCTION;
        return false;
    }

    request->exception = APP_MODBUS_EXCEPTION_NONE;
    return true;
}

bool app_modbus_rtu_build_response(const app_modbus_request_t *request,
                                   uint8_t *tx,
                                   uint32_t tx_size,
                                   uint32_t *tx_len)
{
    uint32_t index;
    uint16_t crc;

    if ((request == NULL) || (tx == NULL) || (tx_len == NULL) || (tx_size < 5U))
    {
        return false;
    }

    memset(tx, 0, tx_size);
    tx[0] = request->slave_addr;

    if (request->exception != APP_MODBUS_EXCEPTION_NONE)
    {
        tx[1] = (uint8_t)(request->function | 0x80U);
        tx[2] = (uint8_t)request->exception;
        crc = app_modbus_crc16(tx, 3U);
        tx[3] = (uint8_t)(crc & 0xFFU);
        tx[4] = (uint8_t)((crc >> 8U) & 0xFFU);
        *tx_len = 5U;
        app_modbus_note_response();
        return true;
    }

    switch (request->function)
    {
    case APP_MODBUS_FUNC_READ_HOLDING:
    case APP_MODBUS_FUNC_READ_INPUT:
        if ((request->quantity > 16U) || (tx_size < (5U + ((uint32_t)request->quantity * 2U))))
        {
            return false;
        }

        tx[1] = request->function;
        tx[2] = (uint8_t)(request->quantity * 2U);
        index = 3U;
        for (uint16_t i = 0; i < request->quantity; ++i)
        {
            uint16_t value;

            if (!app_modbus_map_read((uint16_t)(request->start_reg + i), &value))
            {
                app_modbus_request_t exception = *request;
                exception.exception = APP_MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
                app_modbus_note_exception(exception.exception, request->timestamp_ms);
                return app_modbus_rtu_build_response(&exception, tx, tx_size, tx_len);
            }

            app_modbus_write_u16_be(&tx[index], value);
            index += 2U;
        }
        break;

    case APP_MODBUS_FUNC_WRITE_SINGLE:
        if (!app_modbus_map_write(request->start_reg, request->value))
        {
            app_modbus_request_t exception = *request;
            exception.exception = APP_MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
            app_modbus_note_exception(exception.exception, request->timestamp_ms);
            return app_modbus_rtu_build_response(&exception, tx, tx_size, tx_len);
        }

        tx[1] = request->function;
        app_modbus_write_u16_be(&tx[2], request->start_reg);
        app_modbus_write_u16_be(&tx[4], request->value);
        index = 6U;
        break;

    default:
        return false;
    }

    crc = app_modbus_crc16(tx, index);
    tx[index++] = (uint8_t)(crc & 0xFFU);
    tx[index++] = (uint8_t)((crc >> 8U) & 0xFFU);
    *tx_len = index;
    app_modbus_note_response();
    return true;
}

bool app_modbus_map_read(uint16_t reg, uint16_t *value)
{
    app_system_snapshot_t snapshot;

    if (value == NULL)
    {
        return false;
    }

    app_system_model_get_snapshot(&snapshot);

    switch (reg)
    {
    case 0x0000U:
        *value = snapshot.dsp.online ? 1U : 0U;
        return true;
    case 0x0001U:
        *value = (uint16_t)snapshot.dsp.bus_voltage_v_x10;
        return true;
    case 0x0002U:
        *value = (uint16_t)snapshot.dsp.grid_voltage_v_x10;
        return true;
    case 0x0003U:
        *value = (uint16_t)snapshot.dsp.output_current_a_x10;
        return true;
    case 0x0004U:
        *value = (uint16_t)snapshot.dsp.temperature_c_x10;
        return true;
    case 0x0005U:
        *value = snapshot.dsp.run_state;
        return true;
    case 0x0006U:
        *value = snapshot.dsp.warn_code;
        return true;
    case 0x0007U:
        *value = snapshot.dsp.fault_code;
        return true;

    case 0x0100U:
        *value = snapshot.bms.online ? 1U : 0U;
        return true;
    case 0x0101U:
        *value = snapshot.bms.soc_percent_x10;
        return true;
    case 0x0102U:
        *value = snapshot.bms.soh_percent_x10;
        return true;
    case 0x0103U:
        *value = (uint16_t)snapshot.bms.pack_voltage_v_x10;
        return true;
    case 0x0104U:
        *value = (uint16_t)snapshot.bms.pack_current_a_x10;
        return true;
    case 0x0105U:
        *value = (uint16_t)snapshot.bms.max_temp_c_x10;
        return true;
    case 0x0106U:
        *value = snapshot.bms.charge_limit_a_x10;
        return true;
    case 0x0107U:
        *value = snapshot.bms.discharge_limit_a_x10;
        return true;
    case 0x0108U:
        *value = (uint16_t)(snapshot.bms.alarm_flags & 0xFFFFU);
        return true;
    case 0x0109U:
        *value = (uint16_t)(snapshot.bms.fault_flags & 0xFFFFU);
        return true;

    case 0x0200U:
    case 0x0201U:
    case 0x0202U:
    case 0x0203U:
        *value = 0U;
        return true;

    default:
        return false;
    }
}

bool app_modbus_map_write(uint16_t reg, uint16_t value)
{
    uint32_t primask;

    switch (reg)
    {
    case 0x0200U:
    case 0x0201U:
    case 0x0202U:
    case 0x0203U:
        primask = app_critical_enter();
        s_modbus_diag.write_count++;
        s_modbus_diag.last_reg = reg;
        s_modbus_diag.last_value = value;
        app_critical_exit(primask);
        return true;

    default:
        return false;
    }
}

void app_modbus_rtu_poll(uint32_t now_ms)
{
    uint8_t rx[APP_MODBUS_MAX_FRAME_BYTES];
    uint8_t tx[APP_MODBUS_MAX_FRAME_BYTES];
    uint32_t rx_len = 0U;
    uint32_t tx_len = 0U;
    app_modbus_request_t request;

    if ((int32_t)(now_ms - s_next_poll_ms) < 0)
    {
        return;
    }

    switch (s_virtual_step++ % 5U)
    {
    case 0:
        (void)app_modbus_build_read_request(APP_MODBUS_FUNC_READ_HOLDING, 0x0000U, 8U, rx, sizeof(rx), &rx_len);
        break;
    case 1:
        (void)app_modbus_build_read_request(APP_MODBUS_FUNC_READ_INPUT, 0x0100U, 10U, rx, sizeof(rx), &rx_len);
        break;
    case 2:
        (void)app_modbus_build_write_request(0x0201U, 500U, rx, sizeof(rx), &rx_len);
        break;
    default:
        (void)app_modbus_build_read_request(APP_MODBUS_FUNC_READ_HOLDING, 0x0300U, 2U, rx, sizeof(rx), &rx_len);
        if ((s_virtual_step % 5U) == 0U)
        {
            app_modbus_corrupt_crc(rx, rx_len);
        }
        break;
    }

    if (app_modbus_rtu_parse_request(rx, rx_len, &request))
    {
        request.timestamp_ms = now_ms;
        app_modbus_note_request(&request, now_ms);
        (void)app_modbus_rtu_build_response(&request, tx, sizeof(tx), &tx_len);
        (void)tx_len;
    }
    else
    {
        if (request.exception == APP_MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE)
        {
            app_modbus_note_crc_error(now_ms);
        }
        else
        {
            app_modbus_note_exception(request.exception, now_ms);
        }
    }

    s_next_poll_ms = now_ms + APP_MODBUS_POLL_PERIOD_MS;
}

void app_modbus_get_diag(app_modbus_diag_t *diag)
{
    uint32_t primask;

    if (diag == NULL)
    {
        return;
    }

    primask = app_critical_enter();
    *diag = s_modbus_diag;
    app_critical_exit(primask);
}

static uint16_t app_modbus_crc16(const uint8_t *data, uint32_t length)
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

static void app_modbus_write_u16_be(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)((value >> 8U) & 0xFFU);
    dst[1] = (uint8_t)(value & 0xFFU);
}

static uint16_t app_modbus_read_u16_be(const uint8_t *src)
{
    return (uint16_t)(((uint16_t)src[0] << 8U) | src[1]);
}

static bool app_modbus_build_read_request(uint8_t function,
                                          uint16_t start_reg,
                                          uint16_t quantity,
                                          uint8_t *tx,
                                          uint32_t tx_size,
                                          uint32_t *tx_len)
{
    uint16_t crc;

    if ((tx == NULL) || (tx_len == NULL) || (tx_size < 8U))
    {
        return false;
    }

    tx[0] = s_slave_addr;
    tx[1] = function;
    app_modbus_write_u16_be(&tx[2], start_reg);
    app_modbus_write_u16_be(&tx[4], quantity);
    crc = app_modbus_crc16(tx, 6U);
    tx[6] = (uint8_t)(crc & 0xFFU);
    tx[7] = (uint8_t)((crc >> 8U) & 0xFFU);
    *tx_len = 8U;
    return true;
}

static bool app_modbus_build_write_request(uint16_t reg,
                                           uint16_t value,
                                           uint8_t *tx,
                                           uint32_t tx_size,
                                           uint32_t *tx_len)
{
    uint16_t crc;

    if ((tx == NULL) || (tx_len == NULL) || (tx_size < 8U))
    {
        return false;
    }

    tx[0] = s_slave_addr;
    tx[1] = APP_MODBUS_FUNC_WRITE_SINGLE;
    app_modbus_write_u16_be(&tx[2], reg);
    app_modbus_write_u16_be(&tx[4], value);
    crc = app_modbus_crc16(tx, 6U);
    tx[6] = (uint8_t)(crc & 0xFFU);
    tx[7] = (uint8_t)((crc >> 8U) & 0xFFU);
    *tx_len = 8U;
    return true;
}

static void app_modbus_corrupt_crc(uint8_t *frame, uint32_t frame_len)
{
    if ((frame == NULL) || (frame_len < 2U))
    {
        return;
    }

    frame[frame_len - 2U] ^= 0x01U;
}

static void app_modbus_note_request(const app_modbus_request_t *request, uint32_t now_ms)
{
    uint32_t primask;

    if (request == NULL)
    {
        return;
    }

    primask = app_critical_enter();
    s_modbus_diag.request_count++;
    s_modbus_diag.last_request_ms = now_ms;
    s_modbus_diag.last_reg = request->start_reg;
    s_modbus_diag.last_value = request->value;
    s_modbus_diag.last_function = request->function;
    app_critical_exit(primask);

    app_modbus_publish_event((request->function == APP_MODBUS_FUNC_WRITE_SINGLE) ?
                             APP_COMM_EVENT_MODBUS_WRITE :
                             APP_COMM_EVENT_MODBUS_REQUEST,
                             now_ms,
                             request->start_reg);
}

static void app_modbus_note_response(void)
{
    uint32_t primask = app_critical_enter();
    s_modbus_diag.response_count++;
    app_critical_exit(primask);
}

static void app_modbus_note_crc_error(uint32_t now_ms)
{
    uint32_t primask = app_critical_enter();
    s_modbus_diag.crc_error_count++;
    s_modbus_diag.exception_count++;
    s_modbus_diag.last_request_ms = now_ms;
    s_modbus_diag.last_exception = APP_MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE;
    app_critical_exit(primask);

    app_modbus_publish_event(APP_COMM_EVENT_MODBUS_ERROR, now_ms, APP_COMM_MODBUS_ERROR_CRC);
}

static void app_modbus_note_exception(app_modbus_exception_t exception, uint32_t now_ms)
{
    uint32_t primask = app_critical_enter();
    s_modbus_diag.exception_count++;
    s_modbus_diag.last_request_ms = now_ms;
    s_modbus_diag.last_exception = (uint8_t)exception;
    app_critical_exit(primask);

    app_modbus_publish_event(APP_COMM_EVENT_MODBUS_ERROR, now_ms, APP_COMM_MODBUS_ERROR_EXCEPTION);
}

static void app_modbus_publish_event(app_comm_event_type_t type, uint32_t now_ms, uint32_t code)
{
    app_comm_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.timestamp_ms = now_ms;
    event.source = APP_COMM_SOURCE_MODBUS_RTU;
    event.data.error_code = code;
    (void)app_comm_publish(&event, 0U);
}
