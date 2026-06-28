#include "app_ui_hmi.h"

#include <stdio.h>

#include "app_diag.h"
#include "app_log.h"
#include "app_iap.h"
#include "app_iap_record.h"
#include "app_settings.h"
#include "app_storage.h"
#include "app_system_model.h"
#include "app_tasks.h"
#include "lvgl.h"
#include "system_clock.h"

typedef enum
{
    UI_PAGE_HOME = 0,
    UI_PAGE_PARAMS,
    UI_PAGE_UPGRADE,
    UI_PAGE_LOG,
    UI_PAGE_COUNT,
} ui_page_t;

static lv_obj_t *s_pages[UI_PAGE_COUNT];
static lv_obj_t *s_nav_btns[UI_PAGE_COUNT];
static lv_obj_t *s_home_status;
static lv_obj_t *s_home_values;
static lv_obj_t *s_param_status;
static lv_obj_t *s_brightness_slider;
static lv_obj_t *s_threshold_slider;
static lv_obj_t *s_output_button_label;
static lv_obj_t *s_upgrade_status;
static lv_obj_t *s_upgrade_bar;
static lv_obj_t *s_log_labels[APP_LOG_LINE_COUNT];
static ui_page_t s_active_page;
static bool s_upgrade_running;
static uint32_t s_upgrade_progress;
static uint32_t s_last_home_update;
static uint32_t s_last_log_update;
static uint32_t s_last_upgrade_update;

static void ui_create_nav(lv_obj_t *screen);
static void ui_create_home(lv_obj_t *screen);
static void ui_create_params(lv_obj_t *screen);
static void ui_create_upgrade(lv_obj_t *screen);
static void ui_create_log(lv_obj_t *screen);
static void ui_show_page(ui_page_t page);
static lv_obj_t *ui_make_button(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h);
static lv_obj_t *ui_make_label(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y);
static void ui_nav_event(lv_event_t *event);
static void ui_brightness_event(lv_event_t *event);
static void ui_threshold_event(lv_event_t *event);
static void ui_output_event(lv_event_t *event);
static void ui_save_event(lv_event_t *event);
static void ui_upgrade_event(lv_event_t *event);
static void ui_sd_upgrade_event(lv_event_t *event);
static void ui_update_params(void);
static void ui_update_home(uint32_t now_ms);
static void ui_update_upgrade(uint32_t now_ms);
static void ui_update_log(void);

void app_ui_hmi_create(void)
{
    lv_obj_t *screen = lv_scr_act();

    lv_obj_set_style_bg_color(screen, lv_color_hex(0xF2F4F7), LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_nav(screen);
    ui_create_home(screen);
    ui_create_params(screen);
    ui_create_upgrade(screen);
    ui_create_log(screen);
    ui_show_page(UI_PAGE_HOME);
    ui_update_params();
    ui_update_log();
}

void app_ui_hmi_tick(uint32_t now_ms)
{
    if ((now_ms - s_last_home_update) >= 500U)
    {
        ui_update_home(now_ms);
        ui_update_params();
        s_last_home_update = now_ms;
    }

    if ((now_ms - s_last_upgrade_update) >= 200U)
    {
        ui_update_upgrade(now_ms);
        s_last_upgrade_update = now_ms;
    }

    if ((now_ms - s_last_log_update) >= 1000U)
    {
        ui_update_log();
        s_last_log_update = now_ms;
    }
}

static void ui_create_nav(lv_obj_t *screen)
{
    static const char *names[UI_PAGE_COUNT] = {"Home", "Params", "Upgrade", "Log"};

    for (uint32_t i = 0; i < UI_PAGE_COUNT; ++i)
    {
        lv_obj_t *btn = ui_make_button(screen, names[i], (lv_coord_t)(20 + (int32_t)i * 190), 10, 170, 56);
        lv_obj_add_event_cb(btn, ui_nav_event, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        s_nav_btns[i] = btn;
    }
}

static void ui_create_home(lv_obj_t *screen)
{
    s_pages[UI_PAGE_HOME] = lv_obj_create(screen);
    lv_obj_set_size(s_pages[UI_PAGE_HOME], 760, 392);
    lv_obj_align(s_pages[UI_PAGE_HOME], LV_ALIGN_TOP_LEFT, 20, 72);
    lv_obj_clear_flag(s_pages[UI_PAGE_HOME], LV_OBJ_FLAG_SCROLLABLE);

    (void)ui_make_label(s_pages[UI_PAGE_HOME], "Device Overview", 24, 18);
    s_home_status = ui_make_label(s_pages[UI_PAGE_HOME], "", 24, 58);
    lv_obj_set_width(s_home_status, 700);
    lv_label_set_long_mode(s_home_status, LV_LABEL_LONG_WRAP);
    s_home_values = ui_make_label(s_pages[UI_PAGE_HOME], "", 24, 154);
    lv_obj_set_width(s_home_values, 700);
    lv_label_set_long_mode(s_home_values, LV_LABEL_LONG_WRAP);
}

static void ui_create_params(lv_obj_t *screen)
{
    lv_obj_t *label;
    lv_obj_t *btn;

    s_pages[UI_PAGE_PARAMS] = lv_obj_create(screen);
    lv_obj_set_size(s_pages[UI_PAGE_PARAMS], 760, 392);
    lv_obj_align(s_pages[UI_PAGE_PARAMS], LV_ALIGN_TOP_LEFT, 20, 72);
    lv_obj_clear_flag(s_pages[UI_PAGE_PARAMS], LV_OBJ_FLAG_SCROLLABLE);

    (void)ui_make_label(s_pages[UI_PAGE_PARAMS], "Parameter Setup", 24, 18);

    label = ui_make_label(s_pages[UI_PAGE_PARAMS], "Brightness", 24, 74);
    lv_obj_set_width(label, 160);
    s_brightness_slider = lv_slider_create(s_pages[UI_PAGE_PARAMS]);
    lv_obj_set_size(s_brightness_slider, 360, 28);
    lv_obj_align(s_brightness_slider, LV_ALIGN_TOP_LEFT, 190, 72);
    lv_slider_set_range(s_brightness_slider, 0, 100);
    lv_obj_add_event_cb(s_brightness_slider, ui_brightness_event, LV_EVENT_VALUE_CHANGED, NULL);

    label = ui_make_label(s_pages[UI_PAGE_PARAMS], "Threshold", 24, 132);
    lv_obj_set_width(label, 160);
    s_threshold_slider = lv_slider_create(s_pages[UI_PAGE_PARAMS]);
    lv_obj_set_size(s_threshold_slider, 360, 28);
    lv_obj_align(s_threshold_slider, LV_ALIGN_TOP_LEFT, 190, 130);
    lv_slider_set_range(s_threshold_slider, 0, 100);
    lv_obj_add_event_cb(s_threshold_slider, ui_threshold_event, LV_EVENT_VALUE_CHANGED, NULL);

    btn = ui_make_button(s_pages[UI_PAGE_PARAMS], "", 190, 190, 160, 48);
    lv_obj_add_event_cb(btn, ui_output_event, LV_EVENT_CLICKED, NULL);
    s_output_button_label = lv_obj_get_child(btn, 0);

    btn = ui_make_button(s_pages[UI_PAGE_PARAMS], "Save", 390, 190, 140, 48);
    lv_obj_add_event_cb(btn, ui_save_event, LV_EVENT_CLICKED, NULL);

    s_param_status = ui_make_label(s_pages[UI_PAGE_PARAMS], "", 24, 270);
}

static void ui_create_upgrade(lv_obj_t *screen)
{
    lv_obj_t *btn;

    s_pages[UI_PAGE_UPGRADE] = lv_obj_create(screen);
    lv_obj_set_size(s_pages[UI_PAGE_UPGRADE], 760, 392);
    lv_obj_align(s_pages[UI_PAGE_UPGRADE], LV_ALIGN_TOP_LEFT, 20, 72);
    lv_obj_clear_flag(s_pages[UI_PAGE_UPGRADE], LV_OBJ_FLAG_SCROLLABLE);

    (void)ui_make_label(s_pages[UI_PAGE_UPGRADE], "Upgrade Panel", 24, 18);
    (void)ui_make_label(s_pages[UI_PAGE_UPGRADE], "Current version: App 0.7.0", 24, 64);
    (void)ui_make_label(s_pages[UI_PAGE_UPGRADE], "New version: App 0.8.0 demo", 24, 100);

    s_upgrade_bar = lv_bar_create(s_pages[UI_PAGE_UPGRADE]);
    lv_obj_set_size(s_upgrade_bar, 520, 28);
    lv_obj_align(s_upgrade_bar, LV_ALIGN_TOP_LEFT, 24, 154);
    lv_bar_set_range(s_upgrade_bar, 0, 100);
    lv_bar_set_value(s_upgrade_bar, 0, LV_ANIM_OFF);

    btn = ui_make_button(s_pages[UI_PAGE_UPGRADE], "Start Demo", 24, 214, 160, 50);
    lv_obj_add_event_cb(btn, ui_upgrade_event, LV_EVENT_CLICKED, NULL);

    btn = ui_make_button(s_pages[UI_PAGE_UPGRADE], "SD File", 204, 214, 160, 50);
    lv_obj_add_event_cb(btn, ui_sd_upgrade_event, LV_EVENT_CLICKED, NULL);

    s_upgrade_status = ui_make_label(s_pages[UI_PAGE_UPGRADE], "Idle", 24, 294);
}

static void ui_create_log(lv_obj_t *screen)
{
    s_pages[UI_PAGE_LOG] = lv_obj_create(screen);
    lv_obj_set_size(s_pages[UI_PAGE_LOG], 760, 392);
    lv_obj_align(s_pages[UI_PAGE_LOG], LV_ALIGN_TOP_LEFT, 20, 72);
    lv_obj_clear_flag(s_pages[UI_PAGE_LOG], LV_OBJ_FLAG_SCROLLABLE);

    (void)ui_make_label(s_pages[UI_PAGE_LOG], "Recent Events", 24, 18);

    for (uint32_t i = 0; i < APP_LOG_LINE_COUNT; ++i)
    {
        s_log_labels[i] = ui_make_label(s_pages[UI_PAGE_LOG], "", 24, (lv_coord_t)(58 + i * 36));
        lv_obj_set_width(s_log_labels[i], 700);
    }
}

static void ui_show_page(ui_page_t page)
{
    s_active_page = page;
    for (uint32_t i = 0; i < UI_PAGE_COUNT; ++i)
    {
        if (i == (uint32_t)page)
        {
            lv_obj_clear_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(s_nav_btns[i], lv_color_hex(0x2F80ED), LV_PART_MAIN);
        }
        else
        {
            lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(s_nav_btns[i], lv_color_hex(0x5F6C7B), LV_PART_MAIN);
        }
    }
}

static lv_obj_t *ui_make_button(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_t *label;

    lv_obj_set_size(btn, w, h);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);

    label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

static lv_obj_t *ui_make_label(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, x, y);
    return label;
}

static void ui_nav_event(lv_event_t *event)
{
    ui_page_t page = (ui_page_t)(uintptr_t)lv_event_get_user_data(event);
    ui_show_page(page);
    printf("UI page=%lu\r\n", (unsigned long)page);
}

static void ui_brightness_event(lv_event_t *event)
{
    (void)event;
    if (app_settings_set_brightness((uint32_t)lv_slider_get_value(s_brightness_slider)))
    {
        printf("UI brightness=%ld delayed_save=%ums\r\n",
               (long)lv_slider_get_value(s_brightness_slider),
               (unsigned int)APP_WORKSET_SAVE_DELAY_MS);
        app_log_event("brightness changed");
    }
    ui_update_params();
}

static void ui_threshold_event(lv_event_t *event)
{
    (void)event;
    if (app_settings_set_threshold((int32_t)lv_slider_get_value(s_threshold_slider)))
    {
        printf("UI threshold=%ld delayed_save=%ums\r\n",
               (long)lv_slider_get_value(s_threshold_slider),
               (unsigned int)APP_WORKSET_SAVE_DELAY_MS);
        app_log_event("threshold changed");
    }
    ui_update_params();
}

static void ui_output_event(lv_event_t *event)
{
    app_settings_snapshot_t settings;

    (void)event;
    app_settings_get(&settings);
    if (app_settings_set_output_enabled(settings.output_enabled == 0U))
    {
        printf("UI output=%lu delayed_save=%ums\r\n",
               (unsigned long)(settings.output_enabled == 0U),
               (unsigned int)APP_WORKSET_SAVE_DELAY_MS);
        app_log_event("output toggled");
    }
    ui_update_params();
}

static void ui_save_event(lv_event_t *event)
{
    (void)event;
    app_settings_request_save_now();
    printf("UI manual save requested\r\n");
    app_log_event("manual save requested");
    ui_update_params();
}

static void ui_upgrade_event(lv_event_t *event)
{
    (void)event;
    s_upgrade_running = true;
    s_upgrade_progress = 0;
    lv_bar_set_value(s_upgrade_bar, 0, LV_ANIM_OFF);
    lv_label_set_text(s_upgrade_status, "Downloading demo package...");
    printf("UI upgrade demo started\r\n");
    app_log_event("upgrade demo started");
}

static void ui_sd_upgrade_event(lv_event_t *event)
{
    (void)event;
    s_upgrade_running = false;
    lv_bar_set_value(s_upgrade_bar, 0, LV_ANIM_OFF);
    lv_label_set_text(s_upgrade_status, "SD file upgrade requested. Waiting for task_iap.");
    printf("UI SD upgrade requested\r\n");
    app_log_event("SD upgrade requested");
    app_iap_request_sd_stage();
}

static void ui_update_params(void)
{
    app_settings_snapshot_t settings;
    app_storage_status_t storage;

    app_settings_get(&settings);
    app_storage_get_status(&storage);

    lv_slider_set_value(s_brightness_slider, (int32_t)settings.brightness, LV_ANIM_OFF);
    lv_slider_set_value(s_threshold_slider, settings.threshold, LV_ANIM_OFF);
    lv_label_set_text(s_output_button_label, settings.output_enabled ? "Output ON" : "Output OFF");

    lv_label_set_text_fmt(s_param_status,
                          "Brightness: %lu%%   Threshold: %ld   Dirty: %s\n"
                          "Flash: %s  Load: %s  Save OK/Fail: %lu/%lu\n"
                          "Status: %s",
                          (unsigned long)settings.brightness,
                          (long)settings.threshold,
                          app_settings_is_dirty() ? "yes" : "no",
                          storage.flash_ready ? "ready" : "fail",
                          storage.load_result,
                          (unsigned long)storage.save_ok_count,
                          (unsigned long)storage.save_fail_count,
                          storage.last_status);
}

static void ui_update_home(uint32_t now_ms)
{
    app_settings_snapshot_t settings;
    app_storage_status_t storage;
    app_diag_snapshot_t diag;
    app_task_runtime_status_t tasks;
    app_system_snapshot_t system;
    app_clock_info_t clk;
    uint32_t seconds = now_ms / 1000U;

    app_settings_get(&settings);
    app_storage_get_status(&storage);
    app_diag_get_snapshot(&diag);
    app_tasks_get_runtime_status(&tasks);
    app_system_model_get_snapshot(&system);
    clk = app_clock_get_info();

    lv_label_set_text_fmt(s_home_status,
                          "Uptime: %lu s   RTOS: running   Storage: %s\n"
                          "W25Q JEDEC: 0x%06lX   Params: %s\n"
                          "Reset: %s   Boot: %lu   Fault: %s count=%lu\n"
                          "Heap: %lu/%lu B   Stack G/S/L/I/C/Core/Idle: %lu/%lu/%lu/%lu/%lu/%lu/%lu",
                          (unsigned long)seconds,
                          storage.flash_ready ? "ready" : "fail",
                          (unsigned long)storage.jedec_id,
                          storage.settings_loaded_from_flash ? "flash" : "default",
                          diag.reset_reason,
                          (unsigned long)diag.boot_count,
                          diag.fault_valid ? app_diag_fault_name(diag.fault_type) : "none",
                          (unsigned long)diag.fault_count,
                          (unsigned long)tasks.heap_free_bytes,
                          (unsigned long)tasks.heap_min_free_bytes,
                          (unsigned long)tasks.gui_stack_free_words,
                          (unsigned long)tasks.storage_stack_free_words,
                          (unsigned long)tasks.log_stack_free_words,
                          (unsigned long)tasks.iap_stack_free_words,
                          (unsigned long)tasks.comm_stack_free_words,
                          (unsigned long)tasks.core_stack_free_words,
                          (unsigned long)tasks.idle_stack_free_words);

    lv_label_set_text_fmt(s_home_values,
                          "DSP: %s Vbus=%ld.%ld V Iout=%ld.%ld A Temp=%ld.%ld C frame=%lu err=%lu/%lu\n"
                          "BMS: %s SOC=%lu.%lu%% Vbat=%ld.%ld V Ibat=%ld.%ld A frame=%lu timeout=%lu\n"
                          "MODBUS: req=%lu exc=%lu crc=%lu\n"
                          "Brightness: %lu%%   Threshold: %ld   Output: %s\n"
                          "Clock SYS/HCLK/LTDC/FMC: %lu/%lu/%lu/%lu MHz",
                          system.dsp.online ? "online" : "offline",
                          (long)(system.dsp.bus_voltage_v_x10 / 10),
                          (long)((system.dsp.bus_voltage_v_x10 < 0) ? -(system.dsp.bus_voltage_v_x10 % 10) : (system.dsp.bus_voltage_v_x10 % 10)),
                          (long)(system.dsp.output_current_a_x10 / 10),
                          (long)((system.dsp.output_current_a_x10 < 0) ? -(system.dsp.output_current_a_x10 % 10) : (system.dsp.output_current_a_x10 % 10)),
                          (long)(system.dsp.temperature_c_x10 / 10),
                          (long)((system.dsp.temperature_c_x10 < 0) ? -(system.dsp.temperature_c_x10 % 10) : (system.dsp.temperature_c_x10 % 10)),
                          (unsigned long)system.dsp.frame_count,
                          (unsigned long)system.dsp.crc_error_count,
                          (unsigned long)system.dsp.timeout_count,
                          system.bms.online ? "online" : "offline",
                          (unsigned long)(system.bms.soc_percent_x10 / 10U),
                          (unsigned long)(system.bms.soc_percent_x10 % 10U),
                          (long)(system.bms.pack_voltage_v_x10 / 10),
                          (long)((system.bms.pack_voltage_v_x10 < 0) ? -(system.bms.pack_voltage_v_x10 % 10) : (system.bms.pack_voltage_v_x10 % 10)),
                          (long)(system.bms.pack_current_a_x10 / 10),
                          (long)((system.bms.pack_current_a_x10 < 0) ? -(system.bms.pack_current_a_x10 % 10) : (system.bms.pack_current_a_x10 % 10)),
                          (unsigned long)system.bms.frame_count,
                          (unsigned long)system.bms.timeout_count,
                          (unsigned long)system.modbus.request_count,
                          (unsigned long)system.modbus.exception_count,
                          (unsigned long)system.modbus.crc_error_count,
                          (unsigned long)settings.brightness,
                          (long)settings.threshold,
                          settings.output_enabled ? "ON" : "OFF",
                          (unsigned long)(clk.sysclk_hz / 1000000UL),
                          (unsigned long)(clk.hclk_hz / 1000000UL),
                          (unsigned long)(clk.ltdc_pixel_hz / 1000000UL),
                          (unsigned long)(clk.fmc_kernel_hz / 1000000UL));
}

static void ui_update_upgrade(uint32_t now_ms)
{
    app_iap_status_t iap;

    (void)now_ms;

    if (!s_upgrade_running)
    {
        app_iap_get_status(&iap);
        lv_label_set_text_fmt(s_upgrade_status,
                              "IAP: %s\n"
                              "Serial: %s   USB: %s   SD: %s\n"
                              "Pending AppB: %s version=%lu size=%lu crc=0x%08lX\n"
                              "Boot: %s active=%lu trial=%lu attempts=%lu/%lu\n"
                              "Recv: %s %lu/%lu",
                              iap.status,
                              iap.serial_ready ? "ready" : "off",
                              iap.usb_ready ? "ready" : "reserved",
                              iap.sd_ready ? "ready" : "reserved",
                              iap.pending_valid ? "yes" : "no",
                              (unsigned long)iap.app_b_version,
                              (unsigned long)iap.app_b_size,
                              (unsigned long)iap.app_b_crc32,
                              iap.boot_state_valid ? app_iap_boot_state_name(iap.boot_state) : "none",
                              (unsigned long)iap.boot_active_slot,
                              (unsigned long)iap.boot_trial_slot,
                              (unsigned long)iap.boot_attempts,
                              (unsigned long)iap.boot_max_attempts,
                              iap.recv_active ? "active" : "idle",
                              (unsigned long)iap.recv_received,
                              (unsigned long)iap.recv_expected);
        return;
    }

    if (s_upgrade_progress < 100U)
    {
        s_upgrade_progress += 2U;
        if (s_upgrade_progress > 100U)
        {
            s_upgrade_progress = 100U;
        }
        lv_bar_set_value(s_upgrade_bar, (int32_t)s_upgrade_progress, LV_ANIM_OFF);
        lv_label_set_text_fmt(s_upgrade_status, "Progress: %lu%%  CRC32 check pending", (unsigned long)s_upgrade_progress);
    }
    else
    {
        s_upgrade_running = false;
        app_iap_request_demo_stage(APP_IAP_SOURCE_SERIAL);
        lv_label_set_text(s_upgrade_status, "Demo package verified. Staging to W25Q AppB area.");
        printf("UI upgrade demo verified\r\n");
        app_log_event("upgrade demo verified");
    }
}

static void ui_update_log(void)
{
    char lines[APP_LOG_LINE_COUNT][APP_LOG_LINE_LEN];
    uint32_t count = app_log_copy_recent(lines, APP_LOG_LINE_COUNT);

    for (uint32_t i = 0; i < APP_LOG_LINE_COUNT; ++i)
    {
        if (i < count)
        {
            lv_label_set_text(s_log_labels[i], lines[i]);
        }
        else
        {
            lv_label_set_text(s_log_labels[i], "");
        }
    }

    (void)s_active_page;
}
