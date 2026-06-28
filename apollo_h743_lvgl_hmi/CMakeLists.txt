cmake_minimum_required(VERSION 3.22)

project(apollo_h743_lvgl_hmi C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(BOOT_TARGET apollo_h743_bootloader)
set(APP_TARGET apollo_h743_app)
set(APP_B_TARGET apollo_h743_app_b)

set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/App)
set(BOOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Boot)
set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Common)
set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/lvgl)
set(FREERTOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/FreeRTOS/Source)
set(USB_DEVICE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/STM32_USB_Device_Library)
set(FATFS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/FATFS/source)
set(PACK_APP_PY ${CMAKE_CURRENT_SOURCE_DIR}/Tools/pack_app/pack_app.py)

set(BOOT_LINKER_SCRIPT ${BOOT_DIR}/Linker/STM32H743IITx_BOOT.ld)
set(APP_A_LINKER_SCRIPT ${APP_DIR}/Linker/STM32H743IITx_APP_A.ld)
set(APP_B_LINKER_SCRIPT ${APP_DIR}/Linker/STM32H743IITx_APP_B.ld)

set(STARTUP_SOURCE
    ${APP_DIR}/Drivers/CMSIS/Device/ST/STM32H7xx/Source/Templates/gcc/startup_stm32h743xx.s
)

set(HAL_MIN_SOURCES
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_qspi.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c
)

set(HAL_APP_EXTRA_SOURCES
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc_ex.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sdram.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd_ex.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_fmc.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_sdmmc.c
    ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.c
)

set(USB_DEVICE_SOURCES
    ${USB_DEVICE_DIR}/Core/Src/usbd_core.c
    ${USB_DEVICE_DIR}/Core/Src/usbd_ctlreq.c
    ${USB_DEVICE_DIR}/Core/Src/usbd_ioreq.c
    ${USB_DEVICE_DIR}/Class/CDC/Src/usbd_cdc.c
)

set(COMMON_SOURCES
    ${COMMON_DIR}/Src/app_crc32.c
    ${COMMON_DIR}/Src/app_iap_record.c
    ${COMMON_DIR}/Src/app_tag.c
)

file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS
    ${LVGL_DIR}/src/*.c
)

list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/draw/sdl/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/draw/arm2d/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/draw/nxp/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/draw/renesas/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/draw/stm32_dma2d/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/draw/swm341_dma2d/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/extra/layouts/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/extra/libs/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/extra/others/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/extra/themes/basic/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/extra/themes/mono/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/extra/widgets/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_dejavu_.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_8\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_10\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_12\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_12_subpx\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_16\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_18\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_20\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_22\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_24\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_26\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_28\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_28_compressed\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_30\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_32\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_34\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_36\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_38\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_40\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_42\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_44\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_46\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_montserrat_48\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_simsun_.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/font/lv_font_unscii_.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/misc/lv_templ\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/widgets/lv_objx_templ\\.c$")

function(apollo_common_target_options target linker_script)
    target_include_directories(${target} PRIVATE
        ${COMMON_DIR}/Inc
        ${APP_DIR}/Drivers/STM32H7xx_HAL_Driver/Inc
        ${APP_DIR}/Drivers/CMSIS/Core/Include
        ${APP_DIR}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    )

    target_compile_definitions(${target} PRIVATE
        USE_HAL_DRIVER
        STM32H743xx
        HSE_VALUE=25000000U
    )

    target_compile_options(${target} PRIVATE
        -mcpu=cortex-m7
        -mthumb
        -mfpu=fpv5-d16
        -mfloat-abi=hard
        -ffunction-sections
        -fdata-sections
        -Wall
        -Wextra
        -Wno-unused-parameter
        $<$<COMPILE_LANGUAGE:C>:-fno-common>
    )

    target_link_options(${target} PRIVATE
        -mcpu=cortex-m7
        -mthumb
        -mfpu=fpv5-d16
        -mfloat-abi=hard
        -T${linker_script}
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${target}.map
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs
        --specs=nosys.specs
    )

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME ${target}
        SUFFIX ".elf"
    )
endfunction()

add_executable(${BOOT_TARGET}
    ${BOOT_DIR}/Core/Src/main.c
    ${BOOT_DIR}/Core/Src/boot_jump.c
    ${BOOT_DIR}/Core/Src/boot_iap.c
    ${BOOT_DIR}/Core/Src/stm32h7xx_it.c
    ${COMMON_SOURCES}
    ${APP_DIR}/Core/Src/system_clock.c
    ${APP_DIR}/Core/Src/uart.c
    ${APP_DIR}/Core/Src/led.c
    ${APP_DIR}/Core/Src/app_w25q256.c
    ${APP_DIR}/Core/Src/syscalls.c
    ${APP_DIR}/Core/Src/system_stm32h7xx.c
    ${STARTUP_SOURCE}
    ${HAL_MIN_SOURCES}
)

target_include_directories(${BOOT_TARGET} PRIVATE
    ${BOOT_DIR}/Core/Inc
    ${APP_DIR}/Core/Inc
)
apollo_common_target_options(${BOOT_TARGET} ${BOOT_LINKER_SCRIPT})

add_custom_command(TARGET ${BOOT_TARGET} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${BOOT_TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${BOOT_TARGET}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${BOOT_TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${BOOT_TARGET}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${BOOT_TARGET}>
)

set(APP_SOURCES
    ${APP_DIR}/Core/Src/main.c
    ${APP_DIR}/Core/Src/system_clock.c
    ${APP_DIR}/Core/Src/uart.c
    ${APP_DIR}/Core/Src/led.c
    ${APP_DIR}/Core/Src/sdram.c
    ${APP_DIR}/Core/Src/lcd.c
    ${APP_DIR}/Core/Src/touch.c
    ${APP_DIR}/Core/Src/app_critical.c
    ${APP_DIR}/Core/Src/app_comm_bus.c
    ${APP_DIR}/Core/Src/app_diag.c
    ${APP_DIR}/Core/Src/app_iap.c
    ${APP_DIR}/Core/Src/app_log.c
    ${APP_DIR}/Core/Src/app_sd.c
    ${APP_DIR}/Core/Src/app_settings.c
    ${APP_DIR}/Core/Src/app_system_model.c
    ${APP_DIR}/Core/Src/app_w25q256.c
    ${APP_DIR}/Core/Src/app_storage.c
    ${APP_DIR}/Core/Src/app_ui_hmi.c
    ${APP_DIR}/Core/Src/app_tasks.c
    ${APP_DIR}/Core/Src/app_vector.c
    ${APP_DIR}/Core/Src/app_usb_cdc.c
    ${APP_DIR}/Core/Src/lvgl_port.c
    ${APP_DIR}/Core/Src/stm32h7xx_it.c
    ${APP_DIR}/Core/Src/syscalls.c
    ${APP_DIR}/Core/Src/system_stm32h7xx.c
    ${APP_DIR}/Core/Src/usb_device.c
    ${APP_DIR}/Core/Src/usbd_conf.c
    ${APP_DIR}/Core/Src/usbd_desc.c
    ${COMMON_SOURCES}
    ${FATFS_DIR}/diskio.c
    ${FATFS_DIR}/ff.c
    ${FATFS_DIR}/ffunicode.c
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/list.c
    ${FREERTOS_DIR}/event_groups.c
    ${FREERTOS_DIR}/stream_buffer.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
    ${FREERTOS_DIR}/portable/GCC/ARM_CM7/r0p1/port.c
    ${STARTUP_SOURCE}
    ${HAL_MIN_SOURCES}
    ${HAL_APP_EXTRA_SOURCES}
    ${USB_DEVICE_SOURCES}
    ${LVGL_SOURCES}
)

function(apollo_add_app_target target linker_script slot_name run_addr version max_size slot_output tag_output slot_hex_output slot_hex_addr)
    add_executable(${target}
        ${APP_SOURCES}
    )

    target_include_directories(${target} PRIVATE
        ${APP_DIR}/Core/Inc
        ${LVGL_DIR}
        ${LVGL_DIR}/src
        ${USB_DEVICE_DIR}/Core/Inc
        ${USB_DEVICE_DIR}/Class/CDC/Inc
        ${FATFS_DIR}
        ${FREERTOS_DIR}/include
        ${FREERTOS_DIR}/portable/GCC/ARM_CM7/r0p1
    )
    apollo_common_target_options(${target} ${linker_script})
    target_compile_definitions(${target} PRIVATE
        LV_CONF_INCLUDE_SIMPLE
        APP_USE_FREERTOS
        APP_USB_CDC_ENABLED
        APP_BUILD_SLOT_NAME=\"${slot_name}\"
        APP_BUILD_RUN_ADDR=${run_addr}
        APP_BUILD_VERSION=${version}
    )

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR}/${target}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR}/${target}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target}>
        COMMAND ${CMAKE_COMMAND} -E env python ${PACK_APP_PY}
            --input ${CMAKE_CURRENT_BINARY_DIR}/${target}.bin
            --output ${CMAKE_CURRENT_BINARY_DIR}/${slot_output}
            --tag-output ${CMAKE_CURRENT_BINARY_DIR}/${tag_output}
            --run-addr ${run_addr}
            --version ${version}
            --max-size ${max_size}
        COMMAND ${CMAKE_OBJCOPY} -I binary -O ihex --change-addresses ${slot_hex_addr}
            ${CMAKE_CURRENT_BINARY_DIR}/${slot_output}
            ${CMAKE_CURRENT_BINARY_DIR}/${slot_hex_output}
    )
endfunction()

apollo_add_app_target(${APP_TARGET}
    ${APP_A_LINKER_SCRIPT}
    "AppA"
    0x08020400
    1
    0x000DFC00
    app_a_slot.bin
    app_a_tag.bin
    app_a_slot.hex
    0x08020000
)

apollo_add_app_target(${APP_B_TARGET}
    ${APP_B_LINKER_SCRIPT}
    "AppB"
    0x08100400
    2
    0x000FFC00
    app_b_slot.bin
    app_b_tag.bin
    app_b_slot.hex
    0x08100000
)
