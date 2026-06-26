#include "app_iap.h"

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app_crc32.h"
#include "app_critical.h"
#include "app_iap_record.h"
#include "app_log.h"
#include "app_sd.h"
#include "app_tag.h"
#include "app_usb_cdc.h"
#include "app_w25q256.h"
#include "ff.h"
#include "main.h"
#include "task.h"
#include "uart.h"
#include "usb_device.h"

#define APP_IAP_SERIAL_LINE_LEN      96U
#define APP_IAP_DEMO_IMAGE_SIZE      1024U
#define APP_IAP_RECV_BUFFER_SIZE     256U
#define APP_IAP_RECV_MAX_SIZE        APP_IAP_W25Q_STAGING_SIZE
#define APP_IAP_RECV_PROGRESS_STEP   (16UL * 1024UL)
#define APP_IAP_SD_PACKAGE_PATH      "0:/app_b_slot.bin"

#ifndef APP_BUILD_RUN_ADDR
#define APP_BUILD_RUN_ADDR APP_A_RUN_ADDR
#endif

#ifndef APP_BUILD_VERSION
#define APP_BUILD_VERSION 1U
#endif

typedef enum
{
    APP_IAP_RECV_MODE_LINE = 0,
    APP_IAP_RECV_MODE_BINARY,
} app_iap_recv_mode_t;

typedef struct
{
    app_iap_recv_mode_t mode;
    app_iap_source_t source;
    uint32_t expected_size;
    uint32_t expected_crc32;
    uint32_t version;
    uint32_t received_size;
    uint32_t crc32;
    uint32_t write_addr;
    uint32_t next_progress;
    uint8_t buffer[APP_IAP_RECV_BUFFER_SIZE];
    uint32_t buffer_len;
} app_iap_recv_t;

static app_iap_status_t s_iap_status;
static bool s_demo_stage_requested;
static bool s_sd_stage_requested;
static app_iap_source_t s_demo_source;
static app_iap_recv_t s_recv;
static char s_serial_line[APP_IAP_SERIAL_LINE_LEN];
static uint32_t s_serial_line_len;
static char s_usb_line[APP_IAP_SERIAL_LINE_LEN];
static uint32_t s_usb_line_len;
static uint8_t s_demo_image[APP_IAP_DEMO_IMAGE_SIZE];
static uint8_t s_verify_buffer[APP_IAP_RECV_BUFFER_SIZE];

static void app_iap_set_status(const char *text);
static void app_iap_set_recv_progress(bool active, uint32_t received, uint32_t expected);
static void app_iap_set_pending_status(const app_iap_pending_t *pending, const char *text);
static void app_iap_clear_pending_status(void);
static void app_iap_set_boot_state_status(const app_iap_boot_state_t *state, bool valid);
static void app_iap_print_sources(void);
static void app_iap_reply_source(app_iap_source_t source, const char *text);
static void app_iap_poll_serial(void);
static void app_iap_poll_usb(void);
static void app_iap_poll_stream_byte(app_iap_source_t source, uint8_t ch);
static void app_iap_poll_line_byte(app_iap_source_t source, uint8_t ch, char *line, uint32_t *line_len);
static void app_iap_poll_binary_byte(uint8_t ch);
static void app_iap_handle_line(app_iap_source_t source, const char *line);
static void app_iap_print_status(app_iap_source_t source);
static void app_iap_print_help(app_iap_source_t source);
static bool app_iap_parse_recv_command(app_iap_source_t source,
                                       const char *line,
                                       uint32_t *size,
                                       uint32_t *crc32,
                                       uint32_t *version);
static bool app_iap_start_recv(app_iap_source_t source, uint32_t size, uint32_t crc32, uint32_t version);
static bool app_iap_flush_recv_buffer(void);
static void app_iap_finish_recv(void);
static void app_iap_abort_recv(const char *reason);
static bool app_iap_readback_crc(uint32_t address, uint32_t length, uint32_t *crc32);
static bool app_iap_read_staged_tag(app_tag_t *tag);
static bool app_iap_validate_staged_app_b_package(uint32_t package_size,
                                                  uint32_t command_version,
                                                  app_tag_t *tag);
static bool app_iap_read_sd_package_tag(FIL *file, app_tag_t *tag);
static void app_iap_run_demo_stage(app_iap_source_t source);
static void app_iap_run_sd_stage(void);
static bool app_iap_write_pending(const app_iap_pending_t *pending);
static bool app_iap_read_pending(app_iap_pending_t *pending);
static bool app_iap_read_boot_state(app_iap_boot_state_t *state);
static bool app_iap_write_boot_state(app_iap_boot_state_t *state);
static bool app_iap_w25q_erase_range(uint32_t address, uint32_t length);
static bool app_iap_staging_range_valid(uint32_t size);
static const char *app_iap_source_name(app_iap_source_t source);

void app_iap_init(bool flash_ready)
{
    app_iap_pending_t pending;
    app_iap_boot_state_t boot_state;

    memset(&s_iap_status, 0, sizeof(s_iap_status));
    s_iap_status.flash_ready = flash_ready;
    s_iap_status.serial_ready = true;
    s_iap_status.usb_ready = app_usb_device_init();
    s_iap_status.sd_ready = app_sd_is_mounted();
    app_iap_set_status("IAP idle");
    printf("IAP: USB CDC init %s\r\n", s_iap_status.usb_ready ? "OK" : "FAIL");
    memset(&s_recv, 0, sizeof(s_recv));
    s_recv.mode = APP_IAP_RECV_MODE_LINE;
    s_recv.source = APP_IAP_SOURCE_SERIAL;
    app_uart1_rx_start_it();

    if (flash_ready && app_iap_read_pending(&pending) && app_iap_pending_valid(&pending))
    {
        app_iap_set_pending_status(&pending, "AppB pending flag valid");
    }

    if (flash_ready && app_iap_read_boot_state(&boot_state))
    {
        app_iap_set_boot_state_status(&boot_state, true);
    }

    app_iap_print_sources();
}

bool app_iap_confirm_current_app(void)
{
    app_iap_boot_state_t state;

    if (!s_iap_status.flash_ready)
    {
        printf("IAP confirm: W25Q256 unavailable, skip\r\n");
        return false;
    }

    if (APP_BUILD_RUN_ADDR != APP_B_RUN_ADDR)
    {
        printf("IAP confirm: current App is not AppB, skip\r\n");
        return true;
    }

    if (!app_iap_read_boot_state(&state))
    {
        printf("IAP confirm: no valid boot state, skip\r\n");
        app_iap_set_boot_state_status(NULL, false);
        return false;
    }

    app_iap_set_boot_state_status(&state, true);

    if ((state.state == APP_IAP_BOOT_STATE_CONFIRMED) &&
        (state.active_slot == APP_SLOT_B))
    {
        printf("IAP confirm: AppB already confirmed version=%lu\r\n",
               (unsigned long)state.trial_version);
        return true;
    }

    if ((state.state != APP_IAP_BOOT_STATE_TRIAL) ||
        (state.trial_slot != APP_SLOT_B) ||
        (state.trial_version != APP_BUILD_VERSION))
    {
        printf("IAP confirm: state=%s trial=%lu version=%lu does not match AppB version=%lu\r\n",
               app_iap_boot_state_name(state.state),
               (unsigned long)state.trial_slot,
               (unsigned long)state.trial_version,
               (unsigned long)APP_BUILD_VERSION);
        return false;
    }

    state.state = APP_IAP_BOOT_STATE_CONFIRMED;
    state.active_slot = APP_SLOT_B;
    state.trial_slot = APP_SLOT_B;
    state.boot_attempts = 0U;
    state.max_boot_attempts = APP_IAP_BOOT_MAX_ATTEMPTS;
    state.last_error = APP_IAP_BOOT_ERROR_NONE;

    if (!app_iap_write_boot_state(&state))
    {
        printf("IAP confirm: write confirmed state failed\r\n");
        app_iap_set_status("confirm write fail");
        return false;
    }

    app_iap_set_boot_state_status(&state, true);
    app_iap_set_status("AppB confirmed");
    printf("IAP confirm: AppB version=%lu confirmed\r\n",
           (unsigned long)APP_BUILD_VERSION);
    app_log_event("AppB confirmed version=%lu", (unsigned long)APP_BUILD_VERSION);
    return true;
}

void app_iap_task(void *argument)
{
    (void)argument;

    printf("task_iap started\r\n");
    app_log_event("task_iap started");

    while (1)
    {
        app_iap_poll_serial();
        app_iap_poll_usb();

        if (s_demo_stage_requested)
        {
            app_iap_source_t source = s_demo_source;
            s_demo_stage_requested = false;
            app_iap_run_demo_stage(source);
        }

        if (s_sd_stage_requested)
        {
            s_sd_stage_requested = false;
            app_iap_run_sd_stage();
        }

        if (s_recv.mode == APP_IAP_RECV_MODE_BINARY)
        {
            vTaskDelay(pdMS_TO_TICKS(1U));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(20U));
        }
    }
}

void app_iap_get_status(app_iap_status_t *status)
{
    uint32_t primask;

    if (status == NULL)
    {
        return;
    }

    primask = app_critical_enter();
    *status = s_iap_status;
    app_critical_exit(primask);
}

void app_iap_request_demo_stage(app_iap_source_t source)
{
    s_demo_source = source;
    s_demo_stage_requested = true;
}

static void app_iap_set_status(const char *text)
{
    uint32_t primask = app_critical_enter();
    (void)strncpy(s_iap_status.status, text, sizeof(s_iap_status.status) - 1U);
    s_iap_status.status[sizeof(s_iap_status.status) - 1U] = '\0';
    app_critical_exit(primask);
}

static void app_iap_set_recv_progress(bool active, uint32_t received, uint32_t expected)
{
    uint32_t primask = app_critical_enter();
    s_iap_status.recv_active = active;
    s_iap_status.recv_received = received;
    s_iap_status.recv_expected = expected;
    app_critical_exit(primask);
}

static void app_iap_set_pending_status(const app_iap_pending_t *pending, const char *text)
{
    if (pending == NULL)
    {
        return;
    }

    uint32_t primask = app_critical_enter();
    s_iap_status.pending_valid = true;
    s_iap_status.app_b_version = pending->app_version;
    s_iap_status.app_b_size = pending->app_size;
    s_iap_status.app_b_crc32 = pending->app_crc32;
    app_critical_exit(primask);
    app_iap_set_status(text);
}

static void app_iap_clear_pending_status(void)
{
    uint32_t primask = app_critical_enter();
    s_iap_status.pending_valid = false;
    s_iap_status.app_b_version = 0U;
    s_iap_status.app_b_size = 0U;
    s_iap_status.app_b_crc32 = 0U;
    app_critical_exit(primask);
}

static void app_iap_set_boot_state_status(const app_iap_boot_state_t *state, bool valid)
{
    uint32_t primask = app_critical_enter();
    s_iap_status.boot_state_valid = valid;
    if ((state != NULL) && valid)
    {
        s_iap_status.boot_state = state->state;
        s_iap_status.boot_active_slot = state->active_slot;
        s_iap_status.boot_trial_slot = state->trial_slot;
        s_iap_status.boot_attempts = state->boot_attempts;
        s_iap_status.boot_max_attempts = state->max_boot_attempts;
        s_iap_status.boot_last_error = state->last_error;
    }
    else
    {
        s_iap_status.boot_state = APP_IAP_BOOT_STATE_NONE;
        s_iap_status.boot_active_slot = APP_SLOT_A;
        s_iap_status.boot_trial_slot = APP_SLOT_A;
        s_iap_status.boot_attempts = 0U;
        s_iap_status.boot_max_attempts = 0U;
        s_iap_status.boot_last_error = 0U;
    }
    app_critical_exit(primask);
}

static void app_iap_print_sources(void)
{
    printf("IAP: serial local source ready on USART1, commands: iap status / iap recv <size> <crc32> <version> / iap demo\r\n");
    if (s_iap_status.usb_ready)
    {
        printf("IAP: USB CDC local source ready, use the same iap recv protocol on the USB virtual COM port\r\n");
    }
    else
    {
        printf("IAP: USB CDC local source init failed, USB upgrade unavailable\r\n");
    }
    printf("IAP: SD card local source ready on demand, command: iap sd, file: %s\r\n",
           APP_IAP_SD_PACKAGE_PATH);
    printf("IAP: staging uses W25Q256 0x%08lX..0x%08lX, AppA flash is not erased by App\r\n",
           (unsigned long)APP_IAP_W25Q_BASE,
           (unsigned long)(APP_IAP_W25Q_END_ADDR - 1UL));
    printf("IAP: pending=0x%08lX state=0x%08lX staging=0x%08lX..0x%08lX\r\n",
           (unsigned long)APP_IAP_W25Q_PENDING_ADDR,
           (unsigned long)APP_IAP_W25Q_STATE_ADDR,
           (unsigned long)APP_IAP_W25Q_STAGING_ADDR,
           (unsigned long)(APP_IAP_W25Q_END_ADDR - 1UL));
    printf("IAP: valid slot package will be installed by Boot to AppB trial, AppA remains fallback\r\n");
}

static void app_iap_reply_source(app_iap_source_t source, const char *text)
{
    if ((source == APP_IAP_SOURCE_USB) && (text != NULL) && s_iap_status.usb_ready)
    {
        app_usb_cdc_write_text(text);
    }
}

static void app_iap_poll_serial(void)
{
    uint8_t ch;

    while (app_uart1_rx_read_byte(&ch))
    {
        app_iap_poll_stream_byte(APP_IAP_SOURCE_SERIAL, ch);
    }
}

static void app_iap_poll_usb(void)
{
    uint8_t ch;

    while (app_usb_cdc_rx_read_byte(&ch))
    {
        app_iap_poll_stream_byte(APP_IAP_SOURCE_USB, ch);
    }
}

static void app_iap_poll_stream_byte(app_iap_source_t source, uint8_t ch)
{
    if ((s_recv.mode == APP_IAP_RECV_MODE_BINARY) && (s_recv.source == source))
    {
        app_iap_poll_binary_byte(ch);
        return;
    }

    if (source == APP_IAP_SOURCE_USB)
    {
        app_iap_poll_line_byte(source, ch, s_usb_line, &s_usb_line_len);
    }
    else
    {
        app_iap_poll_line_byte(source, ch, s_serial_line, &s_serial_line_len);
    }
}

static void app_iap_poll_line_byte(app_iap_source_t source, uint8_t ch, char *line, uint32_t *line_len)
{
    if (ch == '\r')
    {
        return;
    }

    if (ch == '\n')
    {
        if (*line_len > 0U)
        {
            line[*line_len] = '\0';
            app_iap_handle_line(source, line);
            *line_len = 0U;
        }
    }
    else if (*line_len < (APP_IAP_SERIAL_LINE_LEN - 1U))
    {
        line[(*line_len)++] = (char)ch;
    }
    else
    {
        *line_len = 0U;
        printf("IAP %s: line too long, dropped\r\n", app_iap_source_name(source));
    }
}

static void app_iap_poll_binary_byte(uint8_t ch)
{
    s_recv.buffer[s_recv.buffer_len++] = ch;
    s_recv.received_size++;
    s_recv.crc32 = app_crc32_update(s_recv.crc32, &ch, 1U);

    if ((s_recv.received_size >= s_recv.next_progress) ||
        (s_recv.received_size == s_recv.expected_size))
    {
        app_iap_set_recv_progress(true, s_recv.received_size, s_recv.expected_size);
        printf("IAP %s: recv progress %lu/%lu\r\n",
               app_iap_source_name(s_recv.source),
               (unsigned long)s_recv.received_size,
               (unsigned long)s_recv.expected_size);
        s_recv.next_progress += APP_IAP_RECV_PROGRESS_STEP;
    }

    if ((s_recv.buffer_len >= sizeof(s_recv.buffer)) ||
        (s_recv.received_size == s_recv.expected_size))
    {
        if (!app_iap_flush_recv_buffer())
        {
            app_iap_abort_recv("staging write fail");
            return;
        }

        if (s_recv.received_size == s_recv.expected_size)
        {
            app_iap_finish_recv();
        }
    }
}

static void app_iap_handle_line(app_iap_source_t source, const char *line)
{
    uint32_t recv_size;
    uint32_t recv_crc32;
    uint32_t recv_version;

    if (strcmp(line, "iap status") == 0)
    {
        app_iap_print_status(source);
    }
    else if (strncmp(line, "iap recv ", 9U) == 0)
    {
        if (app_iap_parse_recv_command(source, line, &recv_size, &recv_crc32, &recv_version))
        {
            (void)app_iap_start_recv(source, recv_size, recv_crc32, recv_version);
        }
    }
    else if (strcmp(line, "iap demo") == 0)
    {
        printf("IAP %s: demo staging requested\r\n", app_iap_source_name(source));
        app_iap_request_demo_stage(source);
    }
    else if (strcmp(line, "iap demo usb") == 0)
    {
        printf("IAP USB: demo staging requested through reserved local source\r\n");
        app_iap_request_demo_stage(APP_IAP_SOURCE_USB);
    }
    else if (strcmp(line, "iap demo sd") == 0)
    {
        printf("IAP SD: demo staging requested through reserved local source\r\n");
        app_iap_request_demo_stage(APP_IAP_SOURCE_SD_CARD);
    }
    else if (strcmp(line, "iap sd") == 0)
    {
        app_iap_request_sd_stage();
    }
    else if (strcmp(line, "iap help") == 0)
    {
        app_iap_print_help(source);
    }
}

static void app_iap_print_status(app_iap_source_t source)
{
    app_iap_status_t status;
    char line[192];

    app_iap_get_status(&status);
    snprintf(line,
             sizeof(line),
             "IAP status: flash=%u pending=%u version=%lu size=%lu crc=0x%08lX recv=%u %lu/%lu state=%s\r\n",
             status.flash_ready ? 1U : 0U,
             status.pending_valid ? 1U : 0U,
             (unsigned long)status.app_b_version,
             (unsigned long)status.app_b_size,
             (unsigned long)status.app_b_crc32,
             status.recv_active ? 1U : 0U,
             (unsigned long)status.recv_received,
             (unsigned long)status.recv_expected,
             status.status);
    printf("%s", line);
    app_iap_reply_source(source, line);

    snprintf(line,
             sizeof(line),
             "IAP boot state: valid=%u state=%s active=%lu trial=%lu attempts=%lu/%lu err=%lu\r\n",
             status.boot_state_valid ? 1U : 0U,
             app_iap_boot_state_name(status.boot_state),
             (unsigned long)status.boot_active_slot,
             (unsigned long)status.boot_trial_slot,
             (unsigned long)status.boot_attempts,
             (unsigned long)status.boot_max_attempts,
             (unsigned long)status.boot_last_error);
    printf("%s", line);
    app_iap_reply_source(source, line);
}

static void app_iap_print_help(app_iap_source_t source)
{
    const char *help =
        "IAP commands:\r\n"
        "  iap status\r\n"
        "  iap recv <size> <crc32> <version>\r\n"
        "  iap sd\r\n"
        "  iap demo / iap demo usb / iap demo sd\r\n"
        "  iap help\r\n";

    printf("%s", help);
    app_iap_reply_source(source, help);
}

void app_iap_request_sd_stage(void)
{
    if (s_recv.mode != APP_IAP_RECV_MODE_LINE)
    {
        printf("IAP SD: busy, %s receive active\r\n", app_iap_source_name(s_recv.source));
        return;
    }

    printf("IAP SD: file staging requested: %s\r\n", APP_IAP_SD_PACKAGE_PATH);
    s_sd_stage_requested = true;
}

static bool app_iap_parse_recv_command(app_iap_source_t source,
                                       const char *line,
                                       uint32_t *size,
                                       uint32_t *crc32,
                                       uint32_t *version)
{
    unsigned long parsed_size;
    unsigned long parsed_crc32;
    unsigned long parsed_version;
    char extra;

    if ((line == NULL) || (size == NULL) || (crc32 == NULL) || (version == NULL))
    {
        return false;
    }

    if (strncmp(line, "iap recv ", 9U) != 0)
    {
        return false;
    }

    if (sscanf(line + 9U, "%lu %lx %lu %c", &parsed_size, &parsed_crc32, &parsed_version, &extra) != 3)
    {
        printf("IAP %s: bad recv command, use: iap recv <size> <crc32> <version>\r\n",
               app_iap_source_name(source));
        app_iap_reply_source(source, "IAP USB: bad recv command, use: iap recv <size> <crc32> <version>\r\n");
        return false;
    }

    *size = (uint32_t)parsed_size;
    *crc32 = (uint32_t)parsed_crc32;
    *version = (uint32_t)parsed_version;
    return true;
}

static bool app_iap_start_recv(app_iap_source_t source, uint32_t size, uint32_t crc32, uint32_t version)
{
    char line[128];

    if (s_recv.mode != APP_IAP_RECV_MODE_LINE)
    {
        printf("IAP %s: busy, %s receive active\r\n",
               app_iap_source_name(source),
               app_iap_source_name(s_recv.source));
        snprintf(line,
                 sizeof(line),
                 "IAP USB: busy, %s receive active\r\n",
                 app_iap_source_name(s_recv.source));
        app_iap_reply_source(source, line);
        return false;
    }

    if (!s_iap_status.flash_ready)
    {
        printf("IAP %s: W25Q256 unavailable, recv rejected\r\n", app_iap_source_name(source));
        app_iap_reply_source(source, "IAP USB: W25Q256 unavailable, recv rejected\r\n");
        app_iap_set_status("flash unavailable");
        return false;
    }

    if ((size == 0U) || !app_iap_staging_range_valid(size))
    {
        printf("IAP %s: invalid package size=%lu max=%lu\r\n",
               app_iap_source_name(source),
               (unsigned long)size,
               (unsigned long)APP_IAP_RECV_MAX_SIZE);
        snprintf(line,
                 sizeof(line),
                 "IAP USB: invalid package size=%lu max=%lu\r\n",
                 (unsigned long)size,
                 (unsigned long)APP_IAP_RECV_MAX_SIZE);
        app_iap_reply_source(source, line);
        app_iap_set_status("recv size invalid");
        return false;
    }

    if (version == APP_TAG_VERSION_NONE)
    {
        printf("IAP %s: invalid version=0\r\n", app_iap_source_name(source));
        app_iap_reply_source(source, "IAP USB: invalid version=0\r\n");
        app_iap_set_status("recv version invalid");
        return false;
    }

    printf("IAP %s: recv begin size=%lu crc=0x%08lX version=%lu\r\n",
           app_iap_source_name(source),
           (unsigned long)size,
           (unsigned long)crc32,
           (unsigned long)version);
    app_iap_set_status("recv clear pending");

    if (!app_iap_w25q_erase_range(APP_IAP_W25Q_PENDING_ADDR, sizeof(app_iap_pending_t)))
    {
        printf("IAP %s: clear old pending flag failed\r\n", app_iap_source_name(source));
        app_iap_reply_source(source, "IAP USB: clear old pending flag failed\r\n");
        app_iap_set_status("pending clear fail");
        return false;
    }
    app_iap_clear_pending_status();

    app_iap_set_status("recv erase staging");

    if (!app_iap_w25q_erase_range(APP_IAP_W25Q_STAGING_ADDR, size))
    {
        printf("IAP %s: erase staging failed\r\n", app_iap_source_name(source));
        app_iap_reply_source(source, "IAP USB: erase staging failed\r\n");
        app_iap_set_status("staging erase fail");
        return false;
    }

    memset(&s_recv, 0, sizeof(s_recv));
    s_recv.mode = APP_IAP_RECV_MODE_BINARY;
    s_recv.source = source;
    s_recv.expected_size = size;
    s_recv.expected_crc32 = crc32;
    s_recv.version = version;
    s_recv.write_addr = APP_IAP_W25Q_STAGING_ADDR;
    s_recv.next_progress = APP_IAP_RECV_PROGRESS_STEP;
    app_iap_set_recv_progress(true, 0U, size);
    app_iap_set_status("recv binary");

    printf("IAP %s: ready for binary size=%lu\r\n", app_iap_source_name(source), (unsigned long)size);
    snprintf(line, sizeof(line), "IAP USB: ready for binary size=%lu\r\n", (unsigned long)size);
    app_iap_reply_source(source, line);
    return true;
}

static bool app_iap_flush_recv_buffer(void)
{
    if (s_recv.buffer_len == 0U)
    {
        return true;
    }

    if (!app_w25q256_write_erased(s_recv.write_addr,
                                  s_recv.buffer,
                                  s_recv.buffer_len))
    {
        return false;
    }

    s_recv.write_addr += s_recv.buffer_len;
    s_recv.buffer_len = 0;
    return true;
}

static void app_iap_finish_recv(void)
{
    app_iap_pending_t pending;
    app_tag_t staged_tag;
    uint32_t readback_crc = 0U;
    app_iap_source_t source = s_recv.source;

    printf("IAP %s: recv done size=%lu crc=0x%08lX\r\n",
           app_iap_source_name(source),
           (unsigned long)s_recv.received_size,
           (unsigned long)s_recv.crc32);
    app_iap_set_status("recv verify");

    if (s_recv.crc32 != s_recv.expected_crc32)
    {
        printf("IAP %s: stream crc mismatch calc=0x%08lX expect=0x%08lX\r\n",
               app_iap_source_name(source),
               (unsigned long)s_recv.crc32,
               (unsigned long)s_recv.expected_crc32);
        app_iap_abort_recv("stream crc fail");
        return;
    }

    if (!app_iap_readback_crc(APP_IAP_W25Q_STAGING_ADDR, s_recv.expected_size, &readback_crc))
    {
        printf("IAP %s: readback failed\r\n", app_iap_source_name(source));
        app_iap_abort_recv("readback fail");
        return;
    }

    if (readback_crc != s_recv.expected_crc32)
    {
        printf("IAP %s: readback crc mismatch calc=0x%08lX expect=0x%08lX\r\n",
               app_iap_source_name(source),
               (unsigned long)readback_crc,
               (unsigned long)s_recv.expected_crc32);
        app_iap_abort_recv("readback crc fail");
        return;
    }

    if (!app_iap_validate_staged_app_b_package(s_recv.expected_size,
                                               s_recv.version,
                                               &staged_tag))
    {
        app_iap_abort_recv("AppB package invalid");
        return;
    }

    memset(&pending, 0, sizeof(pending));
    pending.magic = APP_IAP_PENDING_MAGIC;
    pending.version = APP_IAP_PENDING_VERSION;
    pending.length = sizeof(pending);
    pending.target_slot = APP_SLOT_B;
    pending.app_size = s_recv.expected_size;
    pending.app_version = staged_tag.version;
    pending.app_crc32 = s_recv.expected_crc32;
    pending.staging_addr = APP_IAP_W25Q_STAGING_ADDR;
    pending.header_crc32 = app_iap_pending_crc(&pending);

    if (!app_iap_write_pending(&pending))
    {
        printf("IAP %s: write pending flag failed\r\n", app_iap_source_name(source));
        app_iap_abort_recv("pending write fail");
        return;
    }

    s_recv.mode = APP_IAP_RECV_MODE_LINE;
    app_iap_set_recv_progress(false, pending.app_size, pending.app_size);
    app_iap_set_pending_status(&pending,
                               source == APP_IAP_SOURCE_USB ? "USB staged, pending set" :
                               source == APP_IAP_SOURCE_SD_CARD ? "SD staged, pending set" :
                               "serial staged, pending set");

    printf("IAP %s: package verified, pending flag set version=%lu size=%lu crc=0x%08lX\r\n",
           app_iap_source_name(source),
           (unsigned long)pending.app_version,
           (unsigned long)pending.app_size,
           (unsigned long)pending.app_crc32);
    {
        char line[160];
        snprintf(line,
                 sizeof(line),
                 "IAP USB: package verified, pending flag set version=%lu size=%lu crc=0x%08lX\r\n",
                 (unsigned long)pending.app_version,
                 (unsigned long)pending.app_size,
                 (unsigned long)pending.app_crc32);
        app_iap_reply_source(source, line);
    }
    printf("IAP: reset board to let Boot install package to AppB; AppA remains fallback\r\n");
    app_log_event("IAP %s package staged version=%lu size=%lu",
                  app_iap_source_name(source),
                  (unsigned long)pending.app_version,
                  (unsigned long)pending.app_size);
}

static void app_iap_abort_recv(const char *reason)
{
    app_iap_source_t source = s_recv.source;
    const char *abort_reason = (reason != NULL) ? reason : "unknown";
    char line[96];

    printf("IAP %s: abort, %s\r\n", app_iap_source_name(source), abort_reason);
    snprintf(line, sizeof(line), "IAP USB: abort, %s\r\n", abort_reason);
    app_iap_reply_source(source, line);
    memset(&s_recv, 0, sizeof(s_recv));
    s_recv.mode = APP_IAP_RECV_MODE_LINE;
    s_recv.source = APP_IAP_SOURCE_SERIAL;
    s_serial_line_len = 0;
    s_usb_line_len = 0;
    app_iap_set_recv_progress(false, 0U, 0U);
    app_iap_set_status(abort_reason);
}

static bool app_iap_readback_crc(uint32_t address, uint32_t length, uint32_t *crc32)
{
    uint32_t crc = 0U;
    uint32_t remaining = length;

    if (crc32 == NULL)
    {
        return false;
    }

    while (remaining > 0U)
    {
        uint32_t chunk = (remaining > sizeof(s_verify_buffer)) ? sizeof(s_verify_buffer) : remaining;

        if (!app_w25q256_read(address, s_verify_buffer, chunk))
        {
            return false;
        }

        crc = app_crc32_update(crc, s_verify_buffer, chunk);
        address += chunk;
        remaining -= chunk;
    }

    *crc32 = crc;
    return true;
}

static bool app_iap_read_staged_tag(app_tag_t *tag)
{
    if (tag == NULL)
    {
        return false;
    }

    return app_w25q256_read(APP_IAP_W25Q_STAGING_ADDR + APP_SLOT_HEADER_SIZE - APP_TAG_SIZE_BYTES,
                            tag,
                            sizeof(*tag));
}

static bool app_iap_validate_staged_app_b_package(uint32_t package_size,
                                                  uint32_t command_version,
                                                  app_tag_t *tag)
{
    const app_slot_info_t *slot = app_get_slot_info(APP_SLOT_B);
    uint32_t app_body_crc = 0U;

    if ((tag == NULL) || (slot == NULL) || (package_size < APP_SLOT_HEADER_SIZE))
    {
        printf("IAP: invalid package header size=%lu\r\n", (unsigned long)package_size);
        return false;
    }

    if (!app_iap_read_staged_tag(tag))
    {
        printf("IAP: read staged app_tag_t failed\r\n");
        return false;
    }

    printf("IAP: staged tag magic=0x%08lX run=0x%08lX app_size=%lu version=%lu app_crc=0x%08lX\r\n",
           (unsigned long)tag->magic,
           (unsigned long)tag->app_run_addr,
           (unsigned long)tag->app_size,
           (unsigned long)tag->version,
           (unsigned long)tag->crc32);

    if (!app_tag_basic_valid(tag, slot))
    {
        printf("IAP: staged package is not a valid AppB slot image\r\n");
        return false;
    }

    if (package_size != (APP_SLOT_HEADER_SIZE + tag->app_size))
    {
        printf("IAP: package size mismatch total=%lu expected=%lu\r\n",
               (unsigned long)package_size,
               (unsigned long)(APP_SLOT_HEADER_SIZE + tag->app_size));
        return false;
    }

    if (command_version != tag->version)
    {
        printf("IAP: command version=%lu does not match tag version=%lu\r\n",
               (unsigned long)command_version,
               (unsigned long)tag->version);
        return false;
    }

    if (!app_iap_readback_crc(APP_IAP_W25Q_STAGING_ADDR + APP_SLOT_HEADER_SIZE,
                              tag->app_size,
                              &app_body_crc))
    {
        printf("IAP: staged app body crc readback failed\r\n");
        return false;
    }

    if (app_body_crc != tag->crc32)
    {
        printf("IAP: staged app body crc mismatch calc=0x%08lX expect=0x%08lX\r\n",
               (unsigned long)app_body_crc,
               (unsigned long)tag->crc32);
        return false;
    }

    return true;
}

static bool app_iap_read_sd_package_tag(FIL *file, app_tag_t *tag)
{
    FRESULT fres;
    UINT bytes_read = 0U;

    if ((file == NULL) || (tag == NULL))
    {
        return false;
    }

    if (f_size(file) < APP_SLOT_HEADER_SIZE)
    {
        printf("IAP SD: file too small for App slot header, size=%lu\r\n",
               (unsigned long)f_size(file));
        return false;
    }

    fres = f_lseek(file, APP_SLOT_HEADER_SIZE - APP_TAG_SIZE_BYTES);
    if (fres != FR_OK)
    {
        printf("IAP SD: seek tag failed, fresult=%u\r\n", (unsigned int)fres);
        return false;
    }

    fres = f_read(file, tag, sizeof(*tag), &bytes_read);
    if ((fres != FR_OK) || (bytes_read != sizeof(*tag)))
    {
        printf("IAP SD: read tag failed fresult=%u bytes=%u\r\n",
               (unsigned int)fres,
               (unsigned int)bytes_read);
        return false;
    }

    fres = f_lseek(file, 0U);
    if (fres != FR_OK)
    {
        printf("IAP SD: rewind failed, fresult=%u\r\n", (unsigned int)fres);
        return false;
    }

    return true;
}

static void app_iap_run_demo_stage(app_iap_source_t source)
{
    app_iap_pending_t pending;
    uint32_t crc;

    printf("IAP %s: staging demo data to AppB staging area for Boot reject-path test\r\n",
           app_iap_source_name(source));

    if (!s_iap_status.flash_ready)
    {
        printf("IAP %s: W25Q256 unavailable, staging skipped\r\n", app_iap_source_name(source));
        app_iap_set_status("flash unavailable");
        return;
    }

    for (uint32_t i = 0; i < sizeof(s_demo_image); ++i)
    {
        s_demo_image[i] = (uint8_t)((i * 37U) + 11U);
    }
    crc = app_crc32(s_demo_image, sizeof(s_demo_image));

    if (!app_iap_w25q_erase_range(APP_IAP_W25Q_STAGING_ADDR, sizeof(s_demo_image)))
    {
        printf("IAP %s: erase staging failed\r\n", app_iap_source_name(source));
        app_iap_set_status("staging erase fail");
        return;
    }

    if (!app_w25q256_write_erased(APP_IAP_W25Q_STAGING_ADDR, s_demo_image, sizeof(s_demo_image)))
    {
        printf("IAP %s: write staging failed\r\n", app_iap_source_name(source));
        app_iap_set_status("staging write fail");
        return;
    }

    memset(s_demo_image, 0, sizeof(s_demo_image));
    if (!app_w25q256_read(APP_IAP_W25Q_STAGING_ADDR, s_demo_image, sizeof(s_demo_image)) ||
        (app_crc32(s_demo_image, sizeof(s_demo_image)) != crc))
    {
        printf("IAP %s: readback crc failed\r\n", app_iap_source_name(source));
        app_iap_set_status("staging crc fail");
        return;
    }

    memset(&pending, 0, sizeof(pending));
    pending.magic = APP_IAP_PENDING_MAGIC;
    pending.version = APP_IAP_PENDING_VERSION;
    pending.length = sizeof(pending);
    pending.target_slot = APP_SLOT_B;
    pending.app_size = sizeof(s_demo_image);
    pending.app_version = 2U;
    pending.app_crc32 = crc;
    pending.staging_addr = APP_IAP_W25Q_STAGING_ADDR;
    pending.header_crc32 = app_iap_pending_crc(&pending);

    if (!app_iap_write_pending(&pending))
    {
        printf("IAP %s: write pending flag failed\r\n", app_iap_source_name(source));
        app_iap_set_status("pending write fail");
        return;
    }

    app_iap_set_pending_status(&pending, "demo staged, pending set");

    printf("IAP %s: demo package verified, pending flag set version=%lu size=%lu crc=0x%08lX\r\n",
           app_iap_source_name(source),
           (unsigned long)pending.app_version,
           (unsigned long)pending.app_size,
           (unsigned long)pending.app_crc32);
    printf("IAP: reset board to let Boot inspect pending flag; demo data is not a valid AppB package\r\n");
    app_log_event("IAP demo staged from %s", app_iap_source_name(source));
}

static void app_iap_run_sd_stage(void)
{
    FIL file;
    FRESULT fres;
    FSIZE_t file_size;
    UINT bytes_read;
    app_tag_t sd_tag;
    const app_slot_info_t *app_b_slot = app_get_slot_info(APP_SLOT_B);

    if (!app_sd_mount())
    {
        uint32_t primask = app_critical_enter();
        s_iap_status.sd_ready = false;
        app_critical_exit(primask);
        app_iap_set_status("SD mount fail");
        return;
    }

    uint32_t primask = app_critical_enter();
    s_iap_status.sd_ready = true;
    app_critical_exit(primask);

    fres = f_open(&file, APP_IAP_SD_PACKAGE_PATH, FA_READ);
    if (fres != FR_OK)
    {
        printf("IAP SD: open %s failed, fresult=%u\r\n",
               APP_IAP_SD_PACKAGE_PATH,
               (unsigned int)fres);
        app_iap_set_status("SD file open fail");
        return;
    }

    file_size = f_size(&file);
    if ((file_size == 0U) || (file_size > APP_IAP_RECV_MAX_SIZE))
    {
        printf("IAP SD: invalid file size=%lu max=%lu\r\n",
               (unsigned long)file_size,
               (unsigned long)APP_IAP_RECV_MAX_SIZE);
        (void)f_close(&file);
        app_iap_set_status("SD file size invalid");
        return;
    }

    if (!app_iap_read_sd_package_tag(&file, &sd_tag))
    {
        (void)f_close(&file);
        app_iap_set_status("SD tag read fail");
        return;
    }

    printf("IAP SD: file tag magic=0x%08lX run=0x%08lX app_size=%lu version=%lu app_crc=0x%08lX\r\n",
           (unsigned long)sd_tag.magic,
           (unsigned long)sd_tag.app_run_addr,
           (unsigned long)sd_tag.app_size,
           (unsigned long)sd_tag.version,
           (unsigned long)sd_tag.crc32);

    if (!app_tag_basic_valid(&sd_tag, app_b_slot) ||
        (file_size != (FSIZE_t)(APP_SLOT_HEADER_SIZE + sd_tag.app_size)))
    {
        printf("IAP SD: package is not a valid AppB slot image, file_size=%lu expected=%lu\r\n",
               (unsigned long)file_size,
               (unsigned long)(APP_SLOT_HEADER_SIZE + sd_tag.app_size));
        (void)f_close(&file);
        app_iap_set_status("SD package invalid");
        return;
    }

    if (!app_iap_start_recv(APP_IAP_SOURCE_SD_CARD, (uint32_t)file_size, 0U, sd_tag.version))
    {
        (void)f_close(&file);
        return;
    }

    while (s_recv.received_size < s_recv.expected_size)
    {
        uint32_t remaining = s_recv.expected_size - s_recv.received_size;
        uint32_t to_read = remaining > sizeof(s_recv.buffer) ? sizeof(s_recv.buffer) : remaining;

        fres = f_read(&file, s_recv.buffer, to_read, &bytes_read);
        if ((fres != FR_OK) || (bytes_read == 0U))
        {
            printf("IAP SD: read failed fresult=%u bytes=%u\r\n",
                   (unsigned int)fres,
                   (unsigned int)bytes_read);
            (void)f_close(&file);
            app_iap_abort_recv("SD read fail");
            return;
        }

        s_recv.buffer_len = bytes_read;
        s_recv.received_size += bytes_read;
        s_recv.crc32 = app_crc32_update(s_recv.crc32, s_recv.buffer, bytes_read);

        if (!app_iap_flush_recv_buffer())
        {
            (void)f_close(&file);
            app_iap_abort_recv("staging write fail");
            return;
        }

        if ((s_recv.received_size >= s_recv.next_progress) ||
            (s_recv.received_size == s_recv.expected_size))
        {
            app_iap_set_recv_progress(true, s_recv.received_size, s_recv.expected_size);
            printf("IAP SD: file progress %lu/%lu\r\n",
                   (unsigned long)s_recv.received_size,
                   (unsigned long)s_recv.expected_size);
            s_recv.next_progress += APP_IAP_RECV_PROGRESS_STEP;
        }
    }

    (void)f_close(&file);
    s_recv.expected_crc32 = s_recv.crc32;

    app_iap_finish_recv();
}

static bool app_iap_write_pending(const app_iap_pending_t *pending)
{
    if (!app_iap_w25q_erase_range(APP_IAP_W25Q_PENDING_ADDR, sizeof(*pending)))
    {
        return false;
    }

    return app_w25q256_write_erased(APP_IAP_W25Q_PENDING_ADDR, pending, sizeof(*pending));
}

static bool app_iap_read_pending(app_iap_pending_t *pending)
{
    if (pending == NULL)
    {
        return false;
    }

    return app_w25q256_read(APP_IAP_W25Q_PENDING_ADDR, pending, sizeof(*pending));
}

static bool app_iap_read_boot_state(app_iap_boot_state_t *state)
{
    if (state == NULL)
    {
        return false;
    }

    if (!app_w25q256_read(APP_IAP_W25Q_STATE_ADDR, state, sizeof(*state)))
    {
        return false;
    }

    return app_iap_boot_state_valid(state);
}

static bool app_iap_write_boot_state(app_iap_boot_state_t *state)
{
    if (state == NULL)
    {
        return false;
    }

    state->magic = APP_IAP_BOOT_STATE_MAGIC;
    state->version = APP_IAP_BOOT_STATE_VERSION;
    state->length = sizeof(*state);
    state->record_crc32 = app_iap_boot_state_crc(state);

    if (!app_iap_boot_state_valid(state))
    {
        return false;
    }

    if (!app_iap_w25q_erase_range(APP_IAP_W25Q_STATE_ADDR, sizeof(*state)))
    {
        return false;
    }

    return app_w25q256_write_erased(APP_IAP_W25Q_STATE_ADDR, state, sizeof(*state));
}

static bool app_iap_w25q_erase_range(uint32_t address, uint32_t length)
{
    uint32_t end = address + length;

    if ((length == 0U) || (end < address))
    {
        return false;
    }

    address &= ~(APP_W25Q256_SECTOR_SIZE - 1UL);
    while (address < end)
    {
        if (!app_w25q256_erase_sector(address))
        {
            return false;
        }
        address += APP_W25Q256_SECTOR_SIZE;
    }

    return true;
}

static bool app_iap_staging_range_valid(uint32_t size)
{
    uint32_t end = APP_IAP_W25Q_STAGING_ADDR + size;

    if ((size == 0U) || (size > APP_IAP_RECV_MAX_SIZE) ||
        (end < APP_IAP_W25Q_STAGING_ADDR))
    {
        return false;
    }

    return (end <= APP_IAP_W25Q_END_ADDR);
}

static const char *app_iap_source_name(app_iap_source_t source)
{
    switch (source)
    {
    case APP_IAP_SOURCE_SERIAL:
        return "serial";
    case APP_IAP_SOURCE_USB:
        return "USB";
    case APP_IAP_SOURCE_SD_CARD:
        return "SD";
    default:
        return "unknown";
    }
}
