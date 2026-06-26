#ifndef APP_USB_CDC_H
#define APP_USB_CDC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "usbd_cdc.h"

extern USBD_CDC_ItfTypeDef App_USBD_CDC_Interface;

void app_usb_cdc_reset_buffers(void);
bool app_usb_cdc_is_configured(void);
bool app_usb_cdc_rx_read_byte(uint8_t *byte);
void app_usb_cdc_write(const uint8_t *data, size_t length);
void app_usb_cdc_write_text(const char *text);

#endif
