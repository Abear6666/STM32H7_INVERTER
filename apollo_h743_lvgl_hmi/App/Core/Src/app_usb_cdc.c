#include "app_usb_cdc.h"

#include <string.h>

#include "usbd_core.h"

#define APP_USB_CDC_RX_RING_SIZE 4096U
#define APP_USB_CDC_RX_PACKET_SIZE CDC_DATA_FS_OUT_PACKET_SIZE
#define APP_USB_CDC_TX_PACKET_SIZE CDC_DATA_FS_IN_PACKET_SIZE
#define APP_USB_CDC_TX_TIMEOUT_MS 20U

extern USBD_HandleTypeDef hUsbDeviceFS;

static int8_t app_usb_cdc_init(void);
static int8_t app_usb_cdc_deinit(void);
static int8_t app_usb_cdc_control(uint8_t cmd, uint8_t *pbuf, uint16_t length);
static int8_t app_usb_cdc_receive(uint8_t *buf, uint32_t *len);
static int8_t app_usb_cdc_transmit_complete(uint8_t *buf, uint32_t *len, uint8_t epnum);
static void app_usb_cdc_push_rx(uint8_t byte);
static bool app_usb_cdc_transmit_chunk(const uint8_t *data, uint16_t length);

USBD_CDC_ItfTypeDef App_USBD_CDC_Interface = {
    app_usb_cdc_init,
    app_usb_cdc_deinit,
    app_usb_cdc_control,
    app_usb_cdc_receive,
    app_usb_cdc_transmit_complete,
};

static uint8_t s_rx_packet[APP_USB_CDC_RX_PACKET_SIZE];
static volatile uint8_t s_rx_ring[APP_USB_CDC_RX_RING_SIZE];
static volatile uint16_t s_rx_head;
static volatile uint16_t s_rx_tail;
static volatile bool s_configured;

static USBD_CDC_LineCodingTypeDef s_line_coding = {
    115200U,
    0x00U,
    0x00U,
    0x08U,
};

void app_usb_cdc_reset_buffers(void)
{
    __disable_irq();
    s_rx_head = 0U;
    s_rx_tail = 0U;
    s_configured = false;
    __enable_irq();
}

bool app_usb_cdc_is_configured(void)
{
    return s_configured && (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED);
}

bool app_usb_cdc_rx_read_byte(uint8_t *byte)
{
    uint16_t tail;

    if (byte == NULL)
    {
        return false;
    }

    __disable_irq();
    tail = s_rx_tail;
    if (tail == s_rx_head)
    {
        __enable_irq();
        return false;
    }

    *byte = s_rx_ring[tail];
    s_rx_tail = (uint16_t)((tail + 1U) % APP_USB_CDC_RX_RING_SIZE);
    __enable_irq();
    return true;
}

void app_usb_cdc_write(const uint8_t *data, size_t length)
{
    while ((data != NULL) && (length > 0U) && app_usb_cdc_is_configured())
    {
        uint16_t chunk = (length > APP_USB_CDC_TX_PACKET_SIZE) ? APP_USB_CDC_TX_PACKET_SIZE : (uint16_t)length;

        if (!app_usb_cdc_transmit_chunk(data, chunk))
        {
            return;
        }

        data += chunk;
        length -= chunk;
    }
}

void app_usb_cdc_write_text(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    app_usb_cdc_write((const uint8_t *)text, strlen(text));
}

static int8_t app_usb_cdc_init(void)
{
    s_configured = true;
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, NULL, 0U);
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, s_rx_packet);
    return (int8_t)USBD_OK;
}

static int8_t app_usb_cdc_deinit(void)
{
    s_configured = false;
    return (int8_t)USBD_OK;
}

static int8_t app_usb_cdc_control(uint8_t cmd, uint8_t *pbuf, uint16_t length)
{
    (void)length;

    switch (cmd)
    {
    case CDC_SET_LINE_CODING:
        s_line_coding.bitrate = (uint32_t)(pbuf[0] |
                                           ((uint32_t)pbuf[1] << 8) |
                                           ((uint32_t)pbuf[2] << 16) |
                                           ((uint32_t)pbuf[3] << 24));
        s_line_coding.format = pbuf[4];
        s_line_coding.paritytype = pbuf[5];
        s_line_coding.datatype = pbuf[6];
        break;

    case CDC_GET_LINE_CODING:
        pbuf[0] = (uint8_t)s_line_coding.bitrate;
        pbuf[1] = (uint8_t)(s_line_coding.bitrate >> 8);
        pbuf[2] = (uint8_t)(s_line_coding.bitrate >> 16);
        pbuf[3] = (uint8_t)(s_line_coding.bitrate >> 24);
        pbuf[4] = s_line_coding.format;
        pbuf[5] = s_line_coding.paritytype;
        pbuf[6] = s_line_coding.datatype;
        break;

    case CDC_SET_CONTROL_LINE_STATE:
        s_configured = true;
        break;

    default:
        break;
    }

    return (int8_t)USBD_OK;
}

static int8_t app_usb_cdc_receive(uint8_t *buf, uint32_t *len)
{
    if ((buf != NULL) && (len != NULL))
    {
        for (uint32_t i = 0U; i < *len; ++i)
        {
            app_usb_cdc_push_rx(buf[i]);
        }
    }

    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, s_rx_packet);
    (void)USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    return (int8_t)USBD_OK;
}

static int8_t app_usb_cdc_transmit_complete(uint8_t *buf, uint32_t *len, uint8_t epnum)
{
    (void)buf;
    (void)len;
    (void)epnum;
    return (int8_t)USBD_OK;
}

static void app_usb_cdc_push_rx(uint8_t byte)
{
    uint16_t next = (uint16_t)((s_rx_head + 1U) % APP_USB_CDC_RX_RING_SIZE);

    if (next != s_rx_tail)
    {
        s_rx_ring[s_rx_head] = byte;
        s_rx_head = next;
    }
}

static bool app_usb_cdc_transmit_chunk(const uint8_t *data, uint16_t length)
{
    uint32_t start = HAL_GetTick();

    while (app_usb_cdc_is_configured())
    {
        USBD_CDC_SetTxBuffer(&hUsbDeviceFS, (uint8_t *)data, length);
        if (USBD_CDC_TransmitPacket(&hUsbDeviceFS) == USBD_OK)
        {
            return true;
        }

        if ((HAL_GetTick() - start) >= APP_USB_CDC_TX_TIMEOUT_MS)
        {
            return false;
        }
    }

    return false;
}
