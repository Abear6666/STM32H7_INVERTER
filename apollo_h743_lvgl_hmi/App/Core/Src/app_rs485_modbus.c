#include "app_rs485_modbus.h"

#include <string.h>

#include "app_critical.h"
#include "app_io_expander.h"
#include "app_log.h"
#include "app_modbus_rtu.h"
#include "main.h"
#include "uart.h"

#define APP_RS485_RX_DMA_BUFFER_SIZE  APP_MODBUS_MAX_FRAME_BYTES
#define APP_RS485_TX_SETTLE_DELAY_US  50U
#define APP_RS485_TC_TIMEOUT_MS       20U
#define APP_RS485_MIN_REQUEST_BYTES   8U

static uint8_t s_rx_dma_buffer[APP_RS485_RX_DMA_BUFFER_SIZE];
static uint8_t s_pending_frame[APP_RS485_RX_DMA_BUFFER_SIZE];
static volatile bool s_pending_frame_ready;
static volatile uint16_t s_pending_frame_len;
static volatile uint32_t s_pending_frame_cycles;
static volatile bool s_rx_rearm_needed;
static uint32_t s_cycles_per_us;
static app_rs485_modbus_diag_t s_diag;

static void app_rs485_dwt_init(void);
static uint32_t app_rs485_now_cycles(void);
static uint32_t app_rs485_calculate_t35_us(uint32_t baudrate);
static uint32_t app_rs485_get_actual_baudrate(void);
static uint32_t app_rs485_t35_cycles(void);
static bool app_rs485_t35_elapsed(void);
static void app_rs485_delay_us(uint32_t delay_us);
static void app_rs485_rx_rearm_from_isr(void);
static void app_rs485_rx_rearm_from_task(void);
static bool app_rs485_send_response(const uint8_t *tx, uint32_t tx_len, uint32_t now_ms);

bool app_rs485_modbus_init(void)
{
    memset(&s_diag, 0, sizeof(s_diag));
    memset(s_rx_dma_buffer, 0, sizeof(s_rx_dma_buffer));
    memset(s_pending_frame, 0, sizeof(s_pending_frame));
    s_pending_frame_ready = false;
    s_pending_frame_len = 0U;
    s_pending_frame_cycles = 0U;
    s_rx_rearm_needed = false;
    s_diag.baudrate = APP_RS485_MODBUS_BAUDRATE;
    s_diag.t35_us = APP_RS485_MODBUS_T35_US;

    app_rs485_dwt_init();

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
    s_diag.baudrate = app_rs485_get_actual_baudrate();
    s_diag.t35_us = app_rs485_calculate_t35_us(s_diag.baudrate);
    s_diag.ready = true;
    if (!app_uart2_rx_start_dma(s_rx_dma_buffer, sizeof(s_rx_dma_buffer)))
    {
        s_diag.ready = false;
        app_log_event("RS485 USART2 RX DMA start failed");
        return false;
    }

    app_log_event("RS485 Modbus ready %lu baud T35=%lu us",
                  (unsigned long)s_diag.baudrate,
                  (unsigned long)s_diag.t35_us);
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
    uint32_t rx_len;
    uint32_t tx_len = 0U;
    uint32_t primask;

    if (!s_diag.ready)
    {
        return;
    }

    if (s_rx_rearm_needed)
    {
        app_rs485_rx_rearm_from_task();
    }

    primask = app_critical_enter();
    if (!s_pending_frame_ready)
    {
        app_critical_exit(primask);
        return;
    }
    if (!app_rs485_t35_elapsed())
    {
        app_critical_exit(primask);
        return;
    }

    rx_len = s_pending_frame_len;
    if (rx_len > sizeof(rx))
    {
        rx_len = sizeof(rx);
    }
    memcpy(rx, s_pending_frame, rx_len);
    s_pending_frame_ready = false;
    s_pending_frame_len = 0U;
    app_critical_exit(primask);

    if (rx_len < APP_RS485_MIN_REQUEST_BYTES)
    {
        primask = app_critical_enter();
        s_diag.short_frame_count++;
        app_critical_exit(primask);
        return;
    }

    primask = app_critical_enter();
    s_diag.rx_frame_count++;
    app_critical_exit(primask);

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

void app_uart2_rx_on_idle_from_isr(uint16_t size)
{
    uint32_t now_cycles;
    uint32_t t35_cycles;
    uint32_t pending_len;

    if (!s_diag.ready)
    {
        return;
    }

    if ((size == 0U) || (size > sizeof(s_rx_dma_buffer)))
    {
        s_diag.rx_error_count++;
        app_rs485_rx_rearm_from_isr();
        return;
    }

    s_diag.rx_event_count++;
    s_diag.rx_byte_count += size;
    s_diag.last_rx_ms = HAL_GetTick();
    now_cycles = app_rs485_now_cycles();
    t35_cycles = app_rs485_t35_cycles();

    if (s_pending_frame_ready)
    {
        pending_len = s_pending_frame_len;
        if ((t35_cycles > 0U) &&
            ((uint32_t)(now_cycles - s_pending_frame_cycles) < t35_cycles) &&
            ((pending_len + size) <= sizeof(s_pending_frame)))
        {
            memcpy(&s_pending_frame[pending_len], s_rx_dma_buffer, size);
            s_pending_frame_len = (uint16_t)(pending_len + size);
            s_pending_frame_cycles = now_cycles;
            app_rs485_rx_rearm_from_isr();
            return;
        }

        s_diag.rx_overflow_count++;
        app_rs485_rx_rearm_from_isr();
        return;
    }

    memcpy(s_pending_frame, s_rx_dma_buffer, size);
    s_pending_frame_len = size;
    s_pending_frame_cycles = now_cycles;
    s_pending_frame_ready = true;
    app_rs485_rx_rearm_from_isr();
}

void app_uart2_rx_on_error_from_isr(uint32_t error_code)
{
    (void)error_code;

    if (!s_diag.ready)
    {
        return;
    }

    s_diag.rx_error_count++;
    app_rs485_rx_rearm_from_isr();
}

static void app_rs485_dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    s_cycles_per_us = SystemCoreClock / 1000000U;
}

static uint32_t app_rs485_now_cycles(void)
{
    return DWT->CYCCNT;
}

static uint32_t app_rs485_calculate_t35_us(uint32_t baudrate)
{
    const uint32_t numerator = APP_RS485_MODBUS_BITS_PER_CHAR * 35U * 1000000U;
    const uint32_t denominator = baudrate * 10U;

    if (baudrate == 0U)
    {
        return APP_RS485_MODBUS_T35_US;
    }

    return (numerator + denominator - 1U) / denominator;
}

static uint32_t app_rs485_get_actual_baudrate(void)
{
    uint32_t uart_clock_hz = HAL_RCC_GetPCLK1Freq();
    uint32_t brr = huart2.Instance->BRR;

    if (brr == 0U)
    {
        return APP_RS485_MODBUS_BAUDRATE;
    }

    return (uart_clock_hz + (brr / 2U)) / brr;
}

static uint32_t app_rs485_t35_cycles(void)
{
    if (s_cycles_per_us == 0U)
    {
        return 0U;
    }

    return s_diag.t35_us * s_cycles_per_us;
}

static bool app_rs485_t35_elapsed(void)
{
    uint32_t t35_cycles = app_rs485_t35_cycles();

    if (t35_cycles == 0U)
    {
        return true;
    }

    return ((uint32_t)(app_rs485_now_cycles() - s_pending_frame_cycles) >= t35_cycles);
}

static void app_rs485_delay_us(uint32_t delay_us)
{
    uint32_t delay_cycles;
    uint32_t start_cycles;

    if ((delay_us == 0U) || (s_cycles_per_us == 0U))
    {
        return;
    }

    delay_cycles = delay_us * s_cycles_per_us;
    start_cycles = app_rs485_now_cycles();
    while ((uint32_t)(app_rs485_now_cycles() - start_cycles) < delay_cycles)
    {
        __NOP();
    }
}

static void app_rs485_rx_rearm_from_isr(void)
{
    if (app_uart2_rx_start_dma(s_rx_dma_buffer, sizeof(s_rx_dma_buffer)))
    {
        s_rx_rearm_needed = false;
    }
    else
    {
        s_rx_rearm_needed = true;
    }
}

static void app_rs485_rx_rearm_from_task(void)
{
    uint32_t primask;

    if (!app_uart2_rx_start_dma(s_rx_dma_buffer, sizeof(s_rx_dma_buffer)))
    {
        return;
    }

    primask = app_critical_enter();
    s_rx_rearm_needed = false;
    app_critical_exit(primask);
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
