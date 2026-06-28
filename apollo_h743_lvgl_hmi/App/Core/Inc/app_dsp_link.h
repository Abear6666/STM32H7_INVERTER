#ifndef APP_DSP_LINK_H
#define APP_DSP_LINK_H

#include <stdbool.h>
#include <stdint.h>

#include "app_comm_bus.h"

#define APP_DSP_LINK_FRAME_SIZE 20U

typedef struct
{
    bool online;
    uint32_t rx_frame_count;
    uint32_t valid_frame_count;
    uint32_t crc_error_count;
    uint32_t invalid_frame_count;
    uint32_t timeout_count;
    uint32_t command_tx_count;
    uint32_t ack_count;
    uint32_t last_rx_ms;
    uint32_t last_tx_ms;
    uint8_t last_frame_id;
    uint8_t last_command;
} app_dsp_link_diag_t;

bool app_dsp_link_init(void);
bool app_dsp_link_parse_frame(const uint8_t *frame, uint32_t length, app_comm_event_t *event);
bool app_dsp_link_build_command(uint8_t cmd,
                                const void *payload,
                                uint32_t payload_len,
                                uint8_t *frame,
                                uint32_t frame_size);
void app_dsp_link_poll(uint32_t now_ms);
void app_dsp_link_get_diag(app_dsp_link_diag_t *diag);

#endif
