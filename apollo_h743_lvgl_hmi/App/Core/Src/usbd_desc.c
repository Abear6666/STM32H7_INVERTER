#include "usbd_desc.h"

#include "stm32h7xx_hal.h"
#include "usbd_core.h"
#include "usbd_conf.h"

#define USBD_VID                      0x0483U
#define USBD_PID                      0x5740U
#define USBD_LANGID_STRING            0x0409U
#define USBD_MANUFACTURER_STRING      "Apollo H743"
#define USBD_PRODUCT_FS_STRING        "Apollo H743 IAP CDC"
#define USBD_CONFIGURATION_FS_STRING  "CDC Config"
#define USBD_INTERFACE_FS_STRING      "CDC Interface"
#define APP_USB_SIZ_STRING_SERIAL     26U

static uint8_t *app_usbd_device_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *app_usbd_langid_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *app_usbd_manufacturer_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *app_usbd_product_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *app_usbd_serial_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *app_usbd_config_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *app_usbd_interface_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static void app_usbd_int_to_unicode(uint32_t value, uint8_t *pbuf, uint8_t len);
static void app_usbd_get_serial(void);

USBD_DescriptorsTypeDef App_USBD_Desc = {
    app_usbd_device_descriptor,
    app_usbd_langid_descriptor,
    app_usbd_manufacturer_descriptor,
    app_usbd_product_descriptor,
    app_usbd_serial_descriptor,
    app_usbd_config_descriptor,
    app_usbd_interface_descriptor,
};

__ALIGN_BEGIN static uint8_t s_device_desc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12,
    USB_DESC_TYPE_DEVICE,
    0x00,
    0x02,
    0xEF,
    0x02,
    0x01,
    USB_MAX_EP0_SIZE,
    LOBYTE(USBD_VID),
    HIBYTE(USBD_VID),
    LOBYTE(USBD_PID),
    HIBYTE(USBD_PID),
    0x00,
    0x02,
    USBD_IDX_MFC_STR,
    USBD_IDX_PRODUCT_STR,
    USBD_IDX_SERIAL_STR,
    USBD_MAX_NUM_CONFIGURATION,
};

__ALIGN_BEGIN static uint8_t s_langid_desc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
    USB_LEN_LANGID_STR_DESC,
    USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID_STRING),
    HIBYTE(USBD_LANGID_STRING),
};

__ALIGN_BEGIN static uint8_t s_serial_desc[APP_USB_SIZ_STRING_SERIAL] __ALIGN_END = {
    APP_USB_SIZ_STRING_SERIAL,
    USB_DESC_TYPE_STRING,
};

__ALIGN_BEGIN static uint8_t s_str_desc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

static uint8_t *app_usbd_device_descriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(s_device_desc);
    return s_device_desc;
}

static uint8_t *app_usbd_langid_descriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(s_langid_desc);
    return s_langid_desc;
}

static uint8_t *app_usbd_manufacturer_descriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, s_str_desc, length);
    return s_str_desc;
}

static uint8_t *app_usbd_product_descriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_PRODUCT_FS_STRING, s_str_desc, length);
    return s_str_desc;
}

static uint8_t *app_usbd_serial_descriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    *length = APP_USB_SIZ_STRING_SERIAL;
    app_usbd_get_serial();
    return s_serial_desc;
}

static uint8_t *app_usbd_config_descriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_FS_STRING, s_str_desc, length);
    return s_str_desc;
}

static uint8_t *app_usbd_interface_descriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed;
    USBD_GetString((uint8_t *)USBD_INTERFACE_FS_STRING, s_str_desc, length);
    return s_str_desc;
}

static void app_usbd_int_to_unicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
    for (uint8_t idx = 0U; idx < len; ++idx)
    {
        uint8_t digit = (uint8_t)(value >> 28);
        pbuf[2U * idx] = (digit < 10U) ? (uint8_t)(digit + '0') : (uint8_t)(digit + 'A' - 10U);
        pbuf[(2U * idx) + 1U] = 0U;
        value <<= 4;
    }
}

static void app_usbd_get_serial(void)
{
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();

    uid0 += uid2;
    if (uid0 != 0U)
    {
        app_usbd_int_to_unicode(uid0, &s_serial_desc[2], 8U);
        app_usbd_int_to_unicode(uid1, &s_serial_desc[18], 4U);
    }
}
