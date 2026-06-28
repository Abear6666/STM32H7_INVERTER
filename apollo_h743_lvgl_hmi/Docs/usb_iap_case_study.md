# STM32H743 本地 IAP 升级项目复盘

日期：2026-06-27

## 1. 项目背景

本项目基于 STM32H743，目标是在已有 HMI 应用上实现安全、可回退的本地 IAP 升级能力。系统由 Bootloader、AppA、AppB、外部 W25Q256、FreeRTOS、LVGL HMI、USB CDC、SD 卡文件系统组成。

核心设计不是让 App 直接覆盖正在运行的程序，而是采用 staging + pending + Boot 安装的两阶段升级：

1. App 侧负责从本地升级源接收升级包，包括 USART1、USB CDC 虚拟串口、SD 卡文件。
2. App 将升级包写入外部 W25Q256 staging 区。
3. App 完成流式 CRC、W25Q 读回 CRC、AppB tag 和 App 本体 CRC 校验。
4. 所有校验通过后，App 写入 pending 记录。
5. 复位后 Bootloader 读取 pending，校验 staging 包，再擦写内部 Flash 的 AppB 分区。
6. Boot 写入 trial 状态并启动 AppB。
7. AppB 成功启动后写入 confirmed 状态。
8. 如果升级包无效、写入失败、AppB 校验失败或 trial 次数耗尽，Boot 回退 AppA。

外部 W25Q256 分区：

```text
0x01A00000 - 0x01A00FFF   pending 记录
0x01A01000 - 0x01A01FFF   boot state 记录
0x01A02000 - 0x01FFFFFF   staging 数据区
```

本次复盘覆盖三条升级链路和两个启动稳定性问题：

- USB CDC IAP：Windows Code 10、协议无回包、CDC 发送阻塞。
- Boot 跳 App：`BASEPRI=0x50` 残留导致 App 启动或 RTOS tick 异常。
- FreeRTOS 临界区：调度器启动前误用 `taskENTER_CRITICAL()` 带来的中断屏蔽风险。
- SD 卡 IAP：`SD mount fail`、`FR_DISK_ERR=1`、`SDMMC_ERROR_RX_OVERRUN=0x00000020`。
- 烧录验证：只烧 Boot/AppA 时，Boot 仍可能按 boot state 启动旧 AppB，导致误判修复未生效。

## 2. 总体升级流程

### 2.1 串口和 USB CDC 升级

PC 端发送命令：

```text
iap recv <size> <crc32> <version>
```

App 侧流程：

1. 解析 `size/crc32/version`。
2. 擦除旧 pending。
3. 校验升级包大小和版本。
4. 擦除 W25Q staging 区。
5. 返回 `ready for binary`。
6. 接收固定长度二进制包。
7. 边接收边计算流式 CRC32。
8. 分块写入 W25Q staging。
9. 接收完成后从 W25Q 读回并再次计算 CRC32。
10. 读取升级包中的 `app_tag_t`，确认它是 AppB 包。
11. 校验 App 本体 CRC32。
12. 写 pending。
13. 提示复位，由 Boot 安装 AppB。

### 2.2 SD 卡文件升级

SD 卡升级复用同一套 staging、pending、Boot 安装、trial/confirmed 机制。

App 侧固定读取：

```text
0:/app_b_slot.bin
```

SD 卡升级流程：

1. HMI 点击 `SD File` 或串口发送 `iap sd`。
2. App 调用 FatFs 挂载 `0:`。
3. 打开 `0:/app_b_slot.bin`。
4. 读取文件尾部 App tag，确认是 AppB slot 包。
5. 调用统一的 `app_iap_start_recv()`，擦除 pending 和 staging。
6. 使用 `f_read()` 分块读取 SD 文件。
7. 每块数据写入 W25Q staging，并计算整包 CRC。
8. 读完后复用串口/USB 的 `app_iap_finish_recv()` 校验和写 pending。
9. 复位后 Boot 安装 AppB。

### 2.3 Boot 安装和回退策略

Bootloader 启动后先检查 W25Q pending：

1. pending 无效：按 boot state 选择 AppA 或 AppB。
2. pending 有效：复算 W25Q staging CRC。
3. 校验 staged AppB tag 和 App 本体 CRC。
4. 擦除内部 Flash AppB。
5. 分块写入 AppB。
6. 读回内部 Flash 与 staging 比对。
7. 再校验 AppB tag 和 CRC。
8. 清除 pending。
9. 写入 boot state 为 trial。
10. 启动 AppB。
11. AppB 成功运行后写入 confirmed。

这套设计的安全边界是：AppA 不被 App 侧升级流程擦写。只有 Boot 在 pending 完整有效并多层校验通过后才会写 AppB，因此传输中断、CRC 错误、文件错误、SD mount 失败都不会破坏已有可启动程序。

## 3. 问题一：USB CDC Code 10 和协议不可用

### 3.1 现象

调试 USB CDC IAP 时，先后遇到这些现象：

- Windows 设备管理器出现 `USB 串行设备 (COM4)`，但状态为 Code 10。
- 后续 COM4 可以枚举，但 PC 工具发送升级命令后等待板端响应超时。
- 打开 COM4 后，如果把所有 `printf` 日志同步镜像到 USB CDC，系统容易卡在 CDC 发送路径。
- COM3 串口能看到 App 日志，但 COM4 上位机收不到 `ready for binary`。

### 3.2 根因分析

这是多个问题叠加：

1. USB CDC 描述符不完整，Windows 对 CDC ACM 组合接口识别不稳定。
2. CDC control/data IN 端点 Tx FIFO 配置不完整，可能影响端点发送。
3. 将全量日志镜像到 CDC，会让升级协议通道被大量日志占用，甚至卡在发送 busy。
4. 取消日志镜像后，IAP 协议没有针对 USB 源单独回包，导致 PC 工具无法知道板端已经 ready。

### 3.3 代码修改

设备描述符改为 composite with IAD：

```c
/* App/Core/Src/usbd_desc.c */
0xEF, /* bDeviceClass: Miscellaneous */
0x02, /* bDeviceSubClass: Common Class */
0x01, /* bDeviceProtocol: Interface Association Descriptor */
```

CDC 配置描述符长度从 67 改为 75：

```c
/* Middlewares/STM32_USB_Device_Library/Class/CDC/Inc/usbd_cdc.h */
#define USB_CDC_CONFIG_DESC_SIZ 75U
```

在 CDC control interface 前插入 IAD：

```c
/* Middlewares/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c */
0x08,               /* bLength */
USB_DESC_TYPE_IAD,  /* bDescriptorType */
0x00,               /* bFirstInterface */
0x02,               /* bInterfaceCount */
0x02,               /* bFunctionClass: CDC */
0x02,               /* bFunctionSubClass: ACM */
0x01,               /* bFunctionProtocol */
0x00,               /* iFunction */
```

配置 USB OTG FS FIFO：

```c
/* App/Core/Src/usbd_conf.c */
HAL_PCDEx_SetRxFiFo(&hpcd_USB_FS, 0x80U);
HAL_PCDEx_SetTxFiFo(&hpcd_USB_FS, 0U, 0x40U);
HAL_PCDEx_SetTxFiFo(&hpcd_USB_FS, 1U, 0x40U);
HAL_PCDEx_SetTxFiFo(&hpcd_USB_FS, 2U, 0x20U);
```

取消 `_write()` 全量镜像到 USB CDC。COM3 保持为诊断日志通道，COM4 只作为升级协议通道：

```c
/* App/Core/Src/uart.c */
int _write(int file, char *ptr, int len)
{
    ...
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
    ...
}
```

新增 USB 源定向回包：

```c
/* App/Core/Src/app_iap.c */
static void app_iap_reply_source(app_iap_source_t source, const char *text)
{
    if ((source == APP_IAP_SOURCE_USB) && (text != NULL) && s_iap_status.usb_ready)
    {
        app_usb_cdc_write_text(text);
    }
}
```

在关键协议节点返回短文本：

```c
snprintf(line, sizeof(line),
         "IAP USB: ready for binary size=%lu\r\n",
         (unsigned long)size);
app_iap_reply_source(source, line);
```

```c
snprintf(line,
         sizeof(line),
         "IAP USB: package verified, pending flag set version=%lu size=%lu crc=0x%08lX\r\n",
         (unsigned long)pending.app_version,
         (unsigned long)pending.app_size,
         (unsigned long)pending.app_crc32);
app_iap_reply_source(source, line);
```

### 3.4 结果

USB CDC 能稳定枚举为 MCU 虚拟 COM 口，PC 工具能收到：

```text
IAP USB: ready for binary size=323140
IAP USB: package verified, pending flag set version=2 size=323140 crc=0xB55AEED7
```

## 4. 问题二：Boot 跳转 App 后中断屏蔽状态残留

### 4.1 现象

烧录后怀疑 App 没正常运行，表现为：

- App 早期初始化卡住。
- `HAL_Delay()` 或依赖 SysTick 的逻辑异常。
- FreeRTOS tick 或任务调度不稳定。

现场调试发现 App 进入后 `BASEPRI=0x50`。这会屏蔽一部分优先级较低的中断，典型影响就是 SysTick、RTOS tick 或外设中断无法按预期运行。

### 4.2 根因

Cortex-M 上 Bootloader 和 App 不是两个互相隔离的进程。Boot 跳到 App 时，内核寄存器状态会继承。只设置 MSP 和 VTOR 不够，还必须清理会影响 App 的 CPU 状态。

`BASEPRI=0x50` 常见于 FreeRTOS 临界区，用于屏蔽低优先级中断，让内核在修改队列、任务链表等共享结构时不被可调用 FreeRTOS API 的中断打断。如果 Boot 或 App 启动前某处进入临界区后没有正确恢复，App 就会继承这个中断屏蔽阈值。

### 4.3 代码修改

Boot 跳转前清理外设、中断、SysTick、NVIC pending，并清理内核寄存器：

```c
/* Boot/Core/Src/boot_jump.c */
__disable_irq();
HAL_RCC_DeInit();
HAL_DeInit();

SysTick->CTRL = 0;
SysTick->LOAD = 0;
SysTick->VAL = 0;

for (uint32_t i = 0; i < 8U; ++i)
{
    NVIC->ICER[i] = 0xFFFFFFFFUL;
    NVIC->ICPR[i] = 0xFFFFFFFFUL;
}

SCB->VTOR = app_run_addr;
__DSB();
__ISB();
__set_BASEPRI(0U);
__set_FAULTMASK(0U);
__set_CONTROL(0U);
__ISB();
__set_MSP(app_msp);
__enable_irq();

((boot_app_entry_t)app_reset)();
```

App 早期再做一次防御性清理：

```c
/* App/Core/Src/main.c */
app_vector_relocate_to_ram();
__set_BASEPRI(0U);
```

### 4.4 面试讲法

可以这样讲：

> Bootloader 跳 App 不能只设置 MSP 和 Reset_Handler。Cortex-M 的 `BASEPRI/FAULTMASK/CONTROL/VTOR/SysTick/NVIC pending` 等状态都会影响新 App。现场 App 启动后 `BASEPRI=0x50`，导致 SysTick 或 RTOS tick 被屏蔽，所以表现为 HAL_Delay 卡住或任务不调度。我在 Boot 跳转前清理 `BASEPRI/FAULTMASK/CONTROL`，同时清理 SysTick 和 NVIC pending，App 早期再清一次 BASEPRI，保证 App 在干净上下文启动。

## 5. 问题三：调度器启动前误用 FreeRTOS 临界区

### 5.1 现象

部分模块在 FreeRTOS 调度器启动前就会访问共享状态，例如日志、存储状态、IAP 状态。如果直接使用 `taskENTER_CRITICAL()`，会引入两个问题：

- 调度器未启动时，FreeRTOS 临界区语义不完全符合应用预期。
- `taskENTER_CRITICAL()` 在 Cortex-M FreeRTOS 里会操作 `BASEPRI`，如果进入/退出时序异常，就可能留下中断屏蔽状态。

### 5.2 代码修改

新增统一临界区封装：

```c
/* App/Core/Src/app_critical.c */
uint32_t app_critical_enter(void)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        taskENTER_CRITICAL();
        return UINT32_MAX;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void app_critical_exit(uint32_t primask)
{
    if (primask == UINT32_MAX)
    {
        taskEXIT_CRITICAL();
        return;
    }

    if ((primask & 1U) == 0U)
    {
        __enable_irq();
    }
}
```

策略：

- 调度器启动前：保存并恢复 `PRIMASK`。
- 调度器启动后：使用 FreeRTOS 临界区。

涉及文件：

- `App/Core/Inc/app_critical.h`
- `App/Core/Src/app_critical.c`
- `App/Core/Src/app_log.c`
- `App/Core/Src/app_storage.c`
- `App/Core/Src/app_iap.c`
- `CMakeLists.txt`

### 5.3 面试讲法

可以这样讲：

> 我没有在每个模块里散落 `if scheduler started` 判断，而是封装了 `app_critical_enter/exit`。调度器启动前走 `PRIMASK` 保存恢复，启动后走 FreeRTOS critical section。这样既避免启动阶段误留 `BASEPRI`，也保持任务运行阶段的 RTOS 互斥语义。

## 6. 问题四：SD 卡升级提示 SD mount fail

### 6.1 初始现象

用户插入 4GB FAT32 SD 卡，根目录有：

```text
app_b_slot.bin
```

HMI 点击 `SD File` 或串口发送 `iap sd` 后，日志显示：

```text
IAP SD: file staging requested: 0:/app_b_slot.bin
SD: mount failed, fresult=1
iap status: ... state=SD mount fail
```

FatFs 返回：

```text
FR_DISK_ERR = 1
```

这说明不是文件不存在，也不是文件名错误，而是底层块设备读写失败。

### 6.2 增加诊断日志

为避免只看到上层 `FR_DISK_ERR`，在 SD 驱动和 FatFs diskio 层增加底层错误码。

头文件增加：

```c
/* App/Core/Inc/app_sd.h */
uint32_t app_sd_get_last_error(void);
uint32_t app_sd_get_card_state(void);
```

SD 读失败时打印 sector、count、HAL error、card state：

```c
/* App/Core/Src/app_sd.c */
if (status != HAL_OK)
{
    printf("SD: HAL_SD_ReadBlocks failed sector=%lu count=%lu err=0x%08lX state=%lu\r\n",
           (unsigned long)sector,
           (unsigned long)count,
           (unsigned long)HAL_SD_GetError(&s_sd_handle),
           (unsigned long)HAL_SD_GetCardState(&s_sd_handle));
    return false;
}
```

FatFs diskio 层打印底层失败：

```c
/* Middlewares/FATFS/source/diskio.c */
printf("disk_read: SD read failed sector=%lu count=%u err=0x%08lX state=%lu\r\n",
       (unsigned long)sector,
       (unsigned int)count,
       (unsigned long)app_sd_get_last_error(),
       (unsigned long)app_sd_get_card_state());
```

诊断日志发现：

```text
SD: init OK type=1 class=0x000005B5 rel=58916 blocks=7744512 block_size=512 err=0x00000000 state=4
SD: HAL_SD_ReadBlocks failed sector=0 count=1 err=0x00000020 state=5
disk_read: SD read failed sector=0 count=1 err=0x00000020 state=4
SD: mount failed, fresult=1
```

关键错误：

```text
0x00000020 = SDMMC_ERROR_RX_OVERRUN
```

### 6.3 中间状态

将 SDMMC 从 4-bit 高速配置调整为保守配置后，`f_mount()` 可以通过，但文件读取过程中仍然偶发 RX overrun：

```text
SD: mounted at 0:, blocks=7744512 block_size=512 capacity=3781 MB
SD: HAL_SD_ReadBlocks failed sector=16449 count=1 err=0x00000020 state=4
IAP SD: seek tag failed, fresult=1
state=SD tag read fail
```

这说明：

- SD 卡格式、容量、文件名不是根因。
- SD 初始化和容量读取正常。
- FatFs 挂载可以成功。
- 真正失败点在文件读数据阶段，SDMMC RX FIFO 来不及被 CPU 取走，触发 RX overrun。

### 6.4 根因分析

当前 SD 读块使用 `HAL_SD_ReadBlocks()` 轮询模式。轮询读的本质是 CPU 必须及时从 SDMMC FIFO 取数据。如果在读取过程中被更高优先级任务抢占，或者 SDCLK 太快，就可能导致 FIFO 溢出。

本项目任务优先级：

```text
task_gui:     tskIDLE_PRIORITY + 3
task_iap:     tskIDLE_PRIORITY + 1
task_storage: tskIDLE_PRIORITY + 1
task_log:     tskIDLE_PRIORITY + 1
```

SD IAP 运行在 `task_iap`，而 LVGL GUI 任务优先级更高。`HAL_SD_ReadBlocks()` 轮询读期间如果被 GUI 任务抢占，SDMMC FIFO 继续收数据，CPU 没有及时读取，最终触发：

```text
SDMMC_ERROR_RX_OVERRUN = 0x00000020
```

### 6.5 代码修改

SD 配置改为保守稳定优先：

```c
/* App/Core/Src/app_sd.c */
#define APP_SD_CLOCK_DIV 80U

s_sd_handle.Init.BusWide = SDMMC_BUS_WIDE_1B;
s_sd_handle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
s_sd_handle.Init.ClockDiv = APP_SD_CLOCK_DIV;
```

说明：

- `ClockDiv=80U` 降低 SDMMC 时钟，给 CPU 轮询读 FIFO 留出时间。
- `SDMMC_BUS_WIDE_1B` 降低总线带宽压力，先保证稳定。
- `SDMMC_HARDWARE_FLOW_CONTROL_ENABLE` 允许 SDMMC 在 FIFO 压力较大时通过硬件流控暂停节奏，降低 overrun 风险。

在 HAL 轮询读写期间暂停 FreeRTOS 任务调度：

```c
static bool app_sd_suspend_scheduler(void)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
    {
        return false;
    }

    vTaskSuspendAll();
    return true;
}

static void app_sd_resume_scheduler(bool suspended)
{
    if (suspended)
    {
        (void)xTaskResumeAll();
    }
}
```

应用到读块：

```c
bool app_sd_read_blocks(uint8_t *buffer, uint32_t sector, uint32_t count)
{
    bool scheduler_suspended;
    HAL_StatusTypeDef status;

    ...

    scheduler_suspended = app_sd_suspend_scheduler();
    status = HAL_SD_ReadBlocks(&s_sd_handle, buffer, sector, count, APP_SD_TIMEOUT_MS);
    app_sd_resume_scheduler(scheduler_suspended);

    if (status != HAL_OK)
    {
        printf("SD: HAL_SD_ReadBlocks failed sector=%lu count=%lu err=0x%08lX state=%lu\r\n",
               (unsigned long)sector,
               (unsigned long)count,
               (unsigned long)HAL_SD_GetError(&s_sd_handle),
               (unsigned long)HAL_SD_GetCardState(&s_sd_handle));
        return false;
    }

    return app_sd_wait_transfer_ready();
}
```

关键取舍：

- 这里使用 `vTaskSuspendAll()`，不是 `taskENTER_CRITICAL()`。
- `vTaskSuspendAll()` 只暂停任务切换，不关闭中断。
- SysTick 和 HAL tick 仍能运行，`HAL_SD_ReadBlocks()` 内部超时机制不会失效。
- 不适合用 `taskENTER_CRITICAL()` 包住长时间 SD 读，因为那会屏蔽部分中断，容易影响系统时基和外设中断。

### 6.6 验证结果

修复后触发 `iap sd`：

```text
IAP SD: file staging requested: 0:/app_b_slot.bin
SD: init OK type=1 class=0x000005B5 rel=58916 blocks=7744512 block_size=512 err=0x00000000 state=4
SD: mounted at 0:, blocks=7744512 block_size=512 capacity=3781 MB
IAP SD: file tag magic=0x41505447 run=0x08100400 app_size=323540 version=2 app_crc=0xBD293B71
IAP SD: recv begin size=324564 crc=0x00000000 version=2
IAP SD: ready for binary size=324564
IAP SD: file progress 324564/324564
IAP SD: recv done size=324564 crc=0xAF740721
IAP SD: package verified, pending flag set version=2 size=324564 crc=0xAF740721
```

复位后 Boot 安装：

```text
BOOT IAP: pending magic=0x50414950 version=1 target=1 size=323140 app_ver=2 crc=0xB55AEED7 staging=0x01A02000
BOOT IAP: staging crc calc=0xB55AEED7 expect=0xB55AEED7
BOOT IAP: staged AppB package CRC OK, version=2 app_size=322116
BOOT IAP: AppB erase OK sectors=8
BOOT IAP: AppB flash readback OK bytes=323140
BOOT IAP: pending flag cleared
BOOT IAP: boot state written: trial AppB version=2 max_attempts=3
BOOT IAP: AppB update applied and verified, trial pending, AppA untouched
BOOT: selected slot=AppB
APP: AppB start from 0x08100400 version=2
IAP confirm: AppB version=2 confirmed
```

最终状态：

```text
iap status: flash=1 pending=0 version=0 size=0 crc=0x00000000 recv=0 0/0 state=AppB confirmed
iap boot state: valid=1 state=confirmed active=1 trial=1 attempts=0/3 err=0
```

## 7. 问题五：烧录后仍然提示 SD mount fail 的误判

### 7.1 现象

用户手动烧录后，点击 HMI 的 `SD File` 仍然看到 `SD mount fail`，以为修复无效。

### 7.2 排查结果

串口启动日志显示：

```text
BOOT IAP: boot state=confirmed active=1 trial=1 version=2 attempts=0/3 err=0
BOOT IAP: confirmed AppB selected
BOOT: selected slot=AppB
APP: AppB start from 0x08100400 version=2
```

这说明 Boot 并没有启动刚烧录的 AppA，而是根据 W25Q boot state 启动了已 confirmed 的 AppB。

如果只烧录 Bootloader + AppA：

```powershell
openocd -c "set DUAL_BANK 1" -f interface/stlink.cfg -f target/stm32h7x.cfg -c "transport select swd" -c "adapter speed 1000" -c "program build/gcc-debug/apollo_h743_bootloader.hex verify" -c "program build/gcc-debug/app_a_slot.hex verify reset exit"
```

由于 W25Q boot state 仍是 `confirmed AppB`，Boot 会继续启动 AppB。如果 AppB 还是旧版本，就会继续出现旧问题。

### 7.3 解决方法

需要完整烧录 Bootloader + AppA + AppB：

```powershell
openocd -c "set DUAL_BANK 1" -f interface/stlink.cfg -f target/stm32h7x.cfg -c "transport select swd" -c "adapter speed 1000" -c "program build/gcc-debug/apollo_h743_bootloader.hex verify" -c "program build/gcc-debug/app_a_slot.hex verify" -c "program build/gcc-debug/app_b_slot.hex verify reset exit"
```

烧录后启动日志变为：

```text
BOOT: tag magic=0x41505447 size=64 run=0x08100400 app_size=323540 version=2 crc=0xBD293B71
BOOT: AppB crc calc=0xBD293B71 expect=0xBD293B71
APP: AppB start from 0x08100400 version=2
```

这证明板上运行的是修复后的 AppB。随后 `iap sd` 成功挂载 SD 并读完整个升级包。

### 7.4 面试讲法

可以这样讲：

> 后续用户反馈“烧录后还是 SD mount fail”，我没有直接继续改 SD 驱动，而是先看 Boot 日志，发现 Boot state 仍然是 confirmed AppB，所以只烧 Boot/AppA 并不会改变实际运行镜像。完整烧录 AppB 后，启动日志里的 AppB tag CRC 和本地构建产物一致，问题才真正被验证。这说明 IAP 项目里必须关注 boot state 和当前 active slot，否则很容易拿旧固件现象误判新代码。

## 8. 最终验证命令

### 8.1 编译

```powershell
cd D:\project\gitdemo\STM32H7_INVERTER\apollo_h743_lvgl_hmi
cmake --build --preset gcc-debug
```

### 8.2 完整烧录

```powershell
openocd -c "set DUAL_BANK 1" -f interface/stlink.cfg -f target/stm32h7x.cfg -c "transport select swd" -c "adapter speed 1000" -c "program build/gcc-debug/apollo_h743_bootloader.hex verify" -c "program build/gcc-debug/app_a_slot.hex verify" -c "program build/gcc-debug/app_b_slot.hex verify reset exit"
```

### 8.3 只烧 Bootloader + AppA

只适合验证 Boot/AppA，或者准备让 AppB 由 IAP 安装。注意如果 W25Q boot state 已 confirmed AppB，复位后仍可能启动旧 AppB。

```powershell
openocd -c "set DUAL_BANK 1" -f interface/stlink.cfg -f target/stm32h7x.cfg -c "transport select swd" -c "adapter speed 1000" -c "program build/gcc-debug/apollo_h743_bootloader.hex verify" -c "program build/gcc-debug/app_a_slot.hex verify reset exit"
```

### 8.4 SD 卡升级测试

将文件复制到 SD 卡根目录：

```text
build/gcc-debug/app_b_slot.bin
```

SD 卡根目录文件名必须是：

```text
app_b_slot.bin
```

串口触发：

```text
iap sd
```

预期日志：

```text
SD: mounted at 0:
IAP SD: file tag ...
IAP SD: file progress ...
IAP SD: package verified, pending flag set ...
```

复位安装：

```powershell
openocd -c "set DUAL_BANK 1" -f interface/stlink.cfg -f target/stm32h7x.cfg -c "transport select swd" -c "adapter speed 1000" -c "init; reset run; shutdown"
```

预期：

```text
BOOT IAP: AppB update applied and verified
BOOT: selected slot=AppB
IAP confirm: AppB version=2 confirmed
```

### 8.5 USB CDC 升级测试

```powershell
.\Tools\iap_serial_cli\bin\Release\net10.0-windows\ApolloIapSerialCli.exe `
  --port COM4 `
  --baud 115200 `
  --file build\gcc-debug\app_b_slot.bin `
  --version 2 `
  --chunk 128 `
  --delay-ms 2 `
  --app-timeout-ms 30000 `
  --ready-timeout-ms 10000 `
  --log-tail-ms 8000 `
  --no-wait-app
```

预期：

```text
IAP USB: ready for binary size=<size>
IAP USB: package verified, pending flag set version=<version> size=<size> crc=<crc>
```

## 9. 本次修改文件清单

### USB CDC

- `App/Core/Src/usbd_desc.c`
  - Device Descriptor 改为 composite/IAD。
- `Middlewares/STM32_USB_Device_Library/Class/CDC/Inc/usbd_cdc.h`
  - `USB_CDC_CONFIG_DESC_SIZ` 改为 `75U`。
- `Middlewares/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c`
  - CDC 配置描述符增加 IAD。
- `App/Core/Src/usbd_conf.c`
  - 配置 EP0、EP1、EP2 Tx FIFO。
- `App/Core/Src/uart.c`
  - 取消全量 `printf` 镜像到 USB CDC。
- `App/Core/Src/app_iap.c`
  - 增加 `app_iap_reply_source()`。
  - 增加 USB IAP ready/success/error 定向回包。

### Boot 和 RTOS 启动稳定性

- `Boot/Core/Src/boot_jump.c`
  - 跳转 App 前清理 SysTick、NVIC、`BASEPRI`、`FAULTMASK`、`CONTROL`。
- `App/Core/Src/main.c`
  - App early init 清理 `BASEPRI`。
- `App/Core/Inc/app_critical.h`
- `App/Core/Src/app_critical.c`
  - 新增调度器启动前后兼容的临界区封装。
- `App/Core/Src/app_log.c`
- `App/Core/Src/app_storage.c`
- `App/Core/Src/app_iap.c`
  - 使用 `app_critical_enter/exit`。

### SD 卡 IAP

- `App/Core/Inc/app_sd.h`
  - 增加 `app_sd_get_last_error()`。
  - 增加 `app_sd_get_card_state()`。
- `App/Core/Src/app_sd.c`
  - SDMMC 改为 1-bit。
  - `APP_SD_CLOCK_DIV` 改为 `80U`。
  - 开启 `SDMMC_HARDWARE_FLOW_CONTROL_ENABLE`。
  - `HAL_SD_ReadBlocks/WriteBlocks` 期间使用 `vTaskSuspendAll()` 防止任务抢占轮询读写。
  - 增加 SD init/read/write/timeout 诊断日志。
- `Middlewares/FATFS/source/diskio.c`
  - 增加 FatFs diskio 层诊断日志，打印 `sector/count/error/state`。

## 10. 面试重点

### 10.1 这个案例体现的能力

- 能设计 Bootloader + AppA/AppB 双分区升级架构。
- 能处理 USB CDC 枚举、描述符、端点 FIFO 等底层问题。
- 能理解 Cortex-M Boot 跳转上下文，不只会设置 MSP/VTOR。
- 能识别 `BASEPRI`、`PRIMASK`、FreeRTOS 临界区的区别和风险。
- 能把升级链路拆成协议接收、外部 Flash staging、CRC 校验、Boot 安装、trial/confirmed 回退。
- 能从 FatFs 上层错误追到 SDMMC HAL 错误码。
- 能根据 `SDMMC_ERROR_RX_OVERRUN` 反推轮询读、任务抢占、SD 时钟和硬件流控问题。
- 能通过日志验证实际运行槽位，避免旧 AppB 干扰新代码验证。

### 10.2 简历项目描述

可以放在项目经历中的版本：

> 负责 STM32H743 HMI 项目的 Bootloader + AppA/AppB 双分区本地 IAP 升级链路调试与闭环，支持 USART1、USB CDC 虚拟串口和 SD 卡文件升级。设计 W25Q256 staging/pending/boot-state 两阶段升级机制，实现升级失败不破坏 AppA；排查并修复 Windows USB CDC Code 10、Boot 跳转后 `BASEPRI` 残留、FreeRTOS 启动前临界区误用、USB CDC 协议回包缺失、SD 卡 `FR_DISK_ERR/RX_OVERRUN` 等问题。最终完成 PC/SD 传输、W25Q staging 校验、Boot 安装 AppB、trial/confirmed 回退策略和实板闭环验证。

简历 bullet：

- 设计并调试 STM32H743 Bootloader + AppA/AppB 双分区 IAP 流程，使用 W25Q256 作为 staging/pending/boot-state 存储，实现失败不破坏 AppA 的升级策略。
- 修复 USB CDC 在 Windows 下 Code 10 枚举异常，补齐 composite/IAD 描述符和 CDC 配置描述符长度，并完善 USB OTG FS 端点 FIFO 配置。
- 定位 Boot 跳转后 App 启动异常，发现 `BASEPRI=0x50` 残留屏蔽 SysTick/RTOS tick，完善 Boot 跳转前 `BASEPRI/FAULTMASK/CONTROL` 清理和 App early init 防御性清理。
- 封装 FreeRTOS 启动前后兼容的临界区接口，避免调度器启动前误用 `taskENTER_CRITICAL()` 引入中断屏蔽状态异常。
- 将 USB CDC 从全量日志通道改为升级协议通道，增加 `ready for binary`、校验成功和 abort reason 等定向回包，避免 printf 镜像导致 CDC 发送阻塞。
- 排查 SD 卡升级 `SD mount fail`，通过 FatFs diskio 诊断定位到底层 `SDMMC_ERROR_RX_OVERRUN`，采用降 SDCLK、1-bit 总线、硬件流控和轮询读期间暂停任务调度解决。
- 完成实板验证：USB CDC 和 SD 卡均可传输 AppB slot 包，板端完成流式 CRC、W25Q256 读回 CRC、AppB tag 校验和 pending 写入，复位后 Boot 安装 AppB 并确认 `AppB confirmed`。

## 11. STAR 面试讲述稿

### Situation

项目是 STM32H743 HMI 控制板，需要支持本地固件升级。系统已有 Bootloader、AppA/AppB 双分区和 W25Q256 外部 Flash，目标是通过 USB CDC、串口或 SD 卡把 AppB 包写入 staging，复位后由 Bootloader 安装到 AppB，并支持 trial/confirmed/rollback。

实际调试中问题很多：Windows USB CDC Code 10、COM4 升级工具等待超时、App 启动卡住、Boot 跳转后 RTOS tick 异常、SD 卡点击升级提示 `SD mount fail`。

### Task

我的任务是把这条 IAP 链路从“接口已预留但不可稳定升级”推进到实板可闭环验证，并保证失败场景不会破坏 AppA。

### Action

我先把链路拆成主机枚举、USB 底层、App 协议、W25Q staging、Boot 安装、RTOS 启动上下文、SD/FatFs 底层读写几个层级。

USB 侧，我修复 CDC composite/IAD 描述符，配置 EP0/EP1/EP2 Tx FIFO，解决 Windows Code 10。然后取消全量 printf 镜像到 USB CDC，把 COM3 定位为日志通道，COM4 定位为升级协议通道，并在 `app_iap_reply_source()` 中给 USB 源返回 `ready for binary`、成功和失败原因。

Boot 侧，我发现 App 启动后 `BASEPRI=0x50`，会屏蔽 SysTick 或 RTOS tick，所以在 Boot 跳转前清理 SysTick、NVIC pending、`BASEPRI/FAULTMASK/CONTROL`，App 早期也防御性清理 `BASEPRI`。同时封装 `app_critical_enter/exit`，调度器启动前用 `PRIMASK`，启动后用 FreeRTOS 临界区。

SD 侧，最初只看到 FatFs `FR_DISK_ERR=1` 和 UI `SD mount fail`。我在 `app_sd.c` 和 `diskio.c` 增加 sector/count/HAL error/card state 日志，定位到底层 `SDMMC_ERROR_RX_OVERRUN=0x00000020`。结合任务优先级分析，判断是 SD 轮询读期间被高优先级 GUI 任务抢占，FIFO 来不及读取。我将 SDMMC 改为 1-bit、降低 SDCLK、开启硬件流控，并在 `HAL_SD_ReadBlocks/WriteBlocks` 期间用 `vTaskSuspendAll()` 暂停任务切换但不关中断，保证 HAL 超时仍可工作。

最后还有一次误判：用户只烧了 Boot/AppA，但 Boot state 仍是 confirmed AppB，所以实际跑的是旧 AppB。我通过 Boot 日志确认 active slot，完整烧录 AppB 后再验证，避免旧固件干扰。

### Result

最终 USB CDC 和 SD 卡升级都跑通。USB 侧 PC 工具能收到：

```text
IAP USB: ready for binary size=323140
IAP USB: package verified, pending flag set version=2 size=323140 crc=0xB55AEED7
```

SD 侧能完整读取 FAT32 SD 卡根目录 `app_b_slot.bin`：

```text
SD: mounted at 0:, blocks=7744512 block_size=512 capacity=3781 MB
IAP SD: file progress 324564/324564
IAP SD: package verified, pending flag set version=2 size=324564 crc=0xAF740721
```

复位后 Bootloader 安装 AppB 并确认：

```text
BOOT IAP: AppB update applied and verified, trial pending, AppA untouched
APP: AppB start from 0x08100400 version=2
IAP confirm: AppB version=2 confirmed
```

## 12. 面试官可能追问

### Q1：为什么升级包先写 W25Q256，而不是 App 直接写内部 Flash AppB？

答：为了降低升级失败风险。App 运行时只负责接收、写 staging 和校验，不直接修改内部 AppB。复位后 Bootloader 在受控状态下校验 pending 和 staging，再擦写 AppB。传输中断、CRC 错误、文件错误都不会影响 AppA。

### Q2：CRC 校验做了几层？

答：至少四层。App 接收时做流式 CRC；写入 W25Q 后读回再做 CRC；解析 AppB tag 后校验 App 本体 CRC；Boot 安装前再次复算 staging CRC，安装后还会读回内部 Flash 校验。

### Q3：为什么 USB CDC 不继续输出全量日志？

答：升级通道应该是协议通道，不应该承载全系统日志。全量日志可能造成 CDC busy 或阻塞，也会干扰 PC 工具解析。COM3 负责诊断日志，COM4 只返回升级协议需要的短响应，职责更清晰。

### Q4：为什么 Boot 跳 App 要清理 BASEPRI？

答：Boot 和 App 共享 Cortex-M 内核状态。`BASEPRI` 非零会屏蔽一部分中断，可能导致 SysTick、RTOS tick 或外设中断不工作。跳转前必须清理 `BASEPRI/FAULTMASK/CONTROL`，设置 App 的 `VTOR/MSP`，并清理 SysTick 和 NVIC pending。

### Q5：`taskENTER_CRITICAL()` 和互斥锁有什么区别？

答：互斥锁用于任务之间保护共享资源，会触发任务阻塞和调度；`taskENTER_CRITICAL()` 是临界区，会提高中断屏蔽阈值，阻止一部分中断和调度切换，适合非常短的内核关键段。它不是普通业务互斥锁，不能包住长耗时操作。

### Q6：为什么 SD 修复不用 `taskENTER_CRITICAL()`？

答：SD 读块可能持续较长时间，使用 `taskENTER_CRITICAL()` 会屏蔽部分中断，影响系统时基和外设响应。这里用 `vTaskSuspendAll()` 只暂停任务切换，不关闭中断，HAL tick 和超时仍然正常，适合保护轮询读 FIFO 不被 GUI 任务抢占。

### Q7：为什么 `FR_DISK_ERR=1` 不能直接判断是格式化问题？

答：FatFs 的 `FR_DISK_ERR` 只是上层通用错误，表示底层 `disk_read/disk_ioctl` 失败。必须继续打印 HAL SD error、card state、sector、count。实际根因是 `SDMMC_ERROR_RX_OVERRUN=0x00000020`，不是 FAT32 格式或文件名问题。

### Q8：为什么只烧 Boot/AppA 后还是旧现象？

答：Bootloader 会按 W25Q boot state 选择 active slot。如果 boot state 是 confirmed AppB，只烧 Boot/AppA 后仍会启动旧 AppB。必须查看启动日志确认 `BOOT: selected slot=...` 和 AppB tag CRC，或者完整烧录 AppB。

## 13. 后续优化方向

当前方案已经满足功能闭环，但仍有可优化空间：

- SD 驱动后续可改为 DMA 或中断模式，减少对 CPU 轮询及时性的依赖。
- SD 速度可以在稳定基础上逐步从 1-bit/低速恢复到 4-bit，并做多卡兼容测试。
- IAP 包可加入签名或 SHA256，提升防篡改能力。
- USB CDC 工具可以增加自动复位后状态查询，确认 AppB confirmed。
- HMI 可增加更明确的状态刷新，例如区分历史失败状态、当前 staging 成功、等待复位安装。
- 可增加 boot state 清除或强制启动 AppA 的维护命令，方便调试不同槽位。
