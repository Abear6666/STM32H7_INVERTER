#ifndef APP_BMS_CAN_H
#define APP_BMS_CAN_H

#include <stdbool.h>
#include <stdint.h>

#include "app_comm_bus.h"

typedef struct
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} app_can_frame_t;

typedef struct
{
    bool online;
    uint32_t rx_frame_count;
    uint32_t status_frame_count;
    uint32_t limits_frame_count;
    uint32_t alarm_frame_count;
    uint32_t invalid_frame_count;
    uint32_t timeout_count;
    uint32_t alarm_change_count;
    uint32_t last_rx_ms;
    uint32_t last_alarm_flags;
    uint32_t last_fault_flags;
    uint32_t last_can_id;
} app_bms_can_diag_t;

bool app_bms_can_init(void);
bool app_bms_can_parse_frame(const app_can_frame_t *frame, app_comm_event_t *event);
void app_bms_can_poll(uint32_t now_ms);
void app_bms_can_get_diag(app_bms_can_diag_t *diag);

#endif
