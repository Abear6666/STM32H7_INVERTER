# STM32H743 USB CDC IAP 升级问题复盘

日期：2026-06-25

## 1. 项目背景

本项目基于 STM32H743，目标是在已有 Bootloader + AppA/AppB 双分区框架上，实现本地 IAP 升级能力。系统包含：

- Bootloader：负责校验升级包、写入 AppB、维护 trial/confirmed/rollback 状态。
- AppA/AppB：运行 LVGL HMI、FreeRTOS 任务、存储管理和本地 IAP 任务。
- 外部 W25Q256：作为升级 staging 区、pending 标记区和 boot state 状态区。
- 本地升级入口：USART1、USB CDC 虚拟串口、SD 卡文件升级。

本次问题聚焦 USB CDC 升级链路。设计目标是 PC 端通过 USB 虚拟 COM 口发送：

```text
iap recv <size> <crc32> <version>
```

板端收到后擦除 W25Q256 staging 区，回复 `ready for binary`，然后接收固定长度二进制包，完成流式 CRC、读回 CRC、AppB 包头校验，最后写入 pending 标记。复位后 Bootloader 读取 pending 标记，将 staging 包安装到内部 Flash 的 AppB 分区，并进入 trial/confirmed 流程。

## 2. 故障现象

调试时遇到的现象不是单一 bug，而是多个问题叠加：

1. Windows 设备管理器中出现 `USB 串行设备 (COM4)`，但状态为 Code 10，USB CDC 虚拟串口不能正常使用。
2. 烧录后怀疑单片机程序没有正常运行，App 侧日志不稳定，USB 升级工具无法进入稳定传输流程。
3. COM4 后来可以枚举，但 PC 端 IAP 工具等待板端响应超时。
4. 打开 COM4 后，如果把全部 UART printf 日志镜像到 USB CDC，固件容易卡在 USB 发送路径，影响正常任务调度。
5. 最终表现为“USB 已接上，但升级没有成功”，需要从 USB 枚举、Boot 跳转、RTOS 临界区、CDC 发送策略、IAP 协议回包几个层面同时排查。

## 3. 排查思路

这类问题不能只看一个模块，因为 USB IAP 链路跨越了主机枚举、USB 描述符、底层 FIFO、Boot 到 App 跳转、FreeRTOS 启动时序、串口协议和外部 Flash 写入验证。

实际排查顺序如下：

1. 先确认 PC 端看到的串口来源。
   - `COM3` 是板载 CH340，对应 USART1 调试日志。
   - `COM4` 是 MCU USB CDC，VID/PID 为 ST 常见 CDC 设备 `0483:5740`。
2. 通过 OpenOCD/ST-LINK 重新烧录 Boot、AppA、AppB，排除旧固件干扰。
3. 观察 COM3 日志，确认 Boot 是否跳转到 App，App 内 FreeRTOS 任务是否周期运行。
4. 检查 Windows 枚举状态和 COM4 是否能打开。
5. 用 PowerShell 直接打开 COM4 发送 `iap status\n`，验证是否有板端回包。
6. 再运行 PC 端 IAP 工具，验证完整二进制传输、板端校验、pending 标记和复位后的 Boot 安装流程。

## 4. 根因分析

### 4.1 USB CDC 描述符不完整，导致 Windows Code 10

USB CDC ACM 设备包含控制接口和数据接口。Windows 对 CDC 组合接口比较敏感，如果设备描述符、配置描述符和 IAD 描述不匹配，可能会枚举出虚拟串口但驱动无法正常启动，表现为 Code 10。

本工程原来的 CDC 配置没有正确声明 composite/IAD 语义，配置描述符长度也没有包含新增 IAD 的 8 字节。修复点包括：

- Device Descriptor 使用 composite with IAD：

```c
/* App/Core/Src/usbd_desc.c */
0xEF, /* bDeviceClass: Miscellaneous */
0x02, /* bDeviceSubClass: Common Class */
0x01, /* bDeviceProtocol: Interface Association Descriptor */
```

- CDC 配置描述符长度从 67 改为 75：

```c
/* Middlewares/STM32_USB_Device_Library/Class/CDC/Inc/usbd_cdc.h */
#define USB_CDC_CONFIG_DESC_SIZ 75U
```

- 在 CDC control interface 前插入 IAD：

```c
/* Middlewares/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c */
0x08,              /* bLength */
USB_DESC_TYPE_IAD, /* bDescriptorType */
0x00,              /* bFirstInterface */
0x02,              /* bInterfaceCount */
0x02,              /* bFunctionClass: CDC */
0x02,              /* bFunctionSubClass: ACM */
0x01,              /* bFunctionProtocol */
0x00,              /* iFunction */
```

修改后 Windows 能稳定枚举出 MCU USB CDC 虚拟串口 COM4。

### 4.2 USB Tx FIFO 分配不足或端点不完整

CDC ACM 一般至少涉及：

- EP0：控制端点。
- CDC data IN/OUT 端点。
- CDC command interrupt IN 端点。

如果只配置部分 Tx FIFO，某些 IN 端点发送时会异常或卡住。修复后为 EP0、EP1、EP2 分别配置 Tx FIFO：

```c
/* App/Core/Src/usbd_conf.c */
HAL_PCDEx_SetRxFiFo(&hpcd_USB_FS, 0x80U);
HAL_PCDEx_SetTxFiFo(&hpcd_USB_FS, 0U, 0x40U);
HAL_PCDEx_SetTxFiFo(&hpcd_USB_FS, 1U, 0x40U);
HAL_PCDEx_SetTxFiFo(&hpcd_USB_FS, 2U, 0x20U);
```

这一步解决的是 USB 底层端点发送资源问题，是 CDC 后续能稳定发送回包的前提。

### 4.3 Boot 跳转到 App 前没有清理 CPU 中断屏蔽状态

Bootloader 跳转 App 时，不能只设置 MSP 和 VTOR，还要清理会影响 App 启动的内核寄存器状态。

现场发现 App 进入后 `BASEPRI=0x50`，导致 SysTick 或部分中断被屏蔽，App 在 `HAL_Delay()` 或启动阶段表现为卡住。根因是 Boot 到 App 的执行上下文没有清理干净。

修复点：

```c
/* Boot/Core/Src/boot_jump.c */
SCB->VTOR = app_run_addr;
__DSB();
__ISB();
__set_BASEPRI(0U);
__set_FAULTMASK(0U);
__set_CONTROL(0U);
__ISB();
__set_MSP(app_msp);
__enable_irq();
```

这里的关键点是：

- `BASEPRI` 必须清零，否则 FreeRTOS/SysTick/HAL tick 可能不工作。
- `FAULTMASK` 清零，避免异常被屏蔽。
- `CONTROL` 清零，让 App 从预期的 privileged MSP 状态开始。
- `VTOR` 指向 App 向量表，`MSP` 使用 App 初始栈。

### 4.4 App 早期再次清理 BASEPRI

为了提高鲁棒性，在 App `main()` 启动早期也清理一次 `BASEPRI`，避免 App 早期初始化受到 Boot 残留状态影响。

```c
/* App/Core/Src/main.c */
SCB->VTOR = (uint32_t)&__ram_vector_start__;
__DSB();
__ISB();
__set_BASEPRI(0U);
```

这属于防御式修复。即使 Boot 侧未来改动导致寄存器状态异常，App 也尽量把启动上下文拉回可控状态。

### 4.5 FreeRTOS 调度器启动前误用 taskENTER_CRITICAL

部分模块在 FreeRTOS 调度器启动前会访问共享状态。如果直接使用 `taskENTER_CRITICAL()`，在调度器未启动阶段可能改变 `BASEPRI`，并把系统带入中断屏蔽状态。

为此新增了统一封装：

```c
/* App/Core/Inc/app_critical.h */
uint32_t app_critical_enter(void);
void app_critical_exit(uint32_t primask);
```

实现逻辑：

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

修改策略：

- 调度器启动前：使用 `PRIMASK` 关中断并恢复原状态。
- 调度器启动后：使用 FreeRTOS 的 `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()`。

涉及模块：

- `App/Core/Src/app_log.c`
- `App/Core/Src/app_storage.c`
- `App/Core/Src/app_iap.c`
- `CMakeLists.txt` 增加 `App/Core/Src/app_critical.c`

这解决了 App 启动阶段临界区和 FreeRTOS 临界区混用带来的隐性启动问题。

### 4.6 全量 printf 镜像到 USB CDC 导致发送路径阻塞

最初为了让 COM4 也看到日志，把 `_write()` 中的 UART printf 同步镜像到了 USB CDC。这个设计在 bring-up 阶段看起来方便，但实际存在风险：

- printf 日志量大，USB CDC 发送可能进入 busy 状态。
- 如果在任务上下文或初始化阶段阻塞等待 CDC 发送完成，会影响系统调度。
- 打开 COM4 后，固件曾卡在 `USBD_CDC_TransmitPacket()` 相关发送路径。

因此修复策略不是继续增强日志镜像，而是取消全量镜像，让 `_write()` 只走 USART1 调试串口：

```c
/* App/Core/Src/uart.c */
int _write(int file, char *ptr, int len)
{
    ...
    if (HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY) != HAL_OK)
    {
        ...
    }
    ...
}
```

COM3 负责完整调试日志，COM4 只负责 IAP 协议交互。这是本次问题中很重要的设计取舍：升级协议链路需要稳定、短小、可预期的响应，而不是承载全系统日志。

### 4.7 USB IAP 协议缺少定向回包

取消全量日志镜像后，新的问题是 PC 端升级工具在 COM4 上等不到关键文本。原来 App 内部虽然 `printf("ready for binary")`，但该输出只在 COM3 UART 日志上，USB 工具无法收到。

因此新增 USB 源定向回包函数：

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

然后在 IAP 协议关键节点回包：

1. `iap status`：

```c
static void app_iap_print_status(app_iap_source_t source)
{
    ...
    printf("%s", line);
    app_iap_reply_source(source, line);
    ...
}
```

2. `iap help`：

```c
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
```

3. `iap recv` 参数错误、busy、Flash 不可用、size/version 非法、擦除失败等错误路径：

```c
app_iap_reply_source(source, "IAP USB: invalid version=0\r\n");
app_iap_reply_source(source, "IAP USB: erase staging failed\r\n");
```

4. 板端准备好接收二进制：

```c
printf("IAP %s: ready for binary size=%lu\r\n", app_iap_source_name(source), (unsigned long)size);
snprintf(line, sizeof(line), "IAP USB: ready for binary size=%lu\r\n", (unsigned long)size);
app_iap_reply_source(source, line);
```

5. 包接收、CRC、AppB tag 校验全部通过，pending 写入成功：

```c
printf("IAP %s: package verified, pending flag set version=%lu size=%lu crc=0x%08lX\r\n",
       app_iap_source_name(source),
       (unsigned long)pending.app_version,
       (unsigned long)pending.app_size,
       (unsigned long)pending.app_crc32);

snprintf(line,
         sizeof(line),
         "IAP USB: package verified, pending flag set version=%lu size=%lu crc=0x%08lX\r\n",
         (unsigned long)pending.app_version,
         (unsigned long)pending.app_size,
         (unsigned long)pending.app_crc32);
app_iap_reply_source(source, line);
```

6. 接收中止：

```c
static void app_iap_abort_recv(const char *reason)
{
    app_iap_source_t source = s_recv.source;
    const char *abort_reason = (reason != NULL) ? reason : "unknown";
    char line[96];

    printf("IAP %s: abort, %s\r\n", app_iap_source_name(source), abort_reason);
    snprintf(line, sizeof(line), "IAP USB: abort, %s\r\n", abort_reason);
    app_iap_reply_source(source, line);
    ...
}
```

这一步是协议层修复，解决了“板端其实准备好了，但 PC 端工具不知道”的问题。

## 5. 最终验证结果

### 5.1 编译

```powershell
cmake --build --preset gcc-debug
```

编译通过，生成 AppA/AppB slot 镜像。实测时 AppB 包：

```text
build/gcc-debug/app_b_slot.bin
size=323140
crc32=0xB55AEED7
version=2
```

### 5.2 烧录

```powershell
openocd -c "set DUAL_BANK 1" `
  -f interface/stlink.cfg `
  -f target/stm32h7x.cfg `
  -c "transport select swd" `
  -c "adapter speed 1000" `
  -c "program build/gcc-debug/apollo_h743_bootloader.hex verify" `
  -c "program build/gcc-debug/app_a_slot.hex verify" `
  -c "program build/gcc-debug/app_b_slot.hex verify reset exit"
```

OpenOCD 输出三段 `Verified OK`，说明 Boot、AppA、AppB 均烧录校验通过。

### 5.3 COM 口确认

Windows 当前能看到：

```text
COM3
COM4
```

其中：

- COM3：CH340 / USART1 调试串口。
- COM4：MCU USB CDC 虚拟串口。

### 5.4 USB CDC 状态命令验证

通过 PowerShell 打开 COM4，发送：

```text
iap status
```

板端返回：

```text
IAP status: flash=1 pending=0 version=0 size=0 crc=0x00000000 recv=0 0/0 state=IAP idle
IAP boot state: valid=1 state=confirmed active=1 trial=1 attempts=0/3 err=0
```

说明：

- USB CDC 枚举和收发正常。
- App IAP 任务正常运行。
- W25Q256 可用。
- Boot state 有效。

### 5.5 USB IAP 完整升级验证

执行：

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

关键输出：

```text
iap recv 323140 0xB55AEED7 2
IAP USB: ready for binary size=323140
sent=323140
IAP USB: package verified, pending flag set version=2 size=323140 crc=0xB55AEED7
发送完成: 323140/323140 100.0%
```

说明：

- PC 端发送命令后，板端能通过 COM4 返回 `ready for binary`。
- 二进制包完整发送。
- 板端完成流式 CRC、读回 CRC、AppB 包校验。
- pending 标记写入成功。

### 5.6 复位后 Boot 安装确认

复位命令：

```powershell
openocd -c "set DUAL_BANK 1" `
  -f interface/stlink.cfg `
  -f target/stm32h7x.cfg `
  -c "transport select swd" `
  -c "adapter speed 1000" `
  -c "init" `
  -c "reset run" `
  -c "shutdown"
```

复位后 COM3/COM4 状态：

```text
IAP status: flash=1 pending=0 version=0 size=0 crc=0x00000000 recv=0 0/0 state=AppB confirmed
IAP boot state: valid=1 state=confirmed active=1 trial=1 attempts=0/3 err=0
```

说明 Boot 已读取 pending，安装 AppB，并由 AppB 确认启动成功。

## 6. 本次修改文件清单

### USB 描述符和底层配置

- `App/Core/Src/usbd_desc.c`
  - 设备描述符改为 composite/IAD：`0xEF, 0x02, 0x01`。

- `Middlewares/STM32_USB_Device_Library/Class/CDC/Inc/usbd_cdc.h`
  - `USB_CDC_CONFIG_DESC_SIZ` 从 `67U` 改为 `75U`。

- `Middlewares/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c`
  - CDC 配置描述符增加 8 字节 IAD。

- `App/Core/Src/usbd_conf.c`
  - 为 EP0、EP1、EP2 分配 Tx FIFO。

### Boot 跳转和启动上下文

- `Boot/Core/Src/boot_jump.c`
  - 跳转 App 前清理 `BASEPRI`、`FAULTMASK`、`CONTROL`。

- `App/Core/Src/main.c`
  - App 早期清理 `BASEPRI`。

### 临界区修复

- `App/Core/Inc/app_critical.h`
- `App/Core/Src/app_critical.c`
- `CMakeLists.txt`
  - 新增调度器启动前后兼容的临界区封装。

- `App/Core/Src/app_log.c`
- `App/Core/Src/app_storage.c`
- `App/Core/Src/app_iap.c`
  - 替换启动敏感路径中的临界区操作。

### USB CDC 发送策略和 IAP 协议

- `App/Core/Src/uart.c`
  - 取消全量 printf 到 USB CDC 的镜像，避免 CDC 发送阻塞影响系统运行。

- `App/Core/Src/app_iap.c`
  - 新增 `app_iap_reply_source()`。
  - `iap status` / `iap help` 支持 USB 定向回包。
  - `iap recv` ready、成功、失败路径均支持 USB 协议回包。

## 7. 面试中可以强调的技术点

### 7.1 问题不是“USB 不通”，而是跨层状态机问题

可以这样讲：

> 这个问题表面是 Windows USB CDC Code 10 和升级超时，但我没有只盯着 USB 驱动，而是把链路拆成主机枚举、USB 描述符、端点 FIFO、Boot 跳转上下文、RTOS 启动临界区、CDC 发送策略、IAP 应用协议几个层面逐一验证。最后发现是多个问题叠加：CDC 描述符不完整导致枚举异常，Boot 跳转残留 BASEPRI 导致 App 启动异常，全量 printf 镜像到 USB 导致发送路径阻塞，协议层又缺少 USB 定向回包。

这能体现你不是只会改代码，而是能做系统级定位。

### 7.2 为什么不继续把所有日志打到 USB

面试官可能会问：既然 USB 工具需要看到日志，为什么不继续镜像 printf？

回答要点：

- IAP 升级通道应该是协议通道，不应该承载全系统日志。
- 全量日志不可控，容易造成 CDC busy 或阻塞。
- 调试日志保留在 USART1，USB CDC 只返回升级协议需要的短文本。
- 这样职责清晰，也降低升级过程被无关日志干扰的风险。

可以这样说：

> 我把 COM3 定义为诊断日志通道，COM4 定义为升级协议通道。升级工具只依赖 `ready for binary`、`package verified`、`abort reason` 这类短响应，而不是解析全系统日志。这能避免日志洪泛导致 CDC 发送拥塞，也让 PC 工具行为更确定。

### 7.3 为什么 Boot 跳转必须清理 BASEPRI

回答要点：

- Cortex-M 上 Boot 和 App 不是两个隔离进程，很多内核寄存器状态会继承。
- `BASEPRI` 如果非零，会屏蔽优先级低于阈值的中断。
- App 中 HAL tick、SysTick、FreeRTOS 调度都依赖中断。
- 跳转前要清理 NVIC、SysTick、VTOR、MSP、BASEPRI、FAULTMASK、CONTROL 等状态。

可以这样说：

> 这个问题暴露了 Bootloader 跳 App 不能只设置 MSP 和 Reset_Handler。Boot 的中断屏蔽状态会继承到 App，尤其是 BASEPRI。如果 BASEPRI 残留为 0x50，App 里的 SysTick 或 RTOS tick 就可能被屏蔽，表现为 HAL_Delay 卡住或任务不调度。修复后 Boot 跳转前清理 BASEPRI/FAULTMASK/CONTROL，App early init 再做一次防御性清理。

### 7.4 为什么要新增 app_critical

回答要点：

- FreeRTOS 临界区 API 依赖调度器状态。
- 调度器启动前使用 `taskENTER_CRITICAL()` 可能改变 `BASEPRI`，带来启动时序问题。
- 封装后，启动前走 `PRIMASK`，启动后走 FreeRTOS。

可以这样说：

> 我没有在每个模块里散落判断，而是抽了一个 `app_critical_enter/exit`。调度器没启动时保存并恢复 PRIMASK，调度器启动后再用 FreeRTOS critical section。这样既修复了启动阶段 BASEPRI 残留风险，也保持了任务运行期的 RTOS 语义。

## 8. 简历项目描述

可以放在项目经历中的版本：

> 负责 STM32H743 HMI 项目的 Bootloader + AppA/AppB 双分区 IAP 升级链路调试与闭环，实现基于 USB CDC 虚拟串口的本地固件升级。排查并修复 Windows USB CDC Code 10、Boot 跳转后 App 启动异常、FreeRTOS 启动前临界区误用、CDC 日志镜像阻塞和升级协议无回包等问题；完善 USB 描述符/IAD、端点 FIFO、Boot 跳转寄存器清理、IAP 协议定向响应和 CRC 校验流程。最终通过 PC 工具完成 323KB AppB 镜像传输、W25Q256 staging 写入校验、pending 标记写入、复位后 Boot 安装 AppB 并进入 confirmed 状态。

简历 bullet 版本：

- 设计并调试 STM32H743 Bootloader + AppA/AppB 双分区 IAP 流程，使用 W25Q256 作为 staging/pending/boot-state 存储，实现失败不破坏 AppA 的升级策略。
- 修复 USB CDC 在 Windows 下 Code 10 枚举异常，补齐 composite/IAD 描述符和 CDC 配置描述符长度，并完善 USB OTG FS 端点 FIFO 配置。
- 定位 Boot 跳转后 App 卡死问题，发现 `BASEPRI` 残留屏蔽 SysTick/RTOS tick，完善 Boot 跳转前 `BASEPRI/FAULTMASK/CONTROL` 清理和 App early init 防御性清理。
- 封装 FreeRTOS 启动前后兼容的临界区接口，避免调度器启动前误用 `taskENTER_CRITICAL()` 引入中断屏蔽状态异常。
- 将 USB CDC 从全量日志通道改为升级协议通道，增加 `ready for binary`、校验成功和 abort reason 等定向回包，避免 printf 镜像导致 CDC 发送阻塞。
- 完成实板验证：通过 COM4 USB CDC 传输 323KB AppB 镜像，板端完成流式 CRC、W25Q256 读回 CRC、AppB tag 校验和 pending 写入，复位后 Boot 安装 AppB 并确认 `AppB confirmed`。

## 9. STAR 面试讲述版

### Situation

项目需要在 STM32H743 HMI 系统上实现本地 IAP。系统已经有 Bootloader、AppA/AppB 双分区和 W25Q256 外部 Flash，目标是 PC 通过 USB CDC 虚拟串口发送 AppB 镜像，App 写入 staging 区，复位后由 Boot 安装到 AppB。

调试时 USB 接上后 Windows 出现 `USB 串行设备 (COM4)` Code 10，升级工具无法跑通；后续即使 COM4 能打开，也出现 App 启动异常、工具等待超时、打开 USB 后固件卡住等问题。

### Task

我的任务是把 USB CDC IAP 链路从“能编译但不可用”推进到实板可升级，并确保升级失败不会破坏 AppA，同时保留可诊断性，能解释每个故障点的根因。

### Action

我先把链路拆成几个层级验证：PC 枚举、USB 描述符、底层端点 FIFO、Boot 跳 App、App 任务调度、CDC 收发、IAP 协议和 W25Q staging 校验。

在 USB 枚举层，我修复了 CDC composite/IAD 描述符，把设备类改成 `0xEF/0x02/0x01`，配置描述符长度从 67 改为 75，并插入 IAD，解决 Windows Code 10。

在 USB 底层，我给 EP0、EP1、EP2 配置 Tx FIFO，保证 CDC 控制和数据 IN 端点有发送资源。

在启动链路上，我发现 Boot 跳转到 App 后 `BASEPRI` 残留，导致 SysTick/RTOS tick 被屏蔽，于是在 Boot 跳转前清理 `BASEPRI/FAULTMASK/CONTROL`，App 早期也做防御性 `BASEPRI` 清理。

在 RTOS 启动时序上，我封装了 `app_critical_enter/exit`，调度器启动前使用 PRIMASK 保存恢复，调度器启动后使用 FreeRTOS critical section，避免启动前误用 `taskENTER_CRITICAL()`。

在协议层，我取消了全量 printf 镜像到 USB CDC，因为它会造成 CDC 发送 busy 甚至卡住。然后为 IAP 增加 USB 定向回包，只返回 `ready for binary`、`package verified`、`abort` 和 `status` 这类协议文本，让 COM3 做日志，COM4 做升级协议。

### Result

最终实板验证通过。PC 端通过 COM4 发送 323140 字节 AppB 镜像，CRC 为 `0xB55AEED7`，板端返回：

```text
IAP USB: ready for binary size=323140
IAP USB: package verified, pending flag set version=2 size=323140 crc=0xB55AEED7
```

复位后 Boot 读取 pending，将 staging 包安装到 AppB，AppB 启动后确认成功，状态为：

```text
state=AppB confirmed
pending=0
flash=1
```

这个案例体现了我对嵌入式升级链路的完整理解：不仅能写 IAP 接收逻辑，还能处理 USB 枚举、Boot 跳转上下文、RTOS 启动时序、协议通道设计、外部 Flash 校验和实板闭环验证。

## 10. 面试官可能追问的问题

### Q1：为什么升级包先写 W25Q256，而不是 App 直接写内部 Flash AppB？

答：这是为了降低升级失败的风险。App 负责接收和校验包，只在外部 Flash staging 区写入；复位后由 Bootloader 在受控状态下擦写内部 AppB。这样传输中断、CRC 错误、包格式错误都不会影响内部 Flash 中已有可启动镜像。AppA 始终作为 fallback。

### Q2：CRC 校验做了几层？

答：至少三层。第一层是 PC 发送时的整包 CRC，App 边接收边计算流式 CRC；第二层是 App 从 W25Q256 staging 读回后再次计算 CRC，确认外部 Flash 写入正确；第三层是解析包内 `app_tag_t`，校验它确实是 AppB 镜像，并校验 App 本体 CRC。Boot 安装时还会再次校验 pending 和 staging。

### Q3：为什么 USB CDC 的 baud 参数还要传？

答：CDC 虚拟串口的实际传输不依赖 UART 波特率，但 PC 端串口 API 和 CDC ACM 协议仍有 line coding 概念。工具保留 `--baud 115200` 是为了接口统一，实际吞吐由 USB FS、固件处理速度和 chunk/delay 参数决定。

### Q4：为什么升级工具使用 `--no-wait-app`？

答：旧逻辑会等待全量 App 日志，例如 `task_iap started`。修复后 USB CDC 不再承载全量日志，只承载 IAP 协议响应，所以工具应跳过等待 App 日志，改为等待 `ready for binary` 这类协议回包。日志仍然从 COM3 查看。

### Q5：如果传输中途断开会怎样？

答：App 只有在接收满指定 size、流式 CRC 正确、W25Q256 读回 CRC 正确、AppB tag 校验正确后才写 pending。中途断开不会写有效 pending。复位后 Boot 看不到有效 pending，会按原 boot state 启动已有 App，不会破坏 AppA。

## 11. 复测命令

查看串口：

```powershell
[System.IO.Ports.SerialPort]::GetPortNames()
```

COM4 手工状态查询：

```text
iap status
```

USB IAP 升级：

```powershell
.\Tools\iap_serial_cli\bin\Release\net10.0-windows\ApolloIapSerialCli.exe `
  --port COM4 `
  --baud 115200 `
  --file build\gcc-debug\app_b_slot.bin `
  --version 2 `
  --chunk 128 `
  --delay-ms 2 `
  --no-wait-app
```

预期关键输出：

```text
IAP USB: ready for binary size=<size>
IAP USB: package verified, pending flag set version=<version> size=<size> crc=<crc>
```

复位后查询：

```text
iap status
```

预期：

```text
state=AppB confirmed
pending=0
```

