#include "app_rs485_modbus.h"

#include <string.h>

#include "app_critical.h"
#include "app_io_expander.h"
#include "app_log.h"
#include "app_modbus_rtu.h"
#include "main.h"
#include "uart.h"

#define APP_RS485_RX_RING_SIZE        128U
#define APP_RS485_FRAME_GAP_MS        5U
#define APP_RS485_TX_SETTLE_DELAY_US  50U
#define APP_RS485_TC_TIMEOUT_MS       20U

static volatile uint8_t s_rx_ring[APP_RS485_RX_RING_SIZE];
static volatile uint16_t s_rx_head;
static volatile uint16_t s_rx_tail;
static volatile uint32_t s_last_rx_ms;
static app_rs485_modbus_diag_t s_diag;

static bool app_rs485_ring_read(uint8_t *byte);
static bool app_rs485_ring_has_data(void);
static void app_rs485_delay_us(uint32_t delay_us);
static bool app_rs485_send_response(const uint8_t *tx, uint32_t tx_len, uint32_t now_ms);

bool app_rs485_modbus_init(void)
{
    memset(&s_diag, 0, sizeof(s_diag));
    s_rx_head = 0U;
    s_rx_tail = 0U;
    s_last_rx_ms = 0U;

    if (!app_io_expander_init())
    {
        app_log_event("RS485 IO expander init failed");
        return false;
    }

    if (!app_io_expander_set_rs485_tx(false))
    {
        app_log_event("RS485 direction init failed");
        return false;
    }

    app_uart2_init(APP_RS485_MODBUS_BAUDRATE);
    app_uart2_rx_start_it();

    s_diag.ready = true;
    app_log_event("RS485 Modbus transport ready");
    return true;
}

bool app_rs485_modbus_is_ready(void)
{
    return s_diag.ready;
}

void app_rs485_modbus_poll(uint32_t now_ms)
{
    uint8_t rx[APP_MODBUS_MAX_FRAME_BYTES];
    uint8_t tx[APP_MODBUS_MAX_FRAME_BYTES];
    uint32_t rx_len = 0U;
    uint32_t tx_len = 0U;
    bool frame_overflow = false;

    if (!s_diag.ready)
    {
        return;
    }

    if (!app_rs485_ring_has_data())
    {
        return;
    }

    if ((uint32_t)(now_ms - s_last_rx_ms) < APP_RS485_FRAME_GAP_MS)
    {
        return;
    }

    while (app_rs485_ring_read(&rx[rx_len]))
    {
        rx_len++;
        if (rx_len >= sizeof(rx))
        {
            uint8_t discard;

            if (app_rs485_ring_read(&discard))
            {
                frame_overflow = true;
                while (app_rs485_ring_read(&discard))
                {
                }
            }
            break;
        }
    }

    if (frame_overflow)
    {
        uint32_t primask = app_critical_enter();
        s_diag.rx_overflow_count++;
        app_critical_exit(primask);
        return;
    }

    if (rx_len < 4U)
    {
        uint32_t primask = app_critical_enter();
        s_diag.short_frame_count++;
        app_critical_exit(primask);
        return;
    }

    {
        uint32_t primask = app_critical_enter();
        s_diag.rx_frame_count++;
        app_critical_exit(primask);
    }

    if (app_modbus_rtu_process_frame(rx, rx_len, tx, sizeof(tx), &tx_len, now_ms) && (tx_len > 0U))
    {
        (void)app_rs485_send_response(tx, tx_len, now_ms);
    }
}

void app_rs485_modbus_get_diag(app_rs485_modbus_diag_t *diag)
{
    uint32_t primask;

    if (diag == NULL)
    {
        return;
    }

    primask = app_critical_enter();
    *diag = s_diag;
    app_critical_exit(primask);
}

void app_uart2_rx_on_byte_from_isr(uint8_t byte)
{
    uint16_t head = s_rx_head;
    uint16_t next = (uint16_t)((head + 1U) % APP_RS485_RX_RING_SIZE);

    if (next == s_rx_tail)
    {
        s_diag.rx_overflow_count++;
        return;
    }

    s_rx_ring[head] = byte;
    s_rx_head = next;
    s_last_rx_ms = HAL_GetTick();
    s_diag.rx_byte_count++;
    s_diag.last_rx_ms = s_last_rx_ms;
}

static bool app_rs485_ring_read(uint8_t *byte)
{
    uint32_t primask;
    uint16_t tail;

    if (byte == NULL)
    {
        return false;
    }

    primask = app_critical_enter();
    tail = s_rx_tail;
    if (tail == s_rx_head)
    {
        app_critical_exit(primask);
        return false;
    }

    *byte = s_rx_ring[tail];
    s_rx_tail = (uint16_t)((tail + 1U) % APP_RS485_RX_RING_SIZE);
    app_critical_exit(primask);
    return true;
}

static bool app_rs485_ring_has_data(void)
{
    bool has_data;
    uint32_t primask = app_critical_enter();

    has_data = (s_rx_tail != s_rx_head);
    app_critical_exit(primask);
    return has_data;
}

static void app_rs485_delay_us(uint32_t delay_us)
{
    uint32_t loops = delay_us * 80U;

    while (loops-- > 0U)
    {
        __NOP();
    }
}

static bool app_rs485_send_response(const uint8_t *tx, uint32_t tx_len, uint32_t now_ms)
{
    HAL_StatusTypeDef status;
    uint32_t deadline;

    if ((tx == NULL) || (tx_len == 0U) || (tx_len > UINT16_MAX))
    {
        return false;
    }

    if (!app_io_expander_set_rs485_tx(true))
    {
        s_diag.tx_error_count++;
        s_diag.ready = false;
        return false;
    }

    app_rs485_delay_us(APP_RS485_TX_SETTLE_DELAY_US);
    status = HAL_UART_Transmit(&huart2, (uint8_t *)tx, (uint16_t)tx_len, 100U);
    deadline = HAL_GetTick() + APP_RS485_TC_TIMEOUT_MS;
    while ((__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET) && (status == HAL_OK))
    {
        if ((int32_t)(HAL_GetTick() - deadline) >= 0)
        {
            status = HAL_TIMEOUT;
            break;
        }
    }
    app_rs485_delay_us(APP_RS485_TX_SETTLE_DELAY_US);

    if (!app_io_expander_set_rs485_tx(false))
    {
        s_diag.tx_error_count++;
        s_diag.ready = false;
        return false;
    }

    if (status != HAL_OK)
    {
        s_diag.tx_error_count++;
        return false;
    }

    s_diag.tx_frame_count++;
    s_diag.last_tx_ms = now_ms;
    return true;
}
