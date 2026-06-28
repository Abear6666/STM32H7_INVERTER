# Apollo H743 LVGL HMI 内存规划

阶段：第 0 阶段，已补充第 3 阶段 SDRAM 实板验证结果
日期：2026-05-25

这是第一版内存规划。当前规划偏保守，后续必须根据 Apollo H743 V2 官方 QSPI、LTDC 和 LVGL 例程继续修订。

第 3 阶段已经完成 SDRAM 初始化和 memtest。QSPI、LCD/LTDC、触摸和 LVGL 仍未接入。

## 资料状态

已找到硬件相关资料：

- 官方无屏幕验收工程：`D:\datafile\stm32\1，入门资料.zip` 内的 `atk_h743.uvprojx`。
- 官方 SDRAM BSP：`D:\datafile\stm32\1，入门资料.zip` 内的 `Drivers\BSP\SDRAM\sdram.c`。
- 官方 QSPI BSP：`D:\datafile\stm32\1，入门资料.zip` 内的 `Drivers\BSP\QSPI\qspi.c`。
- 官方 LCD/LTDC BSP 参考：`D:\datafile\stm32\1，入门资料.zip` 内的 `Drivers\BSP\LCD\lcd.c` 和 `ltdc.c`。
- Apollo H743 HAL 标准例程：`D:\datafile\stm32\4，程序源码\2，标准例程-HAL库版本.zip`。
- SDRAM 芯片资料：`D:\datafile\stm32\7，硬件资料.zip` 内的 `W9825G6KH.pdf`。
- QSPI Flash 芯片资料：`D:\datafile\stm32\7，硬件资料.zip` 内的 `W25Q256.pdf`。
- 4.3 寸 800x480 RGB 屏资料：`D:\datafile\stm32\7，硬件资料.zip` 和仓库通用 A 盘中均有线索。
- 核心板、底板和屏幕原理图：`D:\datafile\stm32\3，原理图.zip`。
- Apollo H743 LVGL 参考工程：`D:\datafile\stm32\4，程序源码\3，扩展例程\4，LVGL例程.zip`。
- LVGL v8.2 源码资料：`D:\datafile\stm32\14，LVGL学习资料\lvgl-master.zip`，实际内容为 `lvgl-release-v8.2`。
- LVGL 工具资料：`D:\datafile\stm32\14，LVGL学习资料\LVGL使用工具.zip`。

已找到的 LVGL 参考工程包括：

```text
4，LVGL例程/LVGL例程1 无操作系统移植/Projects/MDK-ARM/atk_h743.uvprojx
4，LVGL例程/LVGL例程2 操作系统移植/Projects/MDK-ARM/atk_h743.uvprojx
4，LVGL例程/LVGL例程49 二维码生成器(800x480)/Projects/MDK-ARM/atk_h743.uvprojx
4，LVGL例程/LVGL例程50 绘画系统(800x480)/Projects/MDK-ARM/atk_h743.uvprojx
4，LVGL例程/LVGL综合实验(800x448)/Projects/MDK-ARM/atk_h743.uvprojx
```

已找到的 HAL 关键参考工程包括：

```text
2，标准例程-HAL库版本/实验14 SDRAM实验/Projects/MDK-ARM/atk_h743.uvprojx
2，标准例程-HAL库版本/实验15 LTDC LCD（RGB屏）实验/Projects/MDK-ARM/atk_h743.uvprojx
2，标准例程-HAL库版本/实验28 QSPI实验/Projects/MDK-ARM/atk_h743.uvprojx
2，标准例程-HAL库版本/实验31 触摸屏实验/Projects/MDK-ARM/atk_h743.uvprojx
```

`LVGL例程1 无操作系统移植` 的 `lv_conf.h` 已抽查，确认：

```text
LV_COLOR_DEPTH = 16
LV_MEM_SIZE    = 46KB
```

46KB 是教学例程配置，不适合作为第一版 HMI 工程的最终堆大小。第一版内存规划仍按 128KB LVGL heap 起步。

仍缺会直接影响外设初始化最终落地的资料：

- 带屏幕版官方验收工程或完整 LCD 显示验收工程；当前已有 LVGL 参考例程，但仍需区分验收工程和教学例程。
- 实际 4.3 寸屏模块的触摸芯片型号和参数。

第 3 阶段已经确认 SDRAM 基地址、FMC Bank 和 SDRAM 时钟；QSPI 映射地址和显示缓冲布局仍按“候选规划”处理。后续阶段需要从官方 LVGL 无操作系统移植例程和 800x480 示例中继续复核 QSPI、LTDC、触摸和 LVGL 关键配置。

## 显示内存预算

目标屏幕：

```text
分辨率：800x480
颜色格式：RGB565，每像素 2 字节
单帧缓冲大小：800 * 480 * 2 = 768000 bytes = 750 KiB
```

750KiB 帧缓冲不适合放在 STM32H743 内部 SRAM 中，第一版规划放到外部 SDRAM。

## 内部 Flash 规划

`STM32H743IIT6` 内部 Flash 总容量为 2MB：

```text
0x08000000 - 0x080FFFFF   Flash Bank 1，1024KB
0x08100000 - 0x081FFFFF   Flash Bank 2，1024KB
```

第一版分区规划：

```text
0x08000000 - 0x0801FFFF   引导程序，128KB
0x08020000 - 0x080FFFFF   AppA，896KB
0x08100000 - 0x081FFFFF   AppB / 升级槽，1024KB
```

AppA 入口规划：

```text
APP_A_START    = 0x08020000
APP_A_RUN_ADDR = 0x08020400

0x08020000 - 0x080203BF   预留
0x080203C0 - 0x080203FF   app_tag_t，64B
0x08020400 - ...          App 向量表 + App 代码
```

第 0 阶段只做规划，不写引导程序或 App 代码。

## 内部 RAM 规划

STM32H743 常用 RAM 区域：

| 区域 | 地址 | 大小 | 第一版用途 |
|---|---:|---:|---|
| ITCM RAM | `0x00000000` | 64KB | 后续可放关键代码 |
| DTCM RAM | `0x20000000` | 128KB | MSP 栈、中断关键数据、可选向量表副本 |
| AXI SRAM | `0x24000000` | 512KB | FreeRTOS 堆、LVGL 绘制缓冲区、普通全局变量 |
| SRAM1/2/3 | `0x30000000` 起 | 约 288KB | DMA 外设缓冲、UART/SPI/QSPI 临时缓冲 |
| SRAM4 | `0x38000000` | 64KB | 低功耗或保留运行数据 |
| Backup SRAM | `0x38800000` | 4KB | 复位原因、Boot/升级状态、标志 |

第一版分配策略：

```text
DTCM：
  MSP 栈
  fault/debug 上下文
  少量关键状态变量

AXI SRAM：
  FreeRTOS heap
  LVGL heap
  LVGL 绘制缓冲区 1
  LVGL 绘制缓冲区 2
  普通全局/静态变量

SRAM1/2/3：
  UART RX ring buffer
  SPI/QSPI 临时缓冲
  不适合放 DTCM 的 DMA buffer

SDRAM：
  LCD 帧缓冲
  GUI 图片解码/缓存区
  后续可选大 LVGL 缓冲区
```

## 外部 SDRAM 规划

已确认硬件：`W9825G6KH`，32MB SDRAM。

第 3 阶段已经通过 Apollo H743 V2 实板验证：

```text
SDRAM_BASE = 0xC0000000
SDRAM_SIZE = 32MB
FMC SDRAM Bank = FMC_SDRAM_BANK1
数据宽度 = 16 bit
列地址 = 9 bit
行地址 = 13 bit
内部 Bank = 4
CAS Latency = 2
Read Burst = Enable
Read Pipe Delay = 1
FMC kernel clock = 220MHz
SDRAM SDCLK = 110MHz
刷新计数 = 839
```

第 3 阶段 memtest 结果：

```text
32MB 全容量 walking bit / 0x55555555 / 0xAAAAAAAA / 递增地址模式测试通过。
连续 reset run 10 次均通过。
```

第一版 SDRAM 布局如下：

```text
0xC0000000 - 0xC00BB7FF   LCD 帧缓冲 0，750KiB
0xC00BB800 - 0xC0176FFF   LCD 帧缓冲 1，后续可选双缓冲，750KiB
0xC0177000 - 0xC01FFFFF   LVGL / DMA2D 临时区
0xC0200000 - 0xC07FFFFF   GUI 图片解码缓存 / 资源缓存
0xC0800000 - 0xC1FFFFFF   可选外部 heap / 文件缓存 / 预留
```

第一版显示策略：

```text
单 SDRAM 帧缓冲 + LVGL 局部绘制缓冲区
LTDC 持续扫描 SDRAM 帧缓冲
LVGL 刷新回调将脏区域拷贝到帧缓冲
```

当前 MPU / Cache 策略：

```text
SDRAM 32MB 区域配置为 non-cacheable、non-bufferable、execute-never。
ICache 开启。
DCache 关闭。
```

如果后续确认 SDRAM 带宽、DCache、MPU 和 DMA2D 配置稳定，再考虑打开 DCache、双帧缓冲或更大的 LVGL 缓冲。

## LVGL RAM 预算

第一版建议：

```text
LV_COLOR_DEPTH = 16
绘制缓冲区高度 = 40 行
单绘制缓冲区 = 800 * 40 * 2 = 64000 bytes，约 62.5KiB
双绘制缓冲区 = 128000 bytes，约 125KiB
LVGL heap 第一版目标 = 128KiB
```

和官方 LVGL 教学例程的关系：

```text
LVGL例程1 无操作系统移植：LV_COLOR_DEPTH = 16，LV_MEM_SIZE = 46KB
本项目第一版 HMI 规划：LV_COLOR_DEPTH = 16，LVGL heap = 128KB
```

原因：教学例程主要用于验证移植链路，界面对象和资源压力较小；本项目目标是 HMI 工程，后续会有更多控件、字体、图片和页面状态，第一版堆空间按 128KB 更稳妥。最终大小需要在真实 UI 加载后通过 LVGL 内存监控再收敛。

第一版推荐放置：

```text
LVGL 绘制缓冲区：优先 AXI SRAM；如果 AXI SRAM 压力过大，再考虑 SDRAM
LVGL heap：第一版优先 AXI SRAM
帧缓冲：只放 SDRAM
```

## 外部 QSPI Flash 规划

已确认硬件：`W25Q256`，32MB。

第一版逻辑分区：

```text
0x00000000 - 0x0000FFFF   参数区 Workset，64KB
0x00010000 - 0x0001FFFF   统计区 Statistic，64KB
0x00020000 - 0x001FFFFF   Boot 图片 / 开机图资源，约 1.875MB
0x00200000 - 0x00DFFFFF   GUI 资源主区，12MB
0x00E00000 - 0x019FFFFF   GUI 资源升级/暂存区，12MB
0x01A00000 - 0x01FFFFFF   App 升级包 / 日志 / 预留，6MB
```

QSPI 内存映射候选地址：

```text
0x90000000
```

在 Apollo H743 V2 W25Q256 QSPI 例程确认前，不依赖内存映射模式。第一版优先按普通命令模式读、擦、写来规划。

## 待确认项

- 从 `LVGL例程1 无操作系统移植` 只读复核 LTDC、SDRAM、触摸和 LVGL 初始化顺序。
- 从 800x480 示例只读复核分辨率相关配置，注意 `LVGL综合实验(800x448)` 不是目标全屏分辨率。
- 确认 DCache 开启时帧缓冲是否需要通过 MPU 设置为不可缓存，或使用显式 Cache clean/invalidate。
- 确认 LTDC 读取 SDRAM 帧缓冲时的带宽余量。
- 确认 QSPI 内存映射模式是否适用于 W25Q256，而不是只适用于 W25Q64 示例脚本。
- 确认最终工具链后，再决定链接脚本或分散加载文件。

## Phase 4 实际使用记录

第 4 阶段已经接入 LTDC 裸 framebuffer，但仍未接入 LVGL。

当前实际使用：

```text
LCD_WIDTH = 800
LCD_HEIGHT = 480
LCD_FORMAT = RGB565
LCD_FB0 = 0xC0000000
LCD_FB0_SIZE = 800 * 480 * 2 = 768000 bytes = 750KiB
LCD_FB0_RANGE = 0xC0000000 - 0xC00BB7FF
```

当前保守布局：

```text
0xC0000000 - 0xC00BB7FF   Phase 4 实际 LTDC framebuffer，750KiB
0xC00BB800 - 0xC0176FFF   预留 framebuffer 1，后续可选双缓冲，750KiB
0xC0177000 - 0xC01FFFFF   预留 LVGL / DMA2D 临时区
0xC0200000 - 0xC07FFFFF   预留 GUI 图片解码缓存 / 资源缓存
0xC0800000 - 0xC1FFFFFF   预留外部 heap / 文件缓存 / 其它用途
```

当前 MPU / Cache 策略不变：

```text
SDRAM 32MB：non-cacheable、non-bufferable、execute-never
ICache：开启
DCache：关闭
```

说明：Phase 4 先用最保守的 cache 策略验证 LTDC 扫描 SDRAM framebuffer。等 LCD 显示稳定并进入 LVGL/DMA2D 阶段后，再评估 DCache、MPU 子区域或显式 cache clean/invalidate。

## Phase 6 实际内存使用记录

Phase 6 已接入 LVGL 8.4.0，当前仍不使用 FreeRTOS，不启用 DMA2D，不启用 DCache。

实际显示内存：

```text
SDRAM framebuffer:
  base = 0xC0000000
  size = 800 * 480 * 2 = 768000 bytes = 750 KiB
  format = RGB565

LVGL draw buffer:
  pixels = 800 * 40
  size = 64000 bytes
  location = RAM_D1 .bss
  mode = single buffer

LVGL heap:
  LV_MEM_SIZE = 128 KiB
  location = RAM_D1 .bss
```

当前 GCC Debug 构建内存统计：

```text
FLASH used = 232100 bytes / 2048 KiB
RAM_D1 used = 203320 bytes / 512 KiB
DTCMRAM used = 0
RAM_D2 used = 0
RAM_D3 used = 0
```

当前 RAM_D1 余量仍足够继续做第一版 HMI bring-up。后续如果启用 FreeRTOS、DCache、DMA2D 或更大字体/图片资源，需要重新评估：

- LVGL heap 是否继续放 RAM_D1。
- draw buffer 是否迁移到 SDRAM。
- framebuffer 是否需要双缓冲。
- SDRAM MPU/cache 策略是否从 non-cacheable 调整为 cacheable + 显式 cache clean。

## Phase 7-9 实际内存使用记录

Phase 7-9 已接入 FreeRTOS、W25Q256 参数保存和四页面 HMI。当前仍不启用 DMA2D，不启用 DCache，framebuffer 仍为 SDRAM 单缓冲。

内部 RAM 主要新增项：

```text
FreeRTOS heap:
  configTOTAL_HEAP_SIZE = 96 KiB
  location = RAM_D1 .bss

task_gui:
  stack = 3072 words = 12 KiB
  priority = tskIDLE_PRIORITY + 3

task_storage:
  stack = 1024 words = 4 KiB
  priority = tskIDLE_PRIORITY + 1

task_log:
  stack = 1024 words = 4 KiB
  priority = tskIDLE_PRIORITY + 1

LVGL draw buffer:
  pixels = 800 * 40
  size = 64000 bytes
  location = RAM_D1 .bss

LVGL heap:
  LV_MEM_SIZE = 128 KiB
  location = RAM_D1 .bss
```

外部 W25Q256 当前实际使用：

```text
0x00000000 - 0x00000FFF   Workset 参数块实际写入 sector，4KB
0x00001000 - 0x0000FFFF   Workset 参数区预留，60KB
0x00010000 - 0x0001FFFF   统计区预留，64KB
0x00020000 - 0x001FFFFF   Boot 图片 / 开机图资源预留
0x00200000 - 0x00DFFFFF   GUI 资源主区预留
0x00E00000 - 0x019FFFFF   GUI 资源升级/暂存区预留
0x01A00000 - 0x01FFFFFF   App 升级包 / 日志 / 预留
```

参数块结构：

```text
magic   4 bytes
version 4 bytes
length  4 bytes
crc32   4 bytes
payload 512 bytes
total   528 bytes
```

当前 GCC Debug 构建内存统计：

```text
FLASH used = 255848 bytes / 2048 KiB
RAM_D1 used = 303960 bytes / 512 KiB
DTCMRAM used = 0
RAM_D2 used = 0
RAM_D3 used = 0
```

实测任务栈余量：

```text
task_gui     high water mark ~= 2486 words
task_storage high water mark ~= 904 words
task_log     high water mark ~= 839 words
idle         high water mark ~= 104 words
FreeRTOS heap free ~= 76760 bytes
```

当前 RAM_D1 占用约 58%。后续进入 Boot/IAP、资源解码、DMA2D 或更复杂页面前，需要继续跟踪 FreeRTOS heap、LVGL heap 和 GUI 任务栈。

## Phase 10-11 Boot/App 分区实装

日期：2026-05-27

内部 Flash 采用固定 Boot + AppA + AppB 规划，第一版不做 Option Byte Bank Swap。

```text
0x08000000 - 0x0801FFFF   Bootloader，128KB
0x08020000 - 0x080FFFFF   AppA 槽，896KB
0x08100000 - 0x081FFFFF   AppB 槽 / 后续内部 Flash 升级槽，1024KB
```

App 槽头固定为 0x400 字节，App 真正链接和运行地址为 `AppStart + 0x400`。

```text
AppA:
0x08020000 - 0x080203BF   槽头预留
0x080203C0 - 0x080203FF   app_tag_t，64 字节
0x08020400 - ...          AppA 向量表和代码，链接地址 0x08020400

AppB:
0x08100000 - 0x081003BF   槽头预留
0x081003C0 - 0x081003FF   app_tag_t，64 字节
0x08100400 - ...          AppB 后续运行/复制目标预留
```

`app_tag_t` 第一版字段：

```text
magic        0x41505447
tag_size     64
app_run_addr App 运行地址，例如 0x08020400
app_size     App bin 实际长度
version      App 版本号
crc32        对 App bin 本体计算 CRC32，不包含 0x400 槽头
sha256       预留，第一版全 0
reserved     预留
```

AppA 链接脚本：

```text
FLASH ORIGIN = 0x08020400
FLASH LENGTH = 0x000DFC00
.ram_vector  = RAM_D1 起始处，大小 0x400
SCB->VTOR    = RAM 向量表地址
```

当前 GCC Debug 构建结果：

```text
Bootloader FLASH used = 39460 bytes / 128 KiB
AppA FLASH used       = 263304 bytes / 0x000DFC00
AppA RAM_D1 used      = 306200 bytes / 512 KiB
AppA slot image       = 264328 bytes
AppA CRC32            = 0xA6F05A8E
```

构建产物：

```text
build/gcc-debug/apollo_h743_bootloader.hex  烧录到 0x08000000
build/gcc-debug/apollo_h743_app.hex         App 裸工程 hex，链接地址 0x08020400
build/gcc-debug/app_a_slot.hex              AppA 槽镜像，包含 0x400 槽头和 app_tag_t
build/gcc-debug/app_a_tag.bin               单独 app_tag_t
```

外部 W25Q256 IAP 第一版 staging 规划：

```text
0x01A00000 - 0x01A00FFF   IAP pending 记录，包含 magic/version/length/crc32
0x01A01000 - 0x01A01FFF   IAP boot state 记录，保存 trial/confirmed/rollback
0x01A02000 - 0x01FFFFFF   本地 IAP staging 区，串口/USB/SD 后端共用
```

更新后的 Phase 11 流程会把有效 AppB slot 包从 W25Q256 staging 写入内部 Flash AppB 槽，不覆盖 AppA。Boot 启动时先处理 pending；安装成功后写入 trial 状态并试启动 AppB。AppB 成功启动后写入 confirmed；后续复位 Boot 只在 confirmed/trial 状态下启动 AppB，否则回退校验并跳转 AppA。

当前构建产物：

```text
build/gcc-debug/apollo_h743_bootloader.hex  Bootloader，烧录到 0x08000000
build/gcc-debug/app_a_slot.hex              AppA 初始槽镜像，烧录到 0x08020000
build/gcc-debug/app_b_slot.bin              串口/GUI IAP 推荐升级包，写入 W25Q staging 后由 Boot 安装到 AppB
build/gcc-debug/app_b_slot.hex              AppB 槽镜像，可用于调试时直接烧录到 0x08100000
```

2026-06-12 Phase 11 USB CDC 升级接入后 GCC Debug 当前静态包校验：

```text
Bootloader FLASH used = 47308 bytes / 128 KiB
AppA slot image       = 296352 bytes
AppA run address      = 0x08020400
AppA version          = 1
AppA body CRC32       = 0xDB2AAED3
AppA slot package CRC = 0x42E672CB
AppB slot image       = 296984 bytes
AppB run address      = 0x08100400
AppB version          = 2
AppB body CRC32       = 0x078FC745
AppB slot package CRC = 0xE4D0DA19
```

USB CDC 说明：

```text
USB CDC 仍只作为 App 侧本地 IAP 输入源，不改变 Boot/AppA/AppB 内部 Flash 分区。
USB CDC 接收到的 app_b_slot.bin 先写入 W25Q staging，再由 Boot 安装到 AppB。
Bootloader 本阶段不接入 USB。
```

2026-06-13 Phase 11 SD 卡本地 IAP 接入后 GCC Debug 当前静态包校验：

```text
Bootloader FLASH used = 47336 bytes / 128 KiB
AppA slot image       = 318652 bytes
AppA run address      = 0x08020400
AppA version          = 1
AppA body CRC32       = 0x33F40418
AppA slot package CRC = 0x3E501EFC
AppB slot image       = 319284 bytes
AppB run address      = 0x08100400
AppB version          = 2
AppB body CRC32       = 0x98F10D5A
AppB slot package CRC = 0x859E2082
```

SD 卡本地 IAP 不改变内部 Flash 和 W25Q256 分区：

```text
SD 输入文件：0:/app_b_slot.bin
SD 文件来源：build/gcc-debug/app_b_slot.bin 复制到 SD 卡根目录
SD 传输路径：SD 卡 -> FatFs -> W25Q staging 0x01A02000..0x01FFFFFF -> Boot 安装到 AppB
状态记录：仍使用 W25Q pending 0x01A00000 和 boot state 0x01A01000
```

## 2026-06-13 系统诊断内存补充

AppA/AppB 新增 DTCM `.noinit` 诊断快照区：

```text
DTCM 0x20000000 起始处
.noinit.dtcm
当前占用：96 bytes
用途：App 复位计数、最近一次 Fault 栈帧、SCB Fault 寄存器快照
```

放在 DTCM 的原因：

```text
1. DTCM 不需要额外开启 SRAM4 时钟，Fault handler 中可以直接写入。
2. Bootloader 当前链接脚本不使用 DTCM，App Fault 后 NVIC_SystemReset() 进入 Boot，再跳回 App 时不会被 Boot 的 .bss 清零。
3. 该记录只用于软件复位后的故障定位，不作为断电保持数据；POR/BOR 冷启动时 App 会清空旧快照。
4. 不写 W25Q，避免诊断功能引入额外擦写次数。
```

当前 GCC Debug 构建内存统计：

```text
Bootloader FLASH used = 47336 bytes / 128 KiB
Bootloader DTCMRAM    = 0 bytes
Bootloader RAM_D1     = 8456 bytes

AppA FLASH used       = 320280 bytes / 895 KiB
AppA DTCMRAM used     = 96 bytes / 128 KiB
AppA RAM_D1 used      = 316640 bytes / 512 KiB

AppB FLASH used       = 320928 bytes / 1023 KiB
AppB DTCMRAM used     = 96 bytes / 128 KiB
AppB RAM_D1 used      = 316640 bytes / 512 KiB
```

## 2026-06-27 IAP 回归后当前构建产物

当前 GCC Debug 构建产物：

```text
build/gcc-debug/apollo_h743_bootloader.hex  Bootloader，烧录到 0x08000000
build/gcc-debug/app_a_slot.hex              AppA 槽镜像，烧录到 0x08020000
build/gcc-debug/app_b_slot.hex              AppB 槽镜像，烧录到 0x08100000
build/gcc-debug/app_b_slot.bin              USB CDC / SD 卡 IAP 推荐升级包
```

当前静态尺寸：

```text
Bootloader text+data = 47372 bytes
AppA text+data       = 322900 bytes
AppB text+data       = 323532 bytes
AppA slot image      = 323932 bytes
AppB slot image      = 324564 bytes
```

当前 AppB 包实板验证信息：

```text
AppB run address      = 0x08100400
AppB version          = 2
AppB body CRC32       = 0xBD293B71
AppB slot package CRC = 0xAF740721
AppB slot package size= 324564 bytes
```

2026-06-27 已验证路径：

```text
完整烧录：Bootloader + AppA + AppB，OpenOCD verify OK
SD IAP：0:/app_b_slot.bin -> W25Q staging -> pending -> Boot 安装 AppB -> AppB confirmed
USB IAP：COM4 USB CDC -> W25Q staging -> pending -> Boot 安装 AppB -> AppB confirmed
```
