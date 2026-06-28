# Apollo H743 LVGL HMI 硬件确认表

阶段：第 0 阶段
日期：2026-05-25
目标硬件：正点原子 Apollo STM32H743 V2 开发板 + 4.3 寸 RGB 屏，分辨率 800x480

## 原则

本项目从 0 开始建立，不假设已有旧工程源码，也不复制其它工程作为基础。

本阶段只做资料确认、硬件确认表和第一版内存规划，不写 SDRAM、LTDC、触摸、QSPI 或 LVGL 驱动代码。

## 已找到资料

Apollo H743 V2 专用文档：

- `D:\datafile\stm32\STM32H743 阿波罗开发指南V1.3.pdf`
- `D:\datafile\stm32\阿波罗V2 硬件参考手册_V1.1（H743版本）.pdf`
- `D:\datafile\stm32\1，入门资料.zip`
- `D:\datafile\stm32\4，程序源码`
- `D:\datafile\stm32\14，LVGL学习资料`
- `C:\Users\Luo\Desktop\getnewjob\docs\STM32H743 阿波罗开发指南V1.0.pdf`
- `C:\Users\Luo\Desktop\getnewjob\docs\阿波罗V2 硬件参考手册_V1.0（H743版本）.pdf`
- `D:\datafile\stm32\实验0-4，新建工程实验-CubeMX版本\Template.ioc`

`1，入门资料.zip` 中已确认存在官方无屏幕验收工程：

- `1，入门资料\验收所需资料（无屏幕版）\1，验收工程（无屏幕版）\Projects\MDK-ARM\atk_h743.uvprojx`
- `Drivers\BSP\SDRAM\sdram.c`
- `Drivers\BSP\QSPI\qspi.c`
- `Drivers\BSP\LCD\lcd.c`
- `Drivers\BSP\LCD\ltdc.c`
- `Drivers\SYSTEM\usart\usart.c`

原理图压缩包：

- `D:\datafile\stm32\3，原理图.zip`
- 核心板：`STM32H743_CORE V2.0.pdf`
- 底板：`DNF429P_F767P_H743P V2.6.pdf`
- 屏幕：`ATK-MD0430R V1.9.pdf`、`ATK-MD0430 V1.7.pdf`
- IO 表：`阿波罗V2 STM32H743 开发板 IO 口功能表.xlsx`

硬件资料压缩包：

- `D:\datafile\stm32\7，硬件资料.zip`
- MCU：`STM32H743IIT6.pdf`
- SDRAM：`W9825G6KH.pdf`
- QSPI Flash：`W25Q256.pdf`
- LCD：4.3 寸 RGB4342/RGB4384、4.3 寸 800x480 屏、ST7262 控制 IC 资料
- 触摸：GT911、GT9147、GT917S、GT1151Q 等数据手册和编程指南

程序源码资料：

- `D:\datafile\stm32\4，程序源码\1，标准例程-寄存器版本.zip`
- `D:\datafile\stm32\4，程序源码\2，标准例程-HAL库版本.zip`
- `D:\datafile\stm32\4，程序源码\3，扩展例程\4，LVGL例程.zip`
- `D:\datafile\stm32\4，程序源码\4，STM32启动文件.zip`

`2，标准例程-HAL库版本.zip` 中已确认的 Apollo H743 关键参考工程：

- `2，标准例程-HAL库版本\实验0 基础入门实验\实验0-3，新建工程实验-HAL库版本\Projects\MDK-ARM\atk_h743.uvprojx`
- `2，标准例程-HAL库版本\实验14 SDRAM实验\Projects\MDK-ARM\atk_h743.uvprojx`
- `2，标准例程-HAL库版本\实验15 LTDC LCD（RGB屏）实验\Projects\MDK-ARM\atk_h743.uvprojx`
- `2，标准例程-HAL库版本\实验28 QSPI实验\Projects\MDK-ARM\atk_h743.uvprojx`
- `2，标准例程-HAL库版本\实验31 触摸屏实验\Projects\MDK-ARM\atk_h743.uvprojx`

`4，LVGL例程.zip` 中已确认的 Apollo H743 LVGL 参考工程：

- `4，LVGL例程\LVGL例程1 无操作系统移植\Projects\MDK-ARM\atk_h743.uvprojx`
- `4，LVGL例程\LVGL例程2 操作系统移植\Projects\MDK-ARM\atk_h743.uvprojx`
- `4，LVGL例程\LVGL例程49 二维码生成器(800x480)\Projects\MDK-ARM\atk_h743.uvprojx`
- `4，LVGL例程\LVGL例程50 绘画系统(800x480)\Projects\MDK-ARM\atk_h743.uvprojx`
- `4，LVGL例程\LVGL综合实验(800x448)\Projects\MDK-ARM\atk_h743.uvprojx`

`LVGL例程1 无操作系统移植` 已确认包含 LCD、LTDC、SDRAM、GT9xxx 触摸、LVGL 配置和 `User/main.c`，适合作为下一阶段只读复核依据。

LVGL 学习资料：

- `D:\datafile\stm32\14，LVGL学习资料\lvgl-master.zip`，实际内容为 `lvgl-release-v8.2`
- `D:\datafile\stm32\14，LVGL学习资料\LVGL使用工具.zip`，包含 LVGL 英文手册、字体工具和 GUI 构建工具资料

通用 HAL 资料 A 盘参考：

- `D:\project\gitdemo\STM32H7_INVERTER\【正点原子】手把手教你学STM32 HAL库（全系列）资料A盘`
- LTDC/RGB 例程：`2. 入门篇\26，LTDC\课堂源码`
- 4.3 寸 800x480 RGB 屏资料：`2. 入门篇\26，LTDC\器件手册\裸屏数据手册\ATK-4.3-800480.pdf`
- SDRAM 例程：`2. 入门篇\25，FSMC_FMC\3，SDRAM实验`
- 触摸例程：`2. 入门篇\38，TOUCH`
- QSPI 例程：`2. 入门篇\35，SPI_QSPI`

官方下载入口：

- 正点原子资料汇总仓库：`https://github.com/alientek-openedv/Products`
- Apollo H743 索引页：`https://github.com/alientek-openedv/Products/blob/master/zdyz_docs/boards/stm32/zdyz_stm32h743_apollo.rst`
- 开发板资料 A 盘：`https://pan.baidu.com/s/1FfkUEBaGzYi-Gl8aWBUqfQ`
- 提取码：`alkw`

## 硬件确认表

| 项目 | 状态 | 当前结论 | 来源 / 备注 |
|---|---|---|---|
| MCU 型号 | 已确认 | `STM32H743IIT6`，LQFP176，2MB Flash，1060KB SRAM | Apollo V2 硬件参考手册、开发指南、`Template.ioc` |
| 开发板型号 | 已确认 | 正点原子 Apollo V2 STM32H743，核心板型号 `ATK-CNH743 V2` | Apollo V2 硬件参考手册 |
| 核心板原理图 | 已找到 | `STM32H743_CORE V2.0.pdf` | `D:\datafile\stm32\3，原理图.zip` |
| 底板原理图 | 已找到 | `DNF429P_F767P_H743P V2.6.pdf` | `D:\datafile\stm32\3，原理图.zip` |
| 屏幕尺寸 | 已确认 | 4.3 寸 RGB LCD | 用户需求、LCD 资料 |
| 屏幕分辨率 | 已确认 | `800x480` | 用户需求、LCD 资料 |
| 屏幕接口 | 已确认 | RGB / LTDC | Apollo V2 硬件参考手册 |
| 屏幕色彩格式 | 已确认 | RGB565，16 bit | Apollo V2 硬件参考手册、LVGL 开发指南 |
| LCD 帧缓冲大小 | 已确认 | `800 * 480 * 2 = 768000 bytes = 750 KiB` | 根据分辨率和 RGB565 计算 |
| 屏幕像素时钟 | 已确认 | Apollo H743 HAL `实验15 LTDC LCD（RGB屏）实验` 对 4.3 寸 800x480 屏使用 PLL3M=25、PLL3N=300、PLL3R=9，LTDC pixel clock 约 33.333MHz | Phase 4 已写入工程，串口日志确认，用户确认画面正常 |
| HSYNC/VSYNC/DE/CLK 极性 | 已确认 | HSPolarity AL、VSPolarity AL、DEPolarity AL、PCPolarity IPC | Phase 4 已写入工程，串口日志确认，用户确认画面正常 |
| 屏幕时序参数 | 已确认 | 4.3 寸 800x480，ID `0x4384`：HSW 48、VSW 3、HBP 88、VBP 32、HFP 40、VFP 13 | Phase 4 已写入工程，串口日志确认，用户确认画面正常 |
| LCD 背光引脚 | 已确认 | `PB5` / `LCD_BL` | Apollo V2 硬件参考手册、LTDC 例程 |
| LCD 实际显示 | 已确认 | Phase 4 已下载，串口显示 LTDC 初始化完成，framebuffer 已写入彩条，用户确认屏幕显示正常 | 可进入触摸裸驱动阶段 |
| 触摸芯片资料 | 已部分确认 | 本屏 I2C 读取 PID 为 `1158`，按 Goodix GT1158 / GT1x 类处理；仍建议最终核对屏幕触摸芯片丝印 | Phase 5 裸驱动和官方触摸 HEX 均输出 `1158` |
| 触摸接口 | 已确认 | 软件 I2C，SCL=PH6、SDA=PI3，另有 INT=PH7、RST=PI8 | Phase 5 已按官方触摸例程接入并读到 PID/配置区 |
| 触摸 I2C 地址 | 已确认 | `0x14`，写命令 `0x28`，读命令 `0x29` | Phase 5 实板识别通过 |
| 触摸配置区 | 已确认 | GT1158 有效配置区为 `0x8050`，读到 `x_max=800`、`y_max=480`、最大触点数 5 | Phase 5 诊断日志 |
| 触摸事件上报 | 已确认 | 官方触摸 HEX 已识别 `CTP ID:1158` 且用户确认可以划线；本项目裸驱动已抓到 `0x814E=0x81`，并输出 `TP down` / `TP up`、raw 坐标和屏幕坐标 | Phase 5 裸触摸通过，后续可接入 LVGL 输入设备 |
| SDRAM 型号 | 已确认 | `W9825G6KH` | Apollo V2 硬件参考手册、`7，硬件资料.zip` |
| SDRAM 容量 | 已确认 | 32MB | Apollo V2 硬件参考手册 |
| SDRAM 数据宽度 | 已确认 | 16 bit | 硬件手册、通用 SDRAM 例程 |
| SDRAM FMC 存储体 | 已确认 | `FMC_SDRAM_BANK1` / Bank5 地址空间 | 已按 Apollo H743 SDRAM 例程配置并通过实板 memtest |
| SDRAM 基地址 | 已确认 | `0xC0000000` | 32MB 全容量 memtest 通过，连续复位 10 次通过 |
| SDRAM 关键配置 | 已确认 | 9 列地址、13 行地址、4 个内部 Bank、CAS 2、Read Burst 使能、Read Pipe Delay 1、SDCLK 110MHz、刷新计数 839 | 正点原子 Apollo H743 HAL `实验14 SDRAM实验`，已实板验证 |
| 外部 SPI Flash 型号 | 已确认 | `W25Q256`，32MB | Apollo V2 硬件参考手册、`7，硬件资料.zip` |
| 外部 Flash 接口 | 已确认 | QSPI | Apollo V2 硬件参考手册 |
| QSPI 引脚 | 已确认 | PB2 CLK、PB6 NCS、PF6 IO0、PF7 IO1、PF8 IO2、PF9 IO3 | 正点原子 Apollo H743 HAL `实验28 QSPI实验`，Phase 7-9 已实板识别 W25Q256 |
| QSPI 内存映射基地址 | 部分确认 | CMSIS 定义 QSPI memory mapped 基址 `0x90000000`；当前阶段不启用 memory mapped | Phase 8 参数保存使用普通命令模式读/擦/写 |
| SD 卡接口 | 已实板确认 | SDMMC1，PC8 D0、PC9 D1、PC10 D2、PC11 D3、PC12 CLK、PD2 CMD，AF12；当前固件为稳定性使用 1-bit + 低速 + 硬件流控 | 4GB FAT32 SD 卡已完成 mount、文件读取和 IAP 闭环；原 4-bit/较高速轮询读曾触发 `SDMMC_ERROR_RX_OVERRUN=0x00000020` |
| SD 卡本地 IAP | 已实板确认 | SD 卡根目录放置 `app_b_slot.bin`，串口 `iap sd` 或升级页 `SD File` 触发，只读文件后写入 W25Q256 staging | 2026-06-27 回归通过：读完 324564 字节 AppB 包，写 pending，复位后 Boot 安装 AppB 并 confirmed；无卡、挂载失败、文件无效或 CRC 失败均不写 pending，不影响 AppA |
| EEPROM | 已确认 | 24C02，256B | Apollo V2 硬件参考手册 |
| NAND Flash | 已确认存在 | 512MB | Apollo V2 硬件参考手册；第一版 LVGL bring-up 暂不使用 |
| 串口日志端口 | 已确认方向 | USART1 引出为 TTL 串口，可用于通信和调试 | 仍需结合原理图和实物确认 USB 转串口接线 |
| USB_UART / CH340 | 已确认 | 当前 Windows `COM5` 为板载 CH340 / USART1，只用于串口日志和串口 IAP | Apollo V2 硬件参考手册 2.1.33；不要把 COM5 当作 USB CDC |
| USB_SLAVE / USB OTG | 已实板确认 | MCU USB Device 使用 USB2_OTG_FS，PA11=USB_D-，PA12=USB_D+，连接 USB_SLAVE / USB OTG Type-C 口 | 2026-06-27 Windows 已枚举 MCU USB CDC COM4，VID_0483&PID_5740 |
| CAN/USB 选择跳帽 | 已实板确认 | P9 必须选择 USB，使 PA11/PA12 连接 USB；若选择 CAN，USB CDC 不会枚举 | 当前板卡已能枚举 USB CDC，说明实测连接路径有效；仍建议装配/交付时记录跳帽位置 |
| USB CDC 实板枚举 | 已通过 | AppB 日志显示 `USB CDC early init OK` 和 `IAP: USB CDC init OK`，Windows 枚举 `USB 串行设备 (COM4)`，PNP 为 `USB\\VID_0483&PID_5740&MI_00` | COM4 `iap status` 回包正常；USB CDC 传输 324564 字节 AppB 包成功，复位后 Boot 安装 AppB 并 confirmed |
| 系统诊断显示 | 串口已确认，HMI 待肉眼确认 | App 首页已增加 reset reason、Fault、heap、任务栈余量和关键时钟摘要；串口启动日志增加 `diag:` 行 | 2026-06-27 串口确认 `diag: boot_count`、`diag: last_fault=none`、任务栈和 heap 周期日志正常；HMI 首页字段仍建议肉眼确认 |
| LED 引脚 | 已确认 | LED0/DS0 红灯：PB1；LED1/DS1 绿灯：PB0 | Apollo V2 硬件参考手册、例程 |
| LED0 心跳 | 已确认 | LED0 每 500ms 翻转 | 用户已实物确认 |
| 按键引脚 | 已确认 | KEY0 PH3、KEY1 PH2、KEY2 PC13、KEY_UP PA0；KEY0/1/2 低电平有效，KEY_UP 高电平有效 | Apollo V2 硬件参考手册、开发指南 |
| HSE / LSE | 部分确认 | `Template.ioc` 使用 HSE 和 LSE 外部晶振；`system_stm32h7xx.c` 默认 HSE 为 25MHz | 第 1/2 阶段使用 Apollo H743 官方时钟参数复核 |
| 工具链基线 | 已确认本地资料 | CubeMX 模板目标为 MDK-ARM V5.32，Cube FW H7 V1.9.1 | `Template.ioc` |
| 官方验收工程 | 已找到 | 无屏幕版 `atk_h743.uvprojx` | `D:\datafile\stm32\1，入门资料.zip` |
| HAL 标准例程 | 已找到 | 新建工程、SDRAM、LTDC RGB LCD、QSPI、触摸屏等 Apollo H743 例程已找到 | `D:\datafile\stm32\4，程序源码\2，标准例程-HAL库版本.zip` |
| LVGL 移植参考工程 | 已找到 | 已找到 Apollo H743 的无操作系统、操作系统和 800x480 LVGL 示例 | `D:\datafile\stm32\4，程序源码\3，扩展例程\4，LVGL例程.zip` |
| LVGL 版本资料 | 已找到 | `lvgl-release-v8.2`，含 `lv_conf_template.h` | `D:\datafile\stm32\14，LVGL学习资料\lvgl-master.zip` |
| LVGL 例程颜色深度 | 已抽查 | `LV_COLOR_DEPTH = 16` | `LVGL例程1 无操作系统移植\Middlewares\LVGL\GUI\lvgl\lv_conf.h` |
| LVGL 例程内部堆 | 已抽查 | `LV_MEM_SIZE = 46KB`，仅作为参考，本项目第一版暂按 128KB 规划 | 同上 |

## 仍缺或待确认资料

在以下项目确认前，不写触摸、QSPI 和 LVGL 相关驱动代码：

- 实际 4.3 寸屏的触摸控制器型号和 I2C 地址。
- 当前板卡 USART1 / USB 转串口接线仍建议结合原理图做最终记录。

## 第 0 阶段结论

当前资料已足够完成项目骨架、硬件确认表和第一版内存规划。官方无屏幕验收工程已补齐，可用于下一阶段复核基础时钟、串口、SDRAM、QSPI 和 BSP 配置。Apollo H743 LVGL 参考例程也已找到，可用于下一阶段只读复核 LCD、LTDC、触摸和 LVGL 配置；本阶段仍不写任何驱动代码，也不复制旧工程作为新工程基础。

## Phase 6 追加确认

| 项目 | 状态 | 当前结论 | 备注 |
|---|---|---|---|
| LVGL 版本 | 已确认 | 已导入官方 `v8.4.0` | 本机 v8.2 学习包未作为本工程源码使用 |
| LVGL 色彩格式 | 已配置 | `LV_COLOR_DEPTH=16`，`LV_COLOR_16_SWAP=0` | 对应 LTDC RGB565 framebuffer |
| LVGL draw buffer | 已配置 | `800x40` pixels，64000 bytes | 位于 RAM_D1 |
| SDRAM framebuffer | 已配置 | `0xC0000000`，768000 bytes | LTDC 扫描该 framebuffer |
| `disp_flush_cb` | 已实现 | CPU `memcpy` 脏区域到 SDRAM framebuffer | 当前不使用 DMA2D |
| LVGL tick | 已实现 | `SysTick_Handler()` 调用 `lv_tick_inc(1)` | 符合 Phase 6 要求 |
| LVGL 主循环 | 已实现 | `while(1)` 调用 `lv_timer_handler()` | 未使用 FreeRTOS |
| 测试页面 | 已确认 | button、label、slider 已显示 | 用户现场确认“可以看到” |
| touch indev | 已确认 | 触摸坐标已进入 LVGL，slider 事件已触发 | 串口抓到 `LVGL TP down x=535 y=249` 和 `LVGL slider value=54` |

当前结论：Phase 6 的 LVGL 显示链路和触摸输入链路均已通过实板确认，可以进入下一阶段前的稳定性复测。

## Phase 7-9 追加确认

| 项目 | 状态 | 当前结论 | 备注 |
|---|---|---|---|
| FreeRTOS 内核 | 已接入 | 使用 STM32Cube_FW_H7_V1.9.1 自带 FreeRTOS V10.3.1，GCC ARM_CM7 r0p1 端口 | CMake 工程已编译通过 |
| `task_gui` | 已运行 | LVGL 初始化、页面创建、`lv_tick_inc()`、`lv_timer_handler()` 均在该任务路径中执行 | 符合“LVGL 只在 task_gui 中直接调用”的约束 |
| `task_storage` | 已运行 | 负责 W25Q256 参数加载、延迟保存、读回校验 | 串口已输出任务栈余量 |
| `task_log` | 已运行 | LED 心跳、周期打印栈余量和参数快照 | 串口周期输出正常 |
| W25Q256 识别 | 已确认 | JEDEC ID `0xEF4019` | 容量类型为 256Mbit / 32MB |
| W25Q256 参数区 | 已确认 | `0x00000000 - 0x0000FFFF`，64KB | 当前实际写入第一个 4KB sector |
| 参数块格式 | 已实现 | 包含 `magic/version/length/crc32/payload[512]` | CRC32 覆盖 version、length 和 payload 有效长度 |
| 默认参数保存 | 已确认 | 空参数区 `bad_magic` 后自动保存默认参数，`save_ok=1` | 避免首次复位仍为无效参数区 |
| 复位后参数保持 | 已确认 | 复位后 `load=flash_ok`，参数快照保持 `brightness=71 threshold=55 output=1 language=0 selftest=0x53385431` | 已通过串口复测 |
| 延迟保存 | 已确认 | 参数自检修改后 dirty，默认 3000ms 后由 `task_storage` 保存，复位后保持 | 串口抓到 `save_ok=1`，再次复位后 `dirty=0` 且未重复保存 |
| 首页 | 已实现，待肉眼确认 | 显示运行时间、模拟电压/电流/温度、参数状态 | Phase 9 页面之一 |
| 参数页 | 已实现，待肉眼确认 | Brightness、Threshold、Output、Save | Phase 9 页面之一 |
| 升级页 | 已实现，待肉眼确认 | 当前版本、新版本、模拟升级进度、校验状态 | Phase 9 页面之一 |
| 日志页 | 已实现，待肉眼确认 | 最近事件列表 | Phase 9 页面之一 |

当前结论：Phase 7 和 Phase 8 的核心链路已通过实板串口验证；Phase 9 页面已实现并下载运行，仍需用户确认屏幕四个页面显示和触摸操作。
