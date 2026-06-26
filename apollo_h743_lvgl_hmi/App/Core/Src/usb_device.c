#include "usb_device.h"

#include "app_usb_cdc.h"
#include "main.h"
#include "usbd_cdc.h"
#include "usbd_core.h"
#include "usbd_desc.h"

USBD_HandleTypeDef hUsbDeviceFS;
static bool s_usb_device_started;

bool app_usb_device_init(void)
{
    if (s_usb_device_started)
    {
        return true;
    }

    app_usb_cdc_reset_buffers();

    if (USBD_Init(&hUsbDeviceFS, &App_USBD_Desc, USE_USB_FS) != USBD_OK)
    {
        return false;
    }

    if (USBD_RegisterClass(&hUsbDeviceFS, USBD_CDC_CLASS) != USBD_OK)
    {
        return false;
    }

    if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &App_USBD_CDC_Interface) != USBD_OK)
    {
        return false;
    }

    if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
    {
        return false;
    }

    s_usb_device_started = true;
    return true;
}

bool app_usb_device_is_ready(void)
{
    return app_usb_cdc_is_configured();
}
