# Apollo H743 LVGL HMI 调试记录

## 2026-05-25 第 0 阶段

本项目从 0 开始建立，不假设已有旧工程源码，也不复制其它工程作为基础。

已新建项目目录：

```text
D:\project\gitdemo\STM32H7_INVERTER\apollo_h743_lvgl_hmi
```

已创建必须维护的文档：

```text
Docs/bringup_log.md
Docs/memory_map.md
Docs/hardware_checklist.md
```

本阶段没有写任何驱动代码。

## 任务书位置

用户提到的路径是：

```text
Document/apollo_h743_lvgl_reproduction_plan.md
```

当前仓库实际存在的任务书是：

```text
D:\project\gitdemo\STM32H7_INVERTER\apollo_h743_lvgl_reproduction_plan.md
```

本次第 0 阶段使用仓库根目录下的任务书作为执行依据。

## 本地资料搜索记录

搜索范围：

- 当前仓库：`D:\project\gitdemo\STM32H7_INVERTER`
- `C:\Users\Luo\Desktop`
- `C:\Users\Luo\Downloads`
- `D:\project`
- `D:\datafile`

已找到 Apollo H743 V2 专用文档：

```text
C:\Users\Luo\Desktop\getnewjob\docs\阿波罗V2 硬件参考手册_V1.0（H743版本）.pdf
C:\Users\Luo\Desktop\getnewjob\docs\STM32H743 阿波罗开发指南V1.0.pdf
C:\Users\Luo\Desktop\getnewjob\docs\LVGL开发指南_V1.5.pdf
C:\Users\Luo\Desktop\getnewjob\docs\FreeRTOS开发指南_V1.10.pdf
C:\Users\Luo\Desktop\getnewjob\docs\实验0-4，新建工程实验-CubeMX版本\Template.ioc
```

后来在 `D:\datafile\stm32` 找到更新或更完整的 Apollo H743 资料：

```text
D:\datafile\stm32\STM32H743 阿波罗开发指南V1.3.pdf
D:\datafile\stm32\阿波罗V2 硬件参考手册_V1.1（H743版本）.pdf
D:\datafile\stm32\1，入门资料.zip
D:\datafile\stm32\3，原理图.zip
D:\datafile\stm32\4，程序源码
D:\datafile\stm32\7，硬件资料.zip
D:\datafile\stm32\14，LVGL学习资料
D:\datafile\stm32\实验0-4，新建工程实验-CubeMX版本\Template.ioc
```

其中 `1，入门资料.zip` 已确认可以读取，包含官方验收工程：

```text
1，入门资料/验收所需资料（无屏幕版）/1，验收工程（无屏幕版）/Projects/MDK-ARM/atk_h743.uvprojx
1，入门资料/验收所需资料（无屏幕版）/1，验收工程（无屏幕版）/Drivers/BSP/SDRAM/sdram.c
1，入门资料/验收所需资料（无屏幕版）/1，验收工程（无屏幕版）/Drivers/BSP/QSPI/qspi.c
1，入门资料/验收所需资料（无屏幕版）/1，验收工程（无屏幕版）/Drivers/BSP/LCD/lcd.c
1，入门资料/验收所需资料（无屏幕版）/1，验收工程（无屏幕版）/Drivers/BSP/LCD/ltdc.c
1，入门资料/验收所需资料（无屏幕版）/1，验收工程（无屏幕版）/Drivers/SYSTEM/usart/usart.c
```

该 zip 是“无屏幕版”验收工程，未发现 LVGL 工程，也未发现独立“带屏幕版”验收工程目录。

其中 `3，原理图.zip` 内已确认包含：

```text
STM32H743_CORE V2.0.pdf
DNF429P_F767P_H743P V2.6.pdf
DNF429P_F767P_H743P V2.5.pdf
ATK-MD0430 V1.7.pdf
ATK-MD0430R V1.9.pdf
阿波罗V2 STM32H743 开发板 IO 口功能表.xlsx
```

其中 `7，硬件资料.zip` 内已确认包含：

```text
STM32H743IIT6.pdf
W9825G6KH.pdf
W25Q256.pdf
GT911 / GT9147 / GT917S / GT1151Q 相关数据手册和编程指南
4.3 寸 RGB4342/RGB4384 资料
4.3 寸 800x480 屏规格资料
4.3 寸 800x480 屏控制 IC ST7262 资料
```

当前仓库内也找到正点原子通用 HAL 资料 A 盘：

```text
D:\project\gitdemo\STM32H7_INVERTER\【正点原子】手把手教你学STM32 HAL库（全系列）资料A盘
```

其中和本项目相关的通用资料包括：

```text
2. 入门篇\26，LTDC\课堂源码
2. 入门篇\26，LTDC\器件手册\裸屏数据手册\ATK-4.3-800480.pdf
2. 入门篇\25，FSMC_FMC\3，SDRAM实验
2. 入门篇\38，TOUCH
2. 入门篇\35，SPI_QSPI
```

这些通用资料只能作为参考，不能视为 Apollo H743 V2 的专用基础工程。

用户补充下载的 `D:\datafile\stm32\4，程序源码` 已复查，目录内容包括：

```text
D:\datafile\stm32\4，程序源码\1，标准例程-寄存器版本.zip
D:\datafile\stm32\4，程序源码\2，标准例程-HAL库版本.zip
D:\datafile\stm32\4，程序源码\3，扩展例程\4，LVGL例程.zip
D:\datafile\stm32\4，程序源码\4，STM32启动文件.zip
```

其中 `2，标准例程-HAL库版本.zip` 已确认可以读取，大小约 314MB，包含 Apollo H743 的 MDK-ARM 工程。和本项目 bring-up 最相关的 HAL 例程如下：

```text
2，标准例程-HAL库版本/实验0 基础入门实验/实验0-3，新建工程实验-HAL库版本/Projects/MDK-ARM/atk_h743.uvprojx
2，标准例程-HAL库版本/实验14 SDRAM实验/Projects/MDK-ARM/atk_h743.uvprojx
2，标准例程-HAL库版本/实验15 LTDC LCD（RGB屏）实验/Projects/MDK-ARM/atk_h743.uvprojx
2，标准例程-HAL库版本/实验28 QSPI实验/Projects/MDK-ARM/atk_h743.uvprojx
2，标准例程-HAL库版本/实验31 触摸屏实验/Projects/MDK-ARM/atk_h743.uvprojx
```

这些例程作为下一阶段复核时钟、FMC SDRAM、LTDC、QSPI、触摸和 MDK 工程结构的依据，不复制为本项目基础工程。

其中 `4，LVGL例程.zip` 已确认可以读取，大小约 545MB，包含约 42047 个条目。和本项目最相关的 Apollo H743 LVGL 工程如下：

```text
4，LVGL例程/LVGL例程1 无操作系统移植/Projects/MDK-ARM/atk_h743.uvprojx
4，LVGL例程/LVGL例程2 操作系统移植/Projects/MDK-ARM/atk_h743.uvprojx
4，LVGL例程/LVGL例程49 二维码生成器(800x480)/Projects/MDK-ARM/atk_h743.uvprojx
4，LVGL例程/LVGL例程50 绘画系统(800x480)/Projects/MDK-ARM/atk_h743.uvprojx
4，LVGL例程/LVGL综合实验(800x448)/Projects/MDK-ARM/atk_h743.uvprojx
```

`LVGL例程1 无操作系统移植` 中已确认存在以下关键参考文件：

```text
Drivers/BSP/LCD/lcd.c
Drivers/BSP/LCD/ltdc.c
Drivers/BSP/SDRAM/sdram.c
Drivers/BSP/TOUCH/gt9xxx.c
Drivers/BSP/TOUCH/touch.c
Middlewares/LVGL/GUI/lvgl/lv_conf.h
User/main.c
```

该例程的 `lv_conf.h` 已只读抽查，确认 `LV_COLOR_DEPTH = 16`，`LV_MEM_SIZE = 46KB`。这只能作为移植参考；本项目第一版 HMI 的 LVGL heap 仍按 `128KB` 规划，后续根据实际界面复杂度调整。

用户补充下载的 `D:\datafile\stm32\14，LVGL学习资料` 已复查，目录内容包括：

```text
D:\datafile\stm32\14，LVGL学习资料\lvgl-master.zip
D:\datafile\stm32\14，LVGL学习资料\LVGL使用工具.zip
```

其中 `lvgl-master.zip` 实际目录名为 `lvgl-release-v8.2`，包含 `README.md` 和 `lv_conf_template.h`；`LVGL使用工具.zip` 包含 LVGL 英文手册、字体工具和 GUI 构建工具压缩包。它们作为 LVGL 版本、配置和资源制作参考，不直接作为本项目源码基础。

## 官方资料入口

已从正点原子官方 GitHub 资料汇总仓库找到 Apollo H743 的资料入口：

```text
https://github.com/alientek-openedv/Products
https://github.com/alientek-openedv/Products/blob/master/zdyz_docs/boards/stm32/zdyz_stm32h743_apollo.rst
```

官方页面列出的 Apollo H743 开发板资料 A 盘：

```text
百度网盘：https://pan.baidu.com/s/1FfkUEBaGzYi-Gl8aWBUqfQ
提取码：alkw
```

当前本机 `D:\datafile\stm32` 已有可读取的 `1，入门资料.zip`，官方无屏幕验收工程已补齐。

## 已补齐的资料

- Apollo H743 V2 官方入门资料包：已找到 `D:\datafile\stm32\1，入门资料.zip`。
- Apollo H743 V2 官方无屏幕验收工程：已找到 `atk_h743.uvprojx`，可作为硬件配置复核参考。
- Apollo H743 V2 核心板原理图：已在 `3，原理图.zip` 中找到 `STM32H743_CORE V2.0.pdf`。
- Apollo F4/F7/H7 通用底板原理图：已在 `3，原理图.zip` 中找到 `DNF429P_F767P_H743P V2.6.pdf`。
- 4.3 寸屏幕原理图：已在 `3，原理图.zip` 中找到 `ATK-MD0430R V1.9.pdf`，另有 `ATK-MD0430 V1.7.pdf`。
- 4.3 寸 800x480 RGB 屏资料：已在 `7，硬件资料.zip` 和仓库通用 A 盘中找到。
- 触摸控制器数据手册：已找到 GT911、GT9147、GT917S、GT1151Q 等资料。
- SDRAM 芯片手册：已找到 `W9825G6KH.pdf`。
- QSPI Flash 芯片手册：已找到 `W25Q256.pdf`。
- Apollo H743 HAL 标准例程：已找到 `D:\datafile\stm32\4，程序源码\2，标准例程-HAL库版本.zip`。
- Apollo H743 LVGL 移植参考工程：已找到 `D:\datafile\stm32\4，程序源码\3，扩展例程\4，LVGL例程.zip`。
- LVGL v8.2 源码和工具资料：已找到 `D:\datafile\stm32\14，LVGL学习资料`。

## 仍缺或仍需确认的资料

写驱动代码前仍需补齐或现场确认：

- 带屏幕版官方验收工程；当前 `1，入门资料.zip` 只看到无屏幕版验收工程，但 `4，LVGL例程.zip` 已提供 Apollo H743 的 LCD/LVGL 参考工程。
- Apollo H743 V2 专用 LTDC/RGB LCD 最终屏幕参数仍需结合 LCD 资料复核。
- 当前实际 4.3 寸屏模块的触摸芯片丝印，确认到底是 GT911、GT9147、GT917S、GT1151Q 还是其它型号。
- 当前板卡 USART1 与 USB 转串口的实际接线，需要结合原理图和实物再核对。

## 第 0 阶段结论

第 0 阶段文档和资料确认工作已完成到可继续规划的程度：

- 已创建项目目录和 `Docs` 文档。
- 已完成中文硬件确认表。
- 已完成第一版中文内存规划。
- 已找到硬件资料、原理图、芯片数据手册、官方无屏幕验收工程和 Apollo H743 LVGL 参考例程。
- 未写任何驱动代码。

下一阶段如要进入最小裸机工程，应优先从官方无屏幕验收工程中复核时钟、FMC SDRAM、QSPI、串口和基础 BSP 配置；LCD/LVGL 则从 `LVGL例程1 无操作系统移植`、`LVGL例程2 操作系统移植` 和 800x480 示例中只读提取配置依据。不能把旧例程直接复制成新工程基础。

## 2026-05-25 第 1 / 2 阶段

本阶段目标是确认最小系统稳定，只创建最小 STM32H743 App 工程，实现：

- `HAL_Init`
- `SystemClock_Config`
- USART1 `printf`
- LED0 心跳
- 记录系统时钟基线

本阶段明确不接入：

- SDRAM / FMC 初始化
- LCD / LTDC 初始化
- LVGL
- FreeRTOS

### 新增工程结构

已在 `apollo_h743_lvgl_hmi` 下新增最小 App 工程：

```text
CMakeLists.txt
CMakePresets.json
cmake/arm-none-eabi-gcc.cmake
App/Core/Inc
App/Core/Src
App/Drivers/CMSIS
App/Drivers/STM32H7xx_HAL_Driver
App/Linker/STM32H743IITx_FLASH.ld
```

工具链选择：

```text
arm-none-eabi-gcc 12.2.1
CMake
Ninja
```

本机未发现可直接调用的 Keil `UV4.exe`，所以 Phase 1/2 先建立可命令行编译的 GCC/CMake 最小工程。后续如需要 Keil 工程，可以在当前最小工程稳定后再补 MDK 工程文件。

### 官方例程依据

时钟参数优先使用正点原子 Apollo H743 HAL 标准例程：

```text
D:\datafile\stm32\4，程序源码\2，标准例程-HAL库版本.zip
2，标准例程-HAL库版本/实验1 跑马灯实验/User/main.c
2，标准例程-HAL库版本/实验1 跑马灯实验/Drivers/SYSTEM/sys/sys.c
```

官方跑马灯例程启动顺序：

```text
sys_cache_enable()
HAL_Init()
sys_stm32_clock_init(160, 5, 2, 4)
delay_init(400)
led_init()
```

本工程采用相同的主 PLL1 参数：

```text
HSE = 25MHz
PLL1M = 5
PLL1N = 160
PLL1P = 2
PLL1Q = 4
SYSCLK = 400MHz
HCLK = 200MHz
APB1 = 100MHz
APB2 = 100MHz
APB3 = 100MHz
APB4 = 100MHz
```

串口和 LED 引脚依据也来自官方跑马灯 / USART 例程：

```text
USART1_TX = PA9，AF7
USART1_RX = PA10，AF7
USART1 baudrate = 115200
LED0/DS0 红灯 = PB1
LED1/DS1 绿灯 = PB0
```

### 当前代码行为

当前入口代码位于：

```text
App/Core/Src/main.c
```

启动流程：

```text
SCB_EnableICache()
HAL_Init()
SystemClock_Config()
app_led_init()
app_uart1_init(115200)
printf 启动日志和时钟信息
while(1) 每 500ms 翻转 LED0 并打印 heartbeat
```

缓存策略：

```text
ICache = 开启
DCache = 关闭
MPU = 未配置
```

原因：本阶段不接 SDRAM/LCD。DCache 和 MPU 留到 SDRAM、framebuffer、DMA2D 相关阶段统一处理，避免在最小系统阶段引入缓存一致性变量。

### 时钟记录

当前最小工程实际配置：

| 时钟项 | 当前值 | 说明 |
|---|---:|---|
| HSE | 25MHz | 正点原子例程和 HAL 配置 |
| PLL1 VCO | 800MHz | `25 / 5 * 160` |
| SYSCLK | 400MHz | `PLL1P = 2` |
| HCLK | 200MHz | `SYSCLK / 2` |
| APB1 | 100MHz | `HCLK / 2` |
| APB2 | 100MHz | `HCLK / 2` |
| APB3 | 100MHz | `HCLK / 2` |
| APB4 | 100MHz | `HCLK / 2` |
| LTDC pixel clock | 未启用 | 本阶段不初始化 LCD/LTDC |
| FMC / SDRAM clock | 未启用 | 本阶段不初始化 SDRAM/FMC |

后续阶段的官方参考值已经记录，但本阶段不启用：

```text
FMC/SDRAM：官方 SDRAM 例程注释记录 PLL2_R = 220MHz；
          SDRAM 配置使用 SDClockPeriod = 2，因此 SDRAM 时钟为 110MHz。

LTDC：官方 LTDC RGB 屏例程中，4.3 寸 800x480 屏 ID 0x4384
      使用 PLL3M = 25、PLL3N = 300、PLL3R = 9，
      像素时钟约 33MHz。
```

### 本地编译验证

已执行：

```text
cmake --preset gcc-debug
cmake --build --preset gcc-debug --target clean
cmake --build --preset gcc-debug
```

编译通过，生成：

```text
build/gcc-debug/apollo_h743_lvgl_hmi.elf
build/gcc-debug/apollo_h743_lvgl_hmi.hex
build/gcc-debug/apollo_h743_lvgl_hmi.bin
build/gcc-debug/apollo_h743_lvgl_hmi.map
```

构建大小：

```text
FLASH used = 25308 bytes
RAM_D1 used = 6768 bytes
text = 25184
data = 116
bss = 6660
```

### 实板下载验证

验证时间：2026-05-25 23:10 +08:00

下载工具：

```text
xPack OpenOCD 0.12.0+dev-02228-ge5888bda3-dirty
ST-LINK V2J38S7
SWD 目标电压约 3.28V
目标识别为 STM32H74x/75x，Cortex-M7 r1p1
```

下载文件：

```text
build/gcc-debug/apollo_h743_lvgl_hmi.hex
```

下载命令：

```text
openocd -s <openocd_scripts> \
  -f interface/stlink.cfg \
  -f target/stm32h7x.cfg \
  -c "transport select swd" \
  -c "adapter speed 1000" \
  -c "program apollo_h743_lvgl_hmi/build/gcc-debug/apollo_h743_lvgl_hmi.hex verify reset exit"
```

下载结果：

```text
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
```

串口验证：

```text
串口设备：USB-SERIAL CH340 (COM5)
串口参数：115200, 8N1
```

复位后抓到的启动日志：

```text
Apollo H743 LVGL HMI minimal bring-up
Phase 1/2: HAL + clock + UART printf + LED heartbeat
SDRAM/LCD/LVGL: disabled in this phase
clock source: HSE=25MHz, PLL1 M=5 N=160 P=2 Q=4
SYSCLK=400000000 Hz
HCLK  =200000000 Hz
APB1  =100000000 Hz
APB2  =100000000 Hz
APB3  =100000000 Hz
APB4  =100000000 Hz
ICache=on, DCache=off
heartbeat tick=29
heartbeat tick=531
heartbeat tick=1033
heartbeat tick=1535
```

连续运行观察：

```text
20 秒串口采样得到 40 条 heartbeat。
首条：heartbeat tick=67297
末条：heartbeat tick=86875
heartbeat 周期约 500ms。
观察期间未看到启动日志重复打印，未看到异常复位迹象。
```

实板验收结果：

```text
串口输出启动日志和时钟信息：通过
串口每 500ms 输出 heartbeat：通过
LED0 每 500ms 翻转一次：通过，用户已确认
系统短时运行不复位：通过，已观察 20 秒
系统长时间运行不复位：待后续长时间老化测试
```

### 第 1 / 2 阶段结论

Phase 1 和 Phase 2 的最小系统已经完成到实板下载和串口验证通过的状态：

- 已创建最小 STM32H743 工程。
- 已实现 `HAL_Init`、`SystemClock_Config`、USART1 `printf`、LED0 心跳。
- 主时钟参数来自正点原子官方 Apollo H743 HAL 例程。
- 已记录 SYSCLK/HCLK/APB/LTDC/FMC 时钟状态。
- 未接 SDRAM、LCD、LVGL。
- 已通过 OpenOCD + ST-LINK 下载并 verify。
- 已通过 CH340 COM5 串口确认启动日志、时钟打印和 heartbeat。
- LED0 物理闪烁已由用户确认。

Phase 1 和 Phase 2 已完成。下一步进入 Phase 3：SDRAM 初始化和 memtest。

## 2026-05-25 第 3 阶段

目标：

```text
外部 SDRAM 可稳定读写。
```

本阶段范围：

```text
启用 FMC SDRAM 初始化。
执行独立 SDRAM memtest。
LCD/LTDC 不初始化。
LVGL 不接入。
DCache 仍关闭。
```

### 官方参数来源

SDRAM 参数来自正点原子 Apollo H743 HAL 标准例程：

```text
D:\datafile\stm32\4，程序源码\2，标准例程-HAL库版本.zip
2，标准例程-HAL库版本/实验14 SDRAM实验/Drivers/BSP/SDRAM/sdram.c
2，标准例程-HAL库版本/实验14 SDRAM实验/Drivers/BSP/SDRAM/sdram.h
```

确认到的 SDRAM 参数：

```text
SDRAM 芯片：W9825G6KH
容量：32MB
FMC SDRAM Bank：FMC_SDRAM_BANK1
基地址：0xC0000000
列地址位数：9
行地址位数：13
数据宽度：16 bit
内部 Bank 数：4
CAS Latency：2
Read Burst：Enable
Read Pipe Delay：1
刷新计数：839
```

官方例程注释记录 FMC 使用 PLL2_R：

```text
PLL2M = 25
PLL2N = 440
PLL2P = 2
PLL2Q = 2
PLL2R = 2
PLL2_R = 220MHz
SDClockPeriod = 2
SDRAM SDCLK = 110MHz
```

当前工程中显式通过 `HAL_RCCEx_PeriphCLKConfig()` 将 FMC kernel clock 选择为 `RCC_FMCCLKSOURCE_PLL2`。

### 代码变更

新增或启用：

```text
App/Core/Inc/sdram.h
App/Core/Src/sdram.c
App/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sdram.c
App/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_fmc.c
App/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c
```

说明：

```text
当前业务代码不使用 MDMA 传输。
启用 MDMA 模块是因为 HAL SDRAM 驱动头文件和 DMA API 依赖 MDMA_HandleTypeDef。
```

MPU 策略：

```text
0xC0000000 - 0xC1FFFFFF，32MB SDRAM
属性：Full access、non-cacheable、non-bufferable、execute-never
原因：当前 DCache 关闭，先用最保守的 SDRAM 可访问属性完成独立 memtest。
后续 LCD/DMA2D 阶段再统一评估缓存策略。
```

### 编译验证

已执行：

```text
cmake --build --preset gcc-debug
```

编译通过，生成：

```text
build/gcc-debug/apollo_h743_lvgl_hmi.elf
build/gcc-debug/apollo_h743_lvgl_hmi.hex
build/gcc-debug/apollo_h743_lvgl_hmi.bin
build/gcc-debug/apollo_h743_lvgl_hmi.map
```

构建大小：

```text
FLASH used = 33500 bytes
RAM_D1 used = 6816 bytes
text = 33376
data = 116
bss = 6708
```

### 实板下载验证

验证时间：2026-05-25 23:43 +08:00

下载命令：

```text
openocd -s <openocd_scripts> \
  -f interface/stlink.cfg \
  -f target/stm32h7x.cfg \
  -c "transport select swd" \
  -c "adapter speed 1000" \
  -c "program apollo_h743_lvgl_hmi/build/gcc-debug/apollo_h743_lvgl_hmi.hex verify reset exit"
```

下载结果：

```text
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
```

串口验证：

```text
串口设备：USB-SERIAL CH340 (COM5)
串口参数：115200, 8N1
```

抓到的关键启动日志：

```text
Phase 3: FMC SDRAM init + standalone memtest
LCD/LVGL: disabled in this phase
SYSCLK=400000000 Hz
HCLK  =200000000 Hz
APB1  =100000000 Hz
APB2  =100000000 Hz
APB3  =100000000 Hz
APB4  =100000000 Hz
PLL2_R=220000000 Hz
FMC   =220000000 Hz
SDCLK =110000000 Hz
ICache=on, DCache=off
SDRAM base=0xC0000000 size=33554432 bytes
SDRAM FMC: bank1, 16-bit, col=9, row=13, banks=4, CAS=2
SDRAM clock: FMC kernel=220000000 Hz, SDCLK=110000000 Hz, refresh=839
SDRAM MPU: 32MB non-cacheable, non-bufferable, execute-never
SDRAM init start
SDRAM init done
SDRAM memtest start: 33554432 bytes
SDRAM test pass: 33554432 bytes
```

memtest 覆盖项：

```text
walking bit
0x55555555 全容量写读校验
0xAAAAAAAA 全容量写读校验
递增地址模式全容量写读校验
```

连续复位验证：

```text
连续 reset run 10 次，10 次均输出：
SDRAM test pass: 33554432 bytes

每次完成时间约 8.8 秒。
```

### 第 3 阶段结论

Phase 3 已完成：

- SDRAM 基地址确认并实板验证为 `0xC0000000`。
- FMC SDRAM Bank 确认为 `FMC_SDRAM_BANK1`。
- FMC kernel clock 为 `220MHz`，SDRAM SDCLK 为 `110MHz`。
- 32MB SDRAM 全容量 memtest 单次通过。
- 连续复位 10 次均通过。
- LCD/LTDC/LVGL 仍未接入。

下一步进入 Phase 4：不接 LVGL，只做 LTDC 裸 framebuffer 点亮。

## Phase 4：LTDC 裸 framebuffer 彩条点亮

日期：2026-05-26

### 阶段目标

本阶段只验证 RGB LCD / LTDC 最小显示链路：

- 使用 SDRAM `0xC0000000` 作为 RGB565 framebuffer。
- 初始化 LTDC，扫描 800x480 framebuffer。
- 写入固定彩条测试图案。
- 打开 PB5 背光。
- 不接 LVGL。
- 不接触摸。
- 不使用 DMA2D 绘图。

### 官方参数来源

参数来自正点原子 Apollo H743 HAL 标准例程：

```text
D:\datafile\stm32\4，程序源码\2，标准例程-HAL库版本.zip
2，标准例程-HAL库版本/实验15 LTDC LCD（RGB屏）实验/Drivers/BSP/LCD/ltdc.c
2，标准例程-HAL库版本/实验15 LTDC LCD（RGB屏）实验/Drivers/BSP/LCD/ltdc.h
```

已确认 4.3 寸 800x480 RGB 屏参数：

```text
屏幕 ID：0x4384
分辨率：800x480
色彩格式：RGB565
HSW = 48
HBP = 88
HFP = 40
VSW = 3
VBP = 32
VFP = 13
PLL3M = 25
PLL3N = 300
PLL3R = 9
LTDC pixel clock = 33.333333MHz
背光：PB5，高电平打开
framebuffer：0xC0000000
```

RGB / LTDC GPIO 按官方例程配置：

```text
PB5  LCD_BL
PF10 LTDC_DE
PG7  LTDC_CLK
PI9  LTDC_VSYNC
PI10 LTDC_HSYNC
PG6, PG11
PH9, PH10, PH11, PH12, PH13, PH14, PH15
PI0, PI1, PI2, PI4, PI5, PI6, PI7
```

### 代码变更

新增或启用：

```text
App/Core/Inc/lcd.h
App/Core/Src/lcd.c
App/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc.c
App/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc_ex.c
```

修改：

```text
App/Core/Src/main.c
App/Core/Src/system_clock.c
App/Core/Inc/system_clock.h
App/Core/Inc/stm32h7xx_hal_conf.h
CMakeLists.txt
```

初始化顺序：

```text
ICache on，DCache off
SDRAM MPU 配置为 non-cacheable
HAL_Init
SystemClock_Config：PLL1 + PLL2/FMC + PLL3/LTDC
LED/UART 初始化
SDRAM 初始化
32MB SDRAM memtest
LTDC 初始化
写入 SDRAM 彩条 framebuffer
打开 PB5 背光
进入 500ms heartbeat
```

说明：当前仍保留 Phase 3 的 32MB 全容量 memtest，所以每次复位后大约等待 8 到 9 秒才会进入 LCD 初始化。这样可以先保证 framebuffer 所在 SDRAM 在接入 LTDC 前仍稳定。

### 编译验证

已执行：

```text
cmake --build --preset gcc-debug
```

编译通过，生成：

```text
build/gcc-debug/apollo_h743_lvgl_hmi.elf
build/gcc-debug/apollo_h743_lvgl_hmi.hex
build/gcc-debug/apollo_h743_lvgl_hmi.bin
build/gcc-debug/apollo_h743_lvgl_hmi.map
```

构建大小：

```text
FLASH used = 36672 bytes
RAM_D1 used = 6984 bytes
text = 36548
data = 116
bss = 6876
```

### 实板下载验证

下载命令：

```text
openocd -s <openocd_scripts> \
  -f interface/stlink.cfg \
  -f target/stm32h7x.cfg \
  -c "transport select swd" \
  -c "adapter speed 1000" \
  -c "program apollo_h743_lvgl_hmi/build/gcc-debug/apollo_h743_lvgl_hmi.hex verify reset exit"
```

下载结果：

```text
Target voltage: 3.283534
Device: STM32H74x/75x
flash size probed value 2048k
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
```

串口验证：

```text
串口设备：USB-SERIAL CH340 (COM5)
串口参数：115200, 8N1
```

抓到的关键启动日志：

```text
Phase 4: LTDC bare framebuffer color bars
LVGL: disabled in this phase
SYSCLK=400000000 Hz
HCLK  =200000000 Hz
APB1  =100000000 Hz
APB2  =100000000 Hz
APB3  =100000000 Hz
APB4  =100000000 Hz
PLL2_R=220000000 Hz
FMC   =220000000 Hz
SDCLK =110000000 Hz
PLL3_R=33333333 Hz
LTDC  =33333333 Hz
ICache=on, DCache=off
SDRAM base=0xC0000000 size=33554432 bytes
SDRAM memtest start: 33554432 bytes
SDRAM test pass: 33554432 bytes
LCD panel id=0x4384, RGB565, 800x480
LTDC timing: HSW=48 HBP=88 HFP=40 VSW=3 VBP=32 VFP=13
LTDC pixel clock: PLL3 M=25 N=300 R=9, 33333333 Hz
LCD framebuffer: 0xC0000000, 768000 bytes
LCD backlight: PB5 high
LCD init start
LCD init done: color bars written to SDRAM framebuffer
heartbeat tick=7869
```

串口侧结论：

- SDRAM 初始化仍通过。
- 32MB SDRAM memtest 仍通过。
- LTDC 参数打印正确。
- LTDC 初始化路径执行完成。
- heartbeat 持续输出，主循环未卡死。

### 用户肉眼确认

软件侧已经完成下载和串口验证。用户已确认 LCD 实际显示正常：

```text
屏幕亮起；
显示红、绿、蓝、黄、品红、青、白、黑彩条；
左上角有白色块和黑色内块；
无明显花屏、撕裂、错位、闪烁或整屏纯白/纯黑。
```

Phase 4 结论：通过。下一步进入 Phase 5：不接 LVGL，只做触摸裸驱动和串口坐标打印。

## Phase 5：触摸裸驱动

日期：2026-05-26

### 阶段目标

本阶段只验证触摸控制器本身，不接 LVGL：

- 初始化 4.3 寸 RGB 屏上的电容触摸控制器。
- 使用软件 I2C 读取触摸芯片 ID、配置区和坐标状态。
- 串口打印触摸按下、松开和坐标。
- 继续保留 SDRAM memtest 和 LTDC 彩条显示，确认触摸接入后基础显示链路不退化。

### 官方参考来源

触摸引脚和 GT9xxx 基础流程来自正点原子 Apollo H743 HAL 标准例程：

```text
D:\datafile\stm32\4，程序源码\2，标准例程-HAL库版本.zip
2，标准例程-HAL库版本/实验31 触摸屏实验/Drivers/BSP/TOUCH/ctiic.c
2，标准例程-HAL库版本/实验31 触摸屏实验/Drivers/BSP/TOUCH/ctiic.h
2，标准例程-HAL库版本/实验31 触摸屏实验/Drivers/BSP/TOUCH/gt9xxx.c
2，标准例程-HAL库版本/实验31 触摸屏实验/Drivers/BSP/TOUCH/gt9xxx.h
2，标准例程-HAL库版本/实验31 触摸屏实验/Drivers/BSP/TOUCH/touch.c
```

实际使用的触摸引脚：

```text
SCL = PH6
SDA = PI3
INT = PH7
RST = PI8
I2C 地址 = 0x14，对应写命令 0x28、读命令 0x29
```

### 当前代码变更

新增：

```text
App/Core/Inc/touch.h
App/Core/Src/touch.c
```

修改：

```text
App/Core/Src/main.c
CMakeLists.txt
```

当前主流程：

```text
ICache on，DCache off
SDRAM MPU non-cacheable
HAL_Init
SystemClock_Config
LED/UART 初始化
32MB SDRAM memtest
LTDC 初始化并显示彩条
GT1158 软件 I2C 初始化和寄存器诊断
主循环每 10ms 轮询触摸，每 500ms 输出 heartbeat
```

### 编译和下载

已执行：

```text
cmake --build --preset gcc-debug
```

当前诊断版构建结果：

```text
FLASH used = 42752 bytes
RAM_D1 used = 7024 bytes
text = 42604
data = 140
bss = 6892
```

已通过 OpenOCD 下载校验：

```text
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
```

### 实板诊断结果

触摸芯片已经能通过 I2C 稳定识别：

```text
Touch pid 0x8140: 31 31 35 38 00 01 01 01
Touch: address=0x14 write=0x28 read=0x29 pid=1158
```

结论：实际触摸控制器 PID 为 `1158`，按 Goodix GT1158 / GT1x 类处理更合理。

GT1158 的有效配置区不是旧 GT9xxx 例程中的 `0x8047` 起始，而是 `0x8050`：

```text
Touch cfg 0x8047: BF 02 48 BE CE 32 40 EF 00 82 20 03 E0 01 05 35
Touch cfg 0x8050: 82 20 03 E0 01 05 35 14 00 00 02 0C 5F 4B 35 01
Touch cfg: reg=0x8050 ver=0x82 x_max=800 y_max=480 touch_num=0x05 module_switch1=0x35
```

结论：

- `0x8050` 配置区有效。
- 分辨率配置为 `800x480`。
- 最大触点数为 5。
- I2C 多字节读写基本正常。

坐标状态寄存器当前结果：

```text
Touch coor 0x814E: 80 00 80 00 00 00 00 00 00
Touch raw status=0x80 req=0xFF int=0 tp1_raw_x=128 tp1_raw_y=0
Touch raw status=0x00 req=0xFF int=1 tp1_raw_x=128 tp1_raw_y=0
```

运行中多次抓取仍未看到触摸按下事件：

```text
Touch raw status=0x00 req=0xFF int=1 tp1_raw_x=128 tp1_raw_y=0
```

当前判断：

- 芯片在线，配置区正确。
- `0x814E` 事件位未在抓取窗口内变为 `0x81`、`0x82` 等有效触摸状态。
- INT 引脚空闲为高，启动清状态时会出现一次低电平，后续没有抓到触摸触发。
- `0x8044` 请求寄存器为 `0xFF`，表示当前未请求主机下发配置或执行复位。

### 官方 HEX 交叉验证

已从官方触摸实验解压 HEX：

```text
apollo_h743_lvgl_hmi/build/official_hex/official_touch_exp31_atk_h743.hex
```

并已临时烧录官方 `实验31 触摸屏实验`：

```text
** Programming Finished **
** Verify Started **
** Verified OK **
```

官方程序串口输出：

```text
CTP ID:1158
```

结论：官方触摸实验也能识别同一颗 `GT1158`。用户已确认官方触摸实验可以在屏幕上划线，说明触摸屏、FPC、GT1158、INT/RST/I2C 硬件链路正常。

验证后已经把本项目 Phase 5 诊断固件重新烧回开发板。

### Phase 5 当前结论

Phase 5 通过。

已确认：

- 触摸硬件 I2C 通路存在。
- 触摸芯片 PID 为 `1158`。
- I2C 地址为 `0x14`。
- GT1158 有效配置区为 `0x8050`，配置分辨率为 `800x480`。
- 官方触摸 HEX 也能识别 `CTP ID:1158`。
- 本项目裸驱动已经抓到 `0x814E=0x81` 有效按下事件。
- 本项目裸驱动已经输出 `TP down` / `TP up` 和坐标。
- 本项目裸 framebuffer 画点接口已接入，触摸有效时会在屏幕留下红色小点。

本项目实测串口节选：

```text
Touch raw status=0x81 int=0
Touch data 0x814F: 00 F9 00 EB 00 22 00 00 00
TP down x=564 y=249 raw_x=249 raw_y=235 points=1
Touch raw status=0x00 int=1
TP up
TP down x=583 y=321 raw_x=321 raw_y=216 points=1
TP down x=565 y=259 raw_x=259 raw_y=234 points=1
TP up
```

当前保留的 Phase 5 行为：

- 不接 LVGL。
- 使用软件 I2C 读取 GT1158。
- 串口打印有效触摸坐标。
- 有效触摸时在 framebuffer 上绘制 5x5 红色触摸点。

官方触摸实验已经能画线，本项目裸驱动也已抓到有效触摸坐标，因此硬件链路和 Phase 5 裸触摸路径均通过。后续进入 LVGL 输入设备接入前，需要保留当前裸驱动作为回归基线。

## Phase 6：LVGL 8.4.0 移植

日期：2026-05-26

### 阶段目标

本阶段目标是把 LVGL 接入已经通过的 SDRAM、LTDC 和触摸基础链路：

- 移植 LVGL 8.4.x。
- 配置 RGB565。
- 使用 SDRAM framebuffer。
- 使用 `800x40` 的 LVGL draw buffer。
- 实现 `disp_flush_cb`。
- 实现 touch indev。
- 创建 button、label、slider 测试页面。
- 暂时不使用 FreeRTOS。
- `lv_timer_handler()` 在 `while(1)` 中调用。
- `SysTick_Handler()` 中调用 `lv_tick_inc(1)`。

### LVGL 来源

本机已有 `D:\datafile\stm32\14，LVGL学习资料\lvgl-master.zip`，但该压缩包实际为 `lvgl-release-v8.2`，不满足本阶段的 8.4.x 要求。

已通过 GitHub 浅克隆获取官方 LVGL v8.4.0：

```text
git clone --depth 1 --branch v8.4.0 https://github.com/lvgl/lvgl.git build/downloads/lvgl-v8.4.0-src
```

导入到工程的源码目录：

```text
Middlewares/lvgl
```

已导入 `src/`、`lvgl.h`、`LICENCE.txt`、`README.md` 和 `VERSION.txt`。未把官方 docs、tests、examples 全量导入工程。

### 代码变更

新增：

```text
Middlewares/lvgl/
App/Core/Inc/lv_conf.h
App/Core/Inc/lvgl_port.h
App/Core/Src/lvgl_port.c
App/Core/Inc/ui_demo.h
App/Core/Src/ui_demo.c
```

修改：

```text
CMakeLists.txt
App/Core/Src/main.c
App/Core/Src/stm32h7xx_it.c
App/Core/Inc/touch.h
App/Core/Src/touch.c
```

当前 LVGL 配置：

```text
LVGL version = 8.4.0
LV_COLOR_DEPTH = 16
LV_COLOR_16_SWAP = 0
LV_MEM_SIZE = 128 KiB
LVGL draw buffer = 800 * 40 pixels = 64000 bytes
framebuffer = 0xC0000000，800 * 480 * 2 = 768000 bytes
FreeRTOS = disabled
DMA2D = disabled
DCache = off
SDRAM MPU = non-cacheable, non-bufferable, execute-never
```

显示适配：

```text
LTDC 持续扫描 SDRAM framebuffer：0xC0000000
LVGL draw buffer 放在 RAM_D1
disp_flush_cb 使用 CPU memcpy 将脏区域拷贝到 SDRAM framebuffer
flush 完成后调用 lv_disp_flush_ready()
```

触摸适配：

```text
新增 app_touch_read(app_touch_sample_t *sample)
LVGL indev 类型：LV_INDEV_TYPE_POINTER
indev read_cb 复用 Phase 5 已验证的 GT1158 软件 I2C 读坐标路径
Goodix 状态清除顺序已调整为：先读坐标数据，再清 0x814E
```

测试页面：

```text
标题 label：Apollo H743 LVGL 8.4.0
状态 label：显示按钮点击计数和 slider 百分比
button：点击后计数增加
slider：0..100，可拖动并更新状态 label
```

### 编译结果

已执行：

```text
cmake --build --preset gcc-debug --target clean
cmake --build --preset gcc-debug
```

编译通过，生成：

```text
build/gcc-debug/apollo_h743_lvgl_hmi.elf
build/gcc-debug/apollo_h743_lvgl_hmi.hex
build/gcc-debug/apollo_h743_lvgl_hmi.bin
build/gcc-debug/apollo_h743_lvgl_hmi.map
```

当前构建大小：

```text
FLASH used = 232100 bytes
RAM_D1 used = 203320 bytes
text = 231920
data = 172
bss = 203156
```

### 下载和串口验证

已使用 OpenOCD + ST-LINK 下载并校验通过：

```text
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
```

串口抓到的关键启动日志：

```text
Phase 6: LVGL 8.4.0 RGB565 framebuffer + touch
FreeRTOS: disabled
SDRAM test pass: 33554432 bytes
LCD panel id=0x4384, RGB565, 800x480
Touch: address=0x14 write=0x28 read=0x29 pid=1158
Touch: config register=0x8050 max_points=5
LVGL version: 8.4.0
LVGL color: RGB565, LV_COLOR_DEPTH=16, LV_COLOR_16_SWAP=0
LVGL draw buffer: 800x40 pixels, 64000 bytes, single buffer
LVGL framebuffer: SDRAM 0xC0000000, 768000 bytes
LVGL tick: lv_tick_inc(1) in SysTick_Handler
LVGL loop: lv_timer_handler() in while(1), no FreeRTOS
LVGL init done: button/label/slider test page created
```

串口侧已确认：

- SDRAM 32MB memtest 仍通过。
- LTDC / LCD 初始化路径仍执行完成。
- GT1158 仍能识别 PID 和配置寄存器。
- LVGL v8.4.0 初始化完成。
- LVGL 显示驱动、draw buffer、framebuffer 地址配置已打印确认。
- `lv_timer_handler()` 主循环路径运行。
- LVGL indev 读取回调在运行，触摸坐标能进入 LVGL。

### 屏幕和触摸实测确认

屏幕显示：

- 用户现场确认测试页面可以看到。
- 测试页面包含标题、button、label、slider。

触摸进入 LVGL 后已抓到有效事件：

```text
Touch diag status=0x81 int=0
LVGL TP down x=535 y=249
LVGL slider value=54

Touch diag status=0x80 int=0
LVGL TP up
```

补充触摸事件：

```text
Touch diag status=0x81 int=0
LVGL TP down x=532 y=232
```

收尾处理：

- 已移除临时低频诊断日志 `Touch diag status=...`。
- 已移除临时 LVGL 轮询计数日志 `LVGL indev poll count=...`。
- 保留 `LVGL TP down/up`、button 和 slider 事件日志，便于后续观察交互。

清理临时诊断日志后已重新编译并下载：

```text
cmake --build --preset gcc-debug

** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
```

清理后的固件复测串口日志：

```text
LVGL TP down x=576 y=260
LVGL slider value=66
LVGL TP up

LVGL TP down x=550 y=252
LVGL slider value=58
LVGL TP up
```

清理后的串口日志中未再出现 `Touch diag status=...` 和 `LVGL indev poll count=...` 周期诊断刷屏。

因此 Phase 6 当前状态为：

```text
LVGL 8.4.0 移植：已完成
RGB565 配置：已完成
SDRAM framebuffer：已完成
800x40 draw buffer：已完成
disp_flush_cb：已完成并通过启动路径
touch indev：已实现并通过实测
button/label/slider 页面：已创建，屏幕显示已确认，slider 事件已确认
Phase 6 总体验收：已完成
```

屏幕和触摸均已通过，可以进入下一阶段前的代码收尾和稳定性复测。

## Phase 7-9：FreeRTOS、W25Q256 参数保存和 HMI 页面

### 实现范围

本阶段在 Phase 6 的 LVGL 显示和触摸链路基础上接入 FreeRTOS，并完成第一版 HMI 小项目。

新增模块：

```text
Middlewares/FreeRTOS/Source/
App/Core/Inc/FreeRTOSConfig.h
App/Core/Inc/app_tasks.h
App/Core/Src/app_tasks.c
App/Core/Inc/app_w25q256.h
App/Core/Src/app_w25q256.c
App/Core/Inc/app_settings.h
App/Core/Src/app_settings.c
App/Core/Inc/app_storage.h
App/Core/Src/app_storage.c
App/Core/Inc/app_crc32.h
App/Core/Src/app_crc32.c
App/Core/Inc/app_log.h
App/Core/Src/app_log.c
App/Core/Inc/app_ui_hmi.h
App/Core/Src/app_ui_hmi.c
```

移除 Phase 6 临时测试页：

```text
App/Core/Inc/ui_demo.h
App/Core/Src/ui_demo.c
```

### FreeRTOS 接入

FreeRTOS 内核来源：

```text
C:\Users\Luo\STM32Cube\Repository\STM32Cube_FW_H7_V1.9.1\Middlewares\Third_Party\FreeRTOS\Source
```

当前任务：

| 任务 | 优先级 | 栈大小 | 职责 |
|---|---:|---:|---|
| `task_gui` | `tskIDLE_PRIORITY + 3` | 3072 words / 12 KiB | 初始化 LVGL、创建页面、调用 `lv_timer_handler()` 和 `lv_tick_inc()` |
| `task_storage` | `tskIDLE_PRIORITY + 1` | 1024 words / 4 KiB | W25Q256 参数读取、延迟保存、读回校验 |
| `task_log` | `tskIDLE_PRIORITY + 1` | 1024 words / 4 KiB | LED 心跳、周期打印任务栈余量和参数快照 |

LVGL 调用规则：

```text
LVGL API 只在 task_gui 及其直接调用的 UI/port 回调中执行。
SysTick_Handler 不再直接调用 lv_tick_inc。
task_gui 根据 HAL_GetTick() 的 elapsed 时间调用 lv_tick_inc(elapsed)。
task_gui 循环调用 lv_timer_handler()。
```

### W25Q256 参数保存

QSPI / W25Q256 参考来源：

```text
D:\datafile\stm32\4，程序源码\2，标准例程-HAL库版本.zip
2，标准例程-HAL库版本\实验28 QSPI实验
```

当前实测：

```text
W25Q256 JEDEC ID = 0xEF4019
QSPI 时钟 = D1HCLK / 2 = 100MHz
QSPI 引脚 = PB2 CLK、PB6 NCS、PF6 IO0、PF7 IO1、PF8 IO2、PF9 IO3
参数区 = 外部 Flash 0x00000000 - 0x0000FFFF，64KB
访问方式 = 普通命令模式，不启用 memory mapped
```

参数块结构满足要求：

```c
typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t length;
    uint32_t crc32;
    uint8_t payload[512];
} app_workset_block_t;
```

保存策略：

```text
上电读取参数区。
校验 magic/version/length/crc32。
校验通过则加载 Flash 参数。
校验失败则加载默认参数，并请求保存默认参数。
UI 修改参数后只置 dirty，不立即擦写。
默认延迟 3000ms 后由 task_storage 擦除 4KB sector、page program 写入、读回校验。
手动 Save 按钮会请求立即保存。
```

### HMI 页面

当前已创建 4 个页面：

```text
Home：设备状态、运行时间、模拟电压/电流/温度、当前参数。
Params：Brightness、Threshold、Output ON/OFF、Save。
Upgrade：当前版本、新版本、模拟升级进度、校验状态。
Log：最近事件列表。
```

### 编译结果

最新构建：

```text
cmake --build --preset gcc-debug
```

编译通过，生成：

```text
build/gcc-debug/apollo_h743_lvgl_hmi.elf
build/gcc-debug/apollo_h743_lvgl_hmi.hex
build/gcc-debug/apollo_h743_lvgl_hmi.bin
build/gcc-debug/apollo_h743_lvgl_hmi.map
```

当前构建大小：

```text
FLASH used = 255848 bytes
RAM_D1 used = 303960 bytes
text = 255648
data = 192
bss = 303776
```

### 下载和复位验证

已使用 OpenOCD + ST-LINK 下载并校验通过：

```text
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
```

复位启动日志关键片段：

```text
Phase 7-9: FreeRTOS + W25Q256 settings + HMI pages
FreeRTOS: enabled, LVGL only in task_gui
QSPI  =D1HCLK/2 = 100000000 Hz
Storage init start
Storage init done: settings loaded from W25Q256
FreeRTOS scheduler start
task_gui started
LVGL version: 8.4.0
LVGL tick: lv_tick_inc(elapsed) in task_gui
LVGL loop: lv_timer_handler() in task_gui, FreeRTOS enabled
LVGL init done: HMI pages created in task_gui
```

W25Q256 首次空参数区时的保存验证：

```text
storage status: flash=1 id=0xEF4019 load=bad_magic save_ok=1 save_fail=0 dirty=0
settings snapshot: brightness=70 threshold=55 output=1 language=0
```

复位后参数读回验证：

```text
storage status: flash=1 id=0xEF4019 load=flash_ok save_ok=0 save_fail=0 dirty=0
settings snapshot: brightness=70 threshold=55 output=1 language=0
```

参数修改后延迟保存验证：

```text
storage: phase8 save selftest scheduled, delayed_save=3000ms
storage: save workset ok count=1
storage status: flash=1 id=0xEF4019 load=flash_ok save_ok=1 save_fail=0 dirty=0
settings snapshot: brightness=71 threshold=55 output=1 language=0 selftest=0x53385431
```

再次复位后保持验证：

```text
storage status: flash=1 id=0xEF4019 load=flash_ok save_ok=0 save_fail=0 dirty=0
settings snapshot: brightness=71 threshold=55 output=1 language=0 selftest=0x53385431
```

任务栈余量周期打印示例：

```text
stack watermark words: gui=2486 storage=904 log=839 idle=104 heap_free=76760
```

当前结论：

```text
FreeRTOS 调度：已通过
task_gui/task_storage/task_log：已创建并运行
LVGL 直接调用限制：已收敛到 task_gui 路径
W25Q256 识别：已通过，JEDEC ID=0xEF4019
参数块 magic/version/length/crc32：已实现
空参数区默认值保存：已通过
复位后参数读回：已通过
参数修改后延迟保存：已通过，延迟 3000ms 后保存，复位后保持
周期打印任务栈余量：已通过
首页/参数页/升级页/日志页：已实现，等待用户肉眼确认显示和触摸操作
```

注意：当前串口输出由多个任务直接 `printf`，偶发日志字符交织；功能不受影响，后续可增加 UART 输出互斥锁让日志更整齐。

## 2026-05-26 Phase 7-9 页签触摸问题修复

### 现象

HMI 画面可以正常显示，但点击顶部 `Home / Params / Upgrade / Log` 页签时页面不切换，串口最初没有看到 `UI page=` 日志。

### 定位过程

临时加入 LVGL 输入设备轮询日志、GT9xxx 状态日志和原始坐标日志后确认：

```text
LVGL indev：持续轮询
GT9xxx：能读到按下/松开事件
LVGL TP down：能打印触摸坐标
UI page=：未触发
```

因此问题不在 FreeRTOS 调度，也不在页面切换函数，而是触摸坐标没有落到顶部页签对象上。

对照正点原子 LVGL 例程中 GT9xxx 的横屏映射逻辑后确认：本项目使用的是 `RGB4384` 4.3 寸 RGB 屏，触摸坐标应按横屏直通处理：

```text
x = raw_x
y = raw_y
```

之前使用了 `x=800-raw_y, y=raw_x`，该映射适用于另一类屏幕方向，会导致顶部页签触摸被映射到错误位置。

### 修复内容

已修改 `App/Core/Src/touch.c`：

```text
Touch map: RGB4384 landscape, x=raw_x, y=raw_y, range=0..799/0..479
```

同时将顶部页签按钮高度从 `44` 调整为 `56`，提高边缘点击容错。

### 验证结果

重新编译、烧录、OpenOCD verify 通过。串口点击页签验证结果：

```text
LVGL TP down x=319 y=38
LVGL TP up
UI page=1
LVGL TP down x=492 y=34
LVGL TP up
UI page=2
LVGL TP down x=683 y=29
LVGL TP up
UI page=3
LVGL TP down x=114 y=12
LVGL TP up
UI page=0
```

当前结论：

```text
触摸输入链路：已通过
页签点击命中：已通过
Home / Params / Upgrade / Log 页面切换：已通过
```

## 2026-05-27 Phase 10-11 Boot/App 与本地 IAP 第一版

### 目标

本阶段从现有 HMI App 拆出 Bootloader 和 App 两个独立构建目标：

```text
Bootloader：apollo_h743_bootloader
AppA：      apollo_h743_app
```

Bootloader 固定运行在 `0x08000000`，AppA 槽从 `0x08020000` 开始，AppA 实际运行地址为 `0x08020400`。

### 已完成内容

已新增公共分区、tag 和 IAP pending 定义：

```text
Common/Inc/flash_layout.h
Common/Inc/app_tag.h
Common/Inc/app_crc32.h
Common/Inc/app_iap_record.h
Common/Src/app_tag.c
Common/Src/app_crc32.c
Common/Src/app_iap_record.c
```

已新增 Bootloader 工程：

```text
Boot/Core/Src/main.c
Boot/Core/Src/boot_jump.c
Boot/Core/Src/boot_iap.c
Boot/Linker/STM32H743IITx_BOOT.ld
```

已新增 AppA 链接脚本和启动向量表重定位：

```text
App/Linker/STM32H743IITx_APP_A.ld
App/Core/Src/app_vector.c
```

App 启动后将 Flash 向量表复制到 RAM_D1 的 `.ram_vector`，然后设置 `SCB->VTOR` 到 RAM 向量表。

已新增本地 IAP 第一版框架：

```text
App/Core/Src/app_iap.c
App/Core/Inc/app_iap.h
task_iap
```

串口命令第一版：

```text
iap status
iap demo
iap demo usb
iap demo sd
iap help
```

串口、USB、SD 卡三种本地 IAP 入口都已在 App 层建好状态和日志入口。当前只有串口命令与 HMI 升级页会触发 demo staging；USB 和 SD 卡后端暂未挂载文件系统/USB 协议栈，第一版作为保留入口，不擦写 AppA。

### 构建验证

执行：

```text
cmake --preset gcc-debug
cmake --build --preset gcc-debug
```

结果：

```text
Bootloader FLASH used = 39460 bytes / 128 KiB
AppA FLASH used       = 263304 bytes / 0x000DFC00
AppA RAM_D1 used      = 306200 bytes / 512 KiB
AppA CRC32            = 0xA6F05A8E
```

AppA 打包校验：

```text
slot_size=264328
magic=0x41505447
tag_size=64
run=0x08020400
app_size=263304
version=1
crc=0xA6F05A8E
calc=0xA6F05A8E
```

### 预期启动日志

烧录 Bootloader 和 AppA 槽镜像后，串口必须能看到 Boot 到 App 的顺序：

```text
BOOT: Apollo H743 bootloader start
BOOT IAP: check W25Q pending flag ...
BOOT: check AppA tag at 0x080203C0
BOOT: AppA crc calc=... expect=...
BOOT: AppA valid, version=1
BOOT: jump to App at 0x08020400
APP: vector copied to RAM ...
APP: start from 0x08020400
Phase 10-11 AppA: FreeRTOS + W25Q256 settings + HMI pages + local IAP
```

### 安全边界

第一版只做 CRC32，不做 SHA256 和签名校验，不做 Option Byte Bank Swap。

App 侧 IAP 只写 W25Q256 staging 区和 pending 记录，不直接擦 AppA。Boot 第一版只读取 pending 并复算 staging CRC32，不复制覆盖 AppA。因此 pending 无效、staging CRC 错误、W25Q 不可用或升级流程中断时，旧 AppA 都保持不变，Boot 继续校验并跳转旧 AppA。

### 实板验证结果

OpenOCD 烧录命令已验证通过：

```text
program build/gcc-debug/apollo_h743_bootloader.hex verify
program build/gcc-debug/app_a_slot.hex verify reset exit
```

串口实测 Boot 到 App 顺序：

```text
BOOT: Apollo H743 bootloader start
BOOT IAP: check W25Q pending flag at 0x01A00000
BOOT: check AppA tag at 0x080203C0
BOOT: tag magic=0x41505447 size=64 run=0x08020400 app_size=263304 version=1 crc=0xA6F05A8E
BOOT: AppA crc calc=0xA6F05A8E expect=0xA6F05A8E
BOOT: AppA valid, version=1
BOOT: vector msp=0x24080000 reset=0x0802786D
BOOT: jump to App at 0x08020400
APP: vector copied to RAM src=0x08020400 dst=0x24000000 bytes=1024
APP: start from 0x08020400
Phase 10-11 AppA: FreeRTOS + W25Q256 settings + HMI pages + local IAP
```

串口 IAP 命令实测：

```text
IAP status: flash=1 pending=0 version=0 size=0 crc=0x00000000 state=IAP idle
IAP serial: demo staging requested
IAP serial: staging demo package to AppB staging area
IAP serial: demo package verified, pending flag set version=2 size=1024 crc=0x6FEA9368
IAP: reset board to let Boot inspect pending flag; first build will not erase AppA
IAP status: flash=1 pending=1 version=2 size=1024 crc=0x6FEA9368 state=demo staged, pending set
```

复位后 Boot 读取 pending 并校验 W25Q staging：

```text
BOOT IAP: pending magic=0x50414950 version=1 target=1 size=1024 app_ver=2 crc=0x6FEA9368 staging=0x01A01000
BOOT IAP: staging crc calc=0x6FEA9368 expect=0x6FEA9368
BOOT IAP: staged AppB package CRC OK
BOOT IAP: first CRC32 build does not copy over AppA; AppA remains untouched
BOOT: AppA crc calc=0xA6F05A8E expect=0xA6F05A8E
BOOT: jump to App at 0x08020400
```

结论：

```text
Bootloader 工程：已创建并实板运行
App 工程：已迁移到 0x08020400 并实板运行
app_tag_t：已生成、烧录并由 Boot 校验通过
App 向量表复制到 RAM + SCB->VTOR：已通过，VTOR=0x24000000
串口本地 IAP demo staging：已通过
USB/SD 本地 IAP：入口和状态已保留，后端未挂载
失败不破坏旧 App：当前实现不擦 AppA，Boot 校验旧 AppA 后继续跳转
```

## 2026-05-30 Phase 11 串口真实包 IAP 补齐

### 本轮补齐内容

在已有 Boot/AppA、`app_tag_t`、W25Q256 pending/staging 框架基础上，补齐串口真实固件包接收链路。

新增串口命令：

```text
iap recv <size> <crc32> <version>
```

App 侧流程：

```text
1. task_iap 收到 iap recv 命令。
2. 校验 size/version，擦除 W25Q256 staging 区。
3. 打印 ready for binary。
4. 接收固定 size 字节二进制数据。
5. 分 256 字节缓冲写入 W25Q256 staging。
6. 边接收边计算流式 CRC32。
7. 接收完成后从 W25Q256 读回 staging，再计算一次 CRC32。
8. 两次 CRC32 都通过后写 pending 记录。
9. 复位后 Boot 读取 pending，并复算 staging CRC32。
```

新增 PC 发送工具：

```text
Tools/iap_send_serial/iap_send_serial.py
```

使用方式：

```text
python Tools/iap_send_serial/iap_send_serial.py --port COM5 --baud 115200 --file build/gcc-debug/app_a_slot.bin --version 2
```

当前推荐发送 `app_a_slot.bin`，该文件包含 `0x400` 槽头和 `app_tag_t`，后续 Boot 真正复制到内部槽时可以沿用这个包格式。

### 代码改动

```text
App/Core/Src/app_iap.c
App/Core/Inc/app_iap.h
App/Core/Src/uart.c
App/Core/Src/app_tasks.c
App/Core/Src/app_ui_hmi.c
Tools/iap_send_serial/iap_send_serial.py
Docs/iap_design.md
```

UART1 RX ring buffer 从 256 字节扩大到 2048 字节，降低二进制包传输时任务调度导致丢字节的风险。

IAP 状态新增接收进度：

```text
recv_active
recv_received
recv_expected
```

周期任务栈日志和 Upgrade 页面都会显示接收进度。

### 构建验证

执行：

```text
cmake --build --preset gcc-debug
```

结果：

```text
Bootloader FLASH used = 39464 bytes / 128 KiB
AppA FLASH used       = 268808 bytes / 895 KiB
AppA RAM_D1 used      = 308808 bytes / 512 KiB
AppA CRC32            = 0xD39AF875
```

生成产物：

```text
build/gcc-debug/apollo_h743_bootloader.hex
build/gcc-debug/apollo_h743_app.hex
build/gcc-debug/app_a_slot.hex
build/gcc-debug/app_a_slot.bin
build/gcc-debug/app_a_tag.bin
```

### 当前安全边界

当前仍不执行内部 Flash 覆盖。App 只写 W25Q256 staging 和 pending；Boot 只校验 pending 与 staging CRC32，然后继续跳转旧 AppA。

因此，串口传输中断、CRC32 错误、W25Q 写入失败、pending 无效，都不会破坏旧 AppA。

### 实板验证

烧录命令：

```text
program build/gcc-debug/apollo_h743_bootloader.hex verify
program build/gcc-debug/app_a_slot.hex verify reset exit
```

OpenOCD verify 通过。

串口发送真实包：

```text
python Tools/iap_send_serial/iap_send_serial.py --port COM5 --baud 115200 --file build/gcc-debug/app_a_slot.bin --version 2
```

本轮发送包：

```text
file=build/gcc-debug/app_a_slot.bin
size=269832
crc32=0x735E2D87
version=2
```

App 侧周期状态确认：

```text
iap status: flash=1 pending=1 version=2 size=269832 crc=0x735E2D87 recv=0 269832/269832 state=serial staged, pending set
```

复位后 Boot 校验 staging：

```text
BOOT IAP: pending magic=0x50414950 version=1 target=1 size=269832 app_ver=2 crc=0x735E2D87 staging=0x01A01000
BOOT IAP: staging crc calc=0x735E2D87 expect=0x735E2D87
BOOT IAP: staged AppB package CRC OK
BOOT IAP: first CRC32 build does not copy over AppA; AppA remains untouched
BOOT: AppA crc calc=0xD39AF875 expect=0xD39AF875
BOOT: jump to App at 0x08020400
```

结论：

```text
串口真实包接收：已通过
W25Q256 staging 写入和读回 CRC32：已通过
pending 记录写入：已通过
Boot 复位读取 pending 并校验 staging CRC32：已通过
失败不破坏旧 AppA：保持通过，当前仍不覆盖内部 Flash
```

## 2026-05-31 C# 串口 IAP 上位机

### 已完成内容

新增 C# WinForms 图形界面工具：

```text
Tools/iap_serial_gui/ApolloIapSerialGui.csproj
Tools/iap_serial_gui/MainForm.cs
Tools/iap_serial_gui/SerialIapTransfer.cs
Tools/iap_serial_gui/Crc32.cs
Tools/iap_serial_gui/README.md
```

界面能力：

```text
串口枚举，默认 115200
打开串口时 DTR=false、RTS=false
选择 bin 文件并自动计算 CRC32
发送 iap recv <size> <crc32> <version>
等待 ready for binary 后分块发送二进制
显示发送进度和串口日志
支持 iap status、取消、清空日志、保存日志
```

为了自动验证 C# 传输核心逻辑，同时新增控制台发送器：

```text
Tools/iap_serial_cli/ApolloIapSerialCli.csproj
Tools/iap_serial_cli/Program.cs
```

CLI 和 GUI 复用同一个 `SerialIapTransfer`，协议实现一致。

### 构建验证

执行：

```text
dotnet build Tools/iap_serial_gui/ApolloIapSerialGui.csproj -c Release
dotnet build Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release
```

结果：

```text
GUI build：0 warning / 0 error
CLI build：0 warning / 0 error
```

GUI 发布：

```text
dotnet publish Tools/iap_serial_gui/ApolloIapSerialGui.csproj -c Release -r win-x64 --self-contained false -o Tools/iap_serial_gui/publish
```

可运行文件：

```text
Tools/iap_serial_gui/publish/ApolloIapSerialGui.exe
```

### 实板验证

使用 C# CLI 发送同一个 AppA slot 包：

```text
dotnet run --project Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release -- --port COM5 --baud 115200 --file build/gcc-debug/app_a_slot.bin --version 3 --chunk 128 --delay-ms 2
```

发送包：

```text
file=build/gcc-debug/app_a_slot.bin
size=269832
crc32=0x735E2D87
version=3
```

App 侧验证：

```text
IAP serial: recv begin size=269832 crc=0x735E2D87 version=3
IAP serial: ready for binary size=269832
IAP serial: recv done size=269832 crc=0x735E2D87
IAP serial: package verified, pending flag set version=3 size=269832 crc=0x735E2D87
iap status: flash=1 pending=1 version=3 size=269832 crc=0x735E2D87 recv=0 269832/269832 state=serial staged, pending set
```

复位后 Boot 校验：

```text
BOOT IAP: pending magic=0x50414950 version=1 target=1 size=269832 app_ver=3 crc=0x735E2D87 staging=0x01A01000
BOOT IAP: staging crc calc=0x735E2D87 expect=0x735E2D87
BOOT IAP: staged AppB package CRC OK
BOOT IAP: first CRC32 build does not copy over AppA; AppA remains untouched
BOOT: AppA crc calc=0xD39AF875 expect=0xD39AF875
BOOT: jump to App at 0x08020400
```

结论：

```text
C# 串口传输核心逻辑：已实板通过
C# GUI：已构建和启动验证，协议逻辑与 CLI 共用
当前升级边界：仍只写 W25Q staging，不覆盖内部 AppA
```

## 2026-05-31 Phase 11 AppB 写入与跳转链路

### 本轮目标

补齐 Phase 11 中“App tag 和本地 IAP”的后半段：生成真正的 AppB 包，App 侧接收并校验 AppB slot 包，Boot 从 W25Q256 staging 写入内部 Flash AppB，校验后优先跳转 AppB，失败时仍回退 AppA。

### 代码变更

新增 / 修改：

```text
App/Linker/STM32H743IITx_APP_B.ld
CMakeLists.txt
Boot/Core/Src/main.c
Boot/Core/Src/boot_iap.c
App/Core/Src/main.c
App/Core/Src/app_vector.c
App/Core/Src/app_iap.c
Tools/iap_serial_gui/MainForm.cs
Tools/iap_serial_cli/Program.cs
Docs/iap_design.md
Docs/memory_map.md
```

主要行为：

```text
1. CMake 同时构建 AppA 和 AppB。
2. AppA 链接地址 0x08020400，版本 1，输出 app_a_slot.bin / app_a_slot.hex。
3. AppB 链接地址 0x08100400，版本 2，输出 app_b_slot.bin / app_b_slot.hex。
4. App 启动日志打印当前槽位、运行地址和版本。
5. App 接收串口 IAP 包前先清除旧 pending，避免旧 pending 指向新 staging。
6. App 接收完成后检查包内 app_tag_t，必须是 AppB 包，且命令版本与 tag version 一致。
7. Boot 处理 pending 时先校验 staging CRC32 和 AppB tag。
8. Boot 只擦写内部 Flash Bank2 的 AppB 槽，不擦 AppA。
9. Boot 写入 AppB 后读回比对，再校验 AppB tag 和 App 本体 CRC32。
10. Boot 清除 pending，启动策略为 AppB 优先、AppA 回退。
```

### 当前构建产物

```text
build/gcc-debug/apollo_h743_bootloader.hex
build/gcc-debug/app_a_slot.hex
build/gcc-debug/app_a_slot.bin
build/gcc-debug/app_b_slot.hex
build/gcc-debug/app_b_slot.bin
build/gcc-debug/app_a_tag.bin
build/gcc-debug/app_b_tag.bin
```

当前推荐升级文件：

```text
build/gcc-debug/app_b_slot.bin
```

上位机 / CLI 发送时版本号使用：

```text
version=2
```

CLI 示例：

```text
dotnet run --project Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release -- --port COM5 --baud 115200 --file build/gcc-debug/app_b_slot.bin --version 2 --chunk 128 --delay-ms 2
```

### 构建和静态验证

执行：

```text
cmake --build --preset gcc-debug
dotnet build Tools/iap_serial_gui/ApolloIapSerialGui.csproj -c Release
dotnet build Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release
```

结果：

```text
Firmware build：通过
C# GUI build：0 warning / 0 error
C# CLI build：0 warning / 0 error
```

当前固件大小：

```text
Bootloader FLASH used = 43616 bytes / 128 KiB
AppA FLASH used       = 270384 bytes / 895 KiB
AppB FLASH used       = 270384 bytes / 1023 KiB
App RAM_D1 used       = 308808 bytes / 512 KiB
```

slot 包静态校验：

```text
app_a_slot.bin: size=271408 slot_crc=0x19092B3C
  tag: magic=0x41505447 run=0x08020400 app_size=270384 version=1 app_crc=0x2AAA74B0
  body_crc=0x2AAA74B0

app_b_slot.bin: size=271408 slot_crc=0xF3FCC623
  tag: magic=0x41505447 run=0x08100400 app_size=270384 version=2 app_crc=0x387FBED3
  body_crc=0x387FBED3
```

### 预期串口顺序

首次烧录 Boot + AppA 后：

```text
BOOT: Apollo H743 bootloader start
BOOT IAP: check W25Q pending flag at 0x01A00000
BOOT: check AppB tag ...
BOOT: AppB tag invalid
BOOT: AppB unavailable, try AppA fallback
BOOT: AppA valid, version=1
BOOT: jump to App at 0x08020400
APP: AppA start from 0x08020400 version=1
```

发送 `app_b_slot.bin` 并复位后：

```text
BOOT IAP: pending magic=0x50414950 ...
BOOT IAP: staging crc calc=...
BOOT IAP: staged tag ... run=0x08100400 version=2
BOOT IAP: erase/program internal AppB only, AppA untouched
BOOT IAP: AppB flash readback OK
BOOT IAP: AppB update applied and verified, AppA untouched
BOOT: pending package applied to AppB
BOOT: AppB valid, version=2
BOOT: jump to App at 0x08100400
APP: AppB start from 0x08100400 version=2
```

### 当前边界

已完成串口本地 IAP 到 AppB 的第一版 CRC32 链路。USB/SD 仍是预留入口，尚未接真实文件后端；还没有 confirmed/rollback 状态机，也没有 SHA256/签名校验。

### 实板验证结果

烧录 Boot + AppA：

```text
program build/gcc-debug/apollo_h743_bootloader.hex verify
program build/gcc-debug/app_a_slot.hex verify reset exit
```

OpenOCD 结果：

```text
apollo_h743_bootloader.hex：Verified OK
app_a_slot.hex：Verified OK
```

通过 C# CLI 发送 AppB 升级包：

```text
dotnet run --project Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release -- --port COM5 --baud 115200 --file build/gcc-debug/app_b_slot.bin --version 2 --chunk 128 --delay-ms 2 --app-timeout-ms 120000 --ready-timeout-ms 30000 --log-tail-ms 10000
```

发送包：

```text
file=build/gcc-debug/app_b_slot.bin
size=271408
crc32=0xF3FCC623
version=2
```

AppA 侧接收验证通过：

```text
IAP serial: recv done size=271408 crc=0xF3FCC623
IAP serial: staged tag magic=0x41505447 run=0x08100400 app_size=270384 version=2 app_crc=0x387FBED3
IAP serial: package verified, pending flag set version=2 size=271408 crc=0xF3FCC623
iap status: flash=1 pending=1 version=2 size=271408 crc=0xF3FCC623 recv=0 271408/271408 state=serial staged, pending set
```

复位后 Boot 写入 AppB 并跳转 AppB：

```text
BOOT IAP: pending magic=0x50414950 version=1 target=1 size=271408 app_ver=2 crc=0xF3FCC623 staging=0x01A01000
BOOT IAP: staging crc calc=0xF3FCC623 expect=0xF3FCC623
BOOT IAP: staged tag magic=0x41505447 size=64 run=0x08100400 app_size=270384 version=2 crc=0x387FBED3
BOOT IAP: staged AppB package CRC OK, version=2 app_size=270384
BOOT IAP: erase/program internal AppB only, AppA untouched
BOOT IAP: AppB erase OK sectors=8
BOOT IAP: AppB flash readback OK bytes=271408
BOOT IAP: AppB crc calc=0x387FBED3 expect=0x387FBED3
BOOT IAP: pending flag cleared
BOOT IAP: AppB update applied and verified, AppA untouched
BOOT: AppB valid, version=2
BOOT: jump to App at 0x08100400
APP: AppB start from 0x08100400 version=2
```

自动检查结果：

```text
PASS BOOT IAP: AppB update applied and verified
PASS BOOT: AppB valid, version=2
PASS BOOT: jump to App at 0x08100400
PASS APP: AppB start from 0x08100400 version=2
```

结论：

```text
Phase 11 串口本地 IAP 到 AppB：实板通过
AppB tag + CRC32 校验：实板通过
Boot 写内部 Flash Bank2：实板通过
Boot AppB 优先跳转：实板通过
失败不覆盖 AppA：保持通过，升级过程中只擦写 AppB
```

## 2026-05-31 Phase 11 confirmed/rollback 状态机补齐

目标：在已有 AppB 真实包 IAP 基础上补齐试运行确认和回退策略，避免 Boot 只因为 AppB 有效就永久优先启动 AppB。

本轮修改：

1. W25Q256 IAP 区拆分为三个区域：

```text
0x01A00000 - 0x01A00FFF   pending 记录
0x01A01000 - 0x01A01FFF   boot state 记录
0x01A02000 - 0x01FFFFFF   staging 数据区
```

2. 新增 `app_iap_boot_state_t`，包含 `magic/version/length/state/active_slot/trial_slot/trial_version/boot_attempts/max_boot_attempts/last_error/crc32`。
3. Boot 安装 AppB 成功后不再直接永久优先启动 AppB，而是写入 `trial` 状态，最大试启动次数为 3。
4. Boot 启动选择策略调整为：

```text
confirmed + active=AppB        -> 校验并启动 AppB
trial + trial=AppB + 次数未满 -> 递增 attempts，校验并启动 AppB
trial 次数耗尽或 AppB 无效   -> 写 rollback，启动 AppA
无有效 boot state 或 rollback -> 启动 AppA
```

5. AppB 启动后，如果读到匹配当前版本的 `trial` 状态，会写入 `confirmed`，串口输出：

```text
IAP confirm: AppB version=2 confirmed
```

6. `iap status`、周期任务栈日志和升级页增加 boot state 状态显示。
7. C# 串口 IAP 工具提示改为：发送完成只代表包进入 staging；复位后 Boot 安装 AppB trial，AppB 启动成功后 confirmed。

保持不变：

- 第一版仍只做 CRC32。
- 不做 Option Byte Bank Swap。
- AppA 不被 App 或 Boot 擦写，仍作为回退槽。
- USB/SD 本地 IAP 后端仍为预留入口。

验证结果：

```text
cmake --build --preset gcc-debug                         PASS
dotnet build Tools/iap_serial_cli -c Release             PASS
dotnet build Tools/iap_serial_gui -c Release             PASS
dotnet publish Tools/iap_serial_gui -c Release win-x64   PASS
```

静态包校验：

```text
Bootloader FLASH used = 47308 bytes / 128 KiB
AppA slot image       = 272852 bytes
AppA run address      = 0x08020400
AppA version          = 1
AppA body CRC32       = 0x4BF8E226
AppA slot package CRC = 0xCA7D64E0
AppB slot image       = 273484 bytes
AppB run address      = 0x08100400
AppB version          = 2
AppB body CRC32       = 0x16CBCA78
AppB slot package CRC = 0xE4BB3099
```

实板验证步骤：

```text
program build/gcc-debug/apollo_h743_bootloader.hex verify  PASS
program build/gcc-debug/app_a_slot.hex verify reset exit   PASS
dotnet run --project Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release -- --port COM5 --baud 115200 --file build/gcc-debug/app_b_slot.bin --version 2 --chunk 128 --delay-ms 2  PASS
```

App 侧接收 AppB 包通过：

```text
IAP serial: recv done size=273484 crc=0xE4BB3099
IAP serial: staged tag magic=0x41505447 run=0x08100400 app_size=272460 version=2 app_crc=0x16CBCA78
IAP serial: package verified, pending flag set version=2 size=273484 crc=0xE4BB3099
```

复位后 Boot 安装 AppB 并进入 trial：

```text
BOOT IAP: pending magic=0x50414950 version=1 target=1 size=273484 app_ver=2 crc=0xE4BB3099 staging=0x01A02000
BOOT IAP: staging crc calc=0xE4BB3099 expect=0xE4BB3099
BOOT IAP: staged AppB package CRC OK, version=2 app_size=272460
BOOT IAP: erase/program internal AppB only, AppA untouched
BOOT IAP: AppB flash readback OK bytes=273484
BOOT IAP: AppB crc calc=0x16CBCA78 expect=0x16CBCA78
BOOT IAP: pending flag cleared
BOOT IAP: boot state written: trial AppB version=2 max_attempts=3
BOOT IAP: AppB update applied and verified, trial pending, AppA untouched
BOOT IAP: trial AppB attempt 1/3 selected
BOOT: jump to App at 0x08100400
```

AppB 启动后 confirmed：

```text
APP: AppB start from 0x08100400 version=2
IAP confirm: AppB version=2 confirmed
iap boot state: valid=1 state=confirmed active=1 trial=1 attempts=0/3 err=0
```

再次复位验证 confirmed 直启 AppB：

```text
BOOT IAP: no valid pending package
BOOT IAP: boot state=confirmed active=1 trial=1 version=2 attempts=0/3 err=0
BOOT IAP: confirmed AppB selected
BOOT: selected slot=AppB
BOOT: AppB valid, version=2
BOOT: jump to App at 0x08100400
APP: AppB start from 0x08100400 version=2
IAP confirm: AppB already confirmed version=2
```

本轮额外修复：

- Boot 编程 AppB 后增加 Flash 视图同步和稳态读回，避免刚写完后即时读回偶发不一致。
- Boot 对 staging CRC 或 tag 无效的 pending 会清除 pending，避免坏 pending 每次复位重复尝试。
- C# 串口工具等待 `task_iap started` 或周期 `iap status` 后再发送，避免在 App 初始化日志阶段过早发送二进制。

## 2026-06-12 Phase 11 USB CDC 本地 IAP 接入

目标：先补齐 USB 升级入口，使用 USB CDC 虚拟串口复用已有 `iap recv <size> <crc32> <version>` 协议和 C# 上位机，不改变 Boot/AppA/AppB 分区，不做 Option Byte Bank Swap。

本轮修改：

```text
1. 新增 ST USB Device CDC 中间件到 Middlewares/STM32_USB_Device_Library。
2. 新增 App 侧 USB Device/CDC 适配层：usb_device.c、usbd_conf.c、usbd_desc.c、app_usb_cdc.c。
3. App 目标启用 HAL PCD、USB OTG FS 中断、HSI48 USB 时钟。
4. USB FS 使用 PA11/PA12，GPIO_AF10_OTG2_FS，OTG_FS_IRQn 优先级 6。
5. task_iap 轮询 USB CDC RX ring buffer，USB CDC 和 USART1 共用同一套 AppB 包接收、CRC32 校验、pending 写入流程。
6. printf 仍输出到 USART1；当 USB CDC 已配置时，同步镜像到 USB CDC，保证上位机能在同一个 USB COM 口等到 ready for binary 和校验结果。
7. Bootloader 不接入 USB，仍只负责 pending 校验、AppB 写入、trial/confirmed/rollback 选择。
```

使用方式：

```text
1. 烧录 Boot + AppA。
2. 板子启动进入 App 后，Windows 会枚举一个 USB CDC 虚拟 COM 口，产品名为 Apollo H743 IAP CDC。
3. C# GUI/CLI 选择这个新 COM 口。
4. 升级文件仍选择 build/gcc-debug/app_b_slot.bin。
5. 版本号仍使用 2。
```

CLI 示例：

```text
dotnet run --project Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release -- --port <USB_CDC_COM> --baud 115200 --file build/gcc-debug/app_b_slot.bin --version 2 --chunk 128 --delay-ms 2 --app-timeout-ms 120000 --ready-timeout-ms 30000 --log-tail-ms 10000
```

本地构建和静态校验：

```text
cmake --build --preset gcc-debug                         PASS
dotnet build Tools/iap_serial_cli -c Release             PASS
dotnet build Tools/iap_serial_gui -c Release             PASS

Bootloader FLASH used = 47316 bytes / 128 KiB
AppA slot image       = 295872 bytes
AppA run address      = 0x08020400
AppA version          = 1
AppA body CRC32       = 0x9CA80FA2
AppA slot package CRC = 0x4B9A90EC
AppB slot image       = 296504 bytes
AppB run address      = 0x08100400
AppB version          = 2
AppB body CRC32       = 0x087BE7A6
AppB slot package CRC = 0x2E8E7862
```

当前安全边界保持不变：

```text
USB CDC 传输中断、CRC32 不匹配、W25Q 写入失败或 AppB tag 无效时，都不会写 pending。
Boot 只在 pending 有效且 AppB 包完整校验通过后擦写 AppB，AppA 不被覆盖。
```

## 2026-06-12 Phase 11 USB CDC 实板收尾验证

本轮目标：先完成 USB CDC 本地升级入口，复用现有 `iap recv <size> <crc32> <version>` 协议和 C# CLI/GUI 工具。

本轮补充修改：

```text
1. USB Device 初始化前移到 App 早期启动阶段：UART 和向量表重定位完成后立即启动 USB CDC。
2. usb_device.c 改为幂等初始化，避免 app_iap_init 再次调用时重置 USB 状态。
3. 对齐正点原子 HAL 实验59 USB虚拟串口例程：
   - USB2_OTG_FS
   - PA11/PA12
   - HSI48
   - ep0_mps = 0x40
   - USE_USB_FS core id
4. 增加 USB connect/disconnect/bus reset 回调日志。
5. 增加触摸初始化阶段日志，确认 App 不再卡在触摸初始化前后。
```

构建验证：

```text
cmake --build --preset gcc-debug                         PASS
dotnet build Tools/iap_serial_cli -c Release             PASS
dotnet build Tools/iap_serial_gui -c Release             PASS
```

当前静态包校验：

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

实板烧录验证：

```text
openocd -c "set DUAL_BANK 1" -f interface/stlink.cfg -f target/stm32h7x.cfg \
  -c "init; reset halt; program build/gcc-debug/app_b_slot.hex verify; reset run; exit"

结果：AppB 槽 0x08100000 烧录并 verify OK。
```

Boot -> AppB 启动顺序验证：

```text
BOOT IAP: confirmed AppB selected
BOOT: selected slot=AppB
BOOT: tag magic=0x41505447 size=64 run=0x08100400 app_size=295960 version=2 crc=0x078FC745
BOOT: AppB crc calc=0x078FC745 expect=0x078FC745
BOOT: AppB valid, version=2
BOOT: jump to App at 0x08100400
APP: vector copied to RAM src=0x08100400 dst=0x24000000 bytes=1024
USB CDC early init start
USB CDC early init OK
APP: AppB start from 0x08100400 version=2
```

App 侧 IAP 和任务启动验证：

```text
IAP: USB CDC init OK
IAP: serial local source ready on USART1, commands: iap status / iap recv <size> <crc32> <version> / iap demo
IAP: USB CDC local source ready, use the same iap recv protocol on the USB virtual COM port
IAP confirm: AppB already confirmed version=2
FreeRTOS scheduler start
task_gui started
task_iap started
```

USB 实板枚举结果：

```text
Windows 当前串口列表：COM5
COM5 = 板载 CH340 / USART1
未出现新的 USB CDC COM 口。
串口日志未出现：
USB CDC: connected
USB CDC: bus reset
```

结论：

```text
固件侧 USB CDC 入口已接入，AppB 已运行到 task_iap，USB Device 初始化返回 OK。
但主机侧没有触发 USB connect/reset 回调，Windows 未枚举新设备。
因此本轮 USB 升级尚未完成实传验证，当前卡点在 USB Device 物理连接/跳帽/接口选择。
```

下一次实板检查必须先确认：

```text
1. 电脑数据线接的是 USB_SLAVE / USB OTG Type-C 口，不是 USB_UART / CH340 口。
2. P9 CAN/USB 选择跳帽选择 USB，使 PA11/PA12 连接到 USB，而不是 CAN。
3. USB HOST 和 USB SLAVE 不同时使用，因为二者共用 PA11/PA12。
4. 使用支持数据传输的 Type-C 数据线，不要只用充电线。
5. 上电供电充足；4.3 寸屏工作时建议 USB_UART + USB_SLAVE 或外部电源供电。
```

待 USB CDC 枚举出新 COM 口后，继续执行：

```text
dotnet run --project Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release -- --port <USB_CDC_COM> --baud 115200 --file build/gcc-debug/app_b_slot.bin --version 2 --chunk 128 --delay-ms 2 --app-timeout-ms 120000 --ready-timeout-ms 30000 --log-tail-ms 10000
```

## 2026-06-13 Phase 11 SD 卡本地 IAP 固件侧接入

当前没有 USB 数据线，先继续收尾 Phase 11 中不依赖 USB 线的 SD 卡本地 IAP。

本轮修改：

```text
1. 接入 HAL SD/SDMMC 源文件和 FatFs 源文件。
2. 新增 app_sd.c/app_sd.h，使用 SDMMC1、4-bit、轮询读写，按需挂载 SD 卡。
3. 新增 FatFs diskio.c，SD 卡作为 0: 盘，只读使用。
4. App 侧新增 iap sd 命令，读取 SD 卡根目录 0:/app_b_slot.bin。
5. 升级页新增 SD File 按钮，只置位请求，实际 SD/FatFs/W25Q 操作仍在 task_iap 中执行。
6. SD 文件升级复用串口 IAP 已验证的 W25Q staging、pending、Boot 安装 AppB、trial/confirmed 流程。
7. SD 文件读入前先解析包头 app_tag_t，要求 AppB run 地址为 0x08100400，版本来自 tag。
8. SDMMC 内核时钟显式选择 PLL1_Q，当前为 200MHz；HAL normal speed 分频为 4，SDMMC 时钟约 25MHz。
9. task_iap 栈从 1536 words 调整到 2048 words，预留 FatFs 长文件名栈缓冲。
```

SD 卡升级使用方式：

```text
1. 使用当前构建产物 build/gcc-debug/app_b_slot.bin。
2. 将该文件复制到 SD 卡根目录，文件名保持 app_b_slot.bin。
3. 插入开发板 SD 卡槽。
4. 串口输入 iap sd，或在 HMI 升级页点击 SD File。
5. 串口应看到 IAP SD: file staging requested、SD: mounted、IAP SD: file tag、file progress、package verified。
6. 复位后 Boot 应安装 AppB trial，AppB 启动后写入 confirmed。
```

本地构建验证：

```text
cmake --preset gcc-debug                                PASS
cmake --build --preset gcc-debug                        PASS
dotnet build Tools/iap_serial_cli -c Release            PASS
dotnet build Tools/iap_serial_gui -c Release            PASS

Bootloader FLASH used = 47336 bytes / 128 KiB
AppA FLASH used       = 317628 bytes / 895 KiB
AppB FLASH used       = 318260 bytes / 1023 KiB
AppA RAM_D1 used      = 316456 bytes / 512 KiB
AppB RAM_D1 used      = 316456 bytes / 512 KiB

app_a_slot.bin size   = 318652 bytes, crc32=0x3E501EFC
app_b_slot.bin size   = 319284 bytes, crc32=0x859E2082
AppA body CRC32       = 0x33F40418
AppB body CRC32       = 0x98F10D5A
```

当前结论：

```text
固件侧 SD 卡本地 IAP 已完成并通过本地 GCC 构建。
由于本轮未插入 SD 卡实板验证，状态仍为待实板确认。
无卡、挂载失败、文件不存在、文件不是 AppB 包、CRC 校验失败或 W25Q 写入失败时，都不会写有效 pending，不会破坏 AppA。
```

## 2026-06-13 系统诊断底座

本轮先不继续升级链路，补齐 App 运行期诊断基础能力，便于后续排查复位、Fault、任务栈和堆余量问题。

本轮修改：

```text
1. 新增 app_diag.c/app_diag.h。
2. App 启动早期读取 RCC->RSR，生成 reset reason，并清除复位标志。
3. 新增 Fault 快照记录：HardFault、MemManage、BusFault、UsageFault 进入裸汇编入口，提取 MSP/PSP 栈帧后记录寄存器和 SCB Fault 状态。
4. Fault 记录完成后调用 NVIC_SystemReset()，下一次 Boot -> App 启动后可通过串口和 HMI 首页看到上一次 Fault。
5. Fault 快照放在 DTCM 的 .noinit 段，当前占用 96B；Bootloader 当前不使用 DTCM，软件复位穿过 Bootloader 后仍可保留。
6. POR/BOR 冷启动时清空旧诊断快照，避免上电后误显示历史 Fault。
7. USART1 printf 增加 FreeRTOS 互斥保护，调度器启动后多任务日志不再交叉输出；Bootloader 和 App 启动早期仍直接输出。
8. app_tasks 增加运行状态接口，输出 GUI/storage/log/IAP/idle 栈余量和 FreeRTOS heap 余量。
9. HMI 首页增加复位原因、Boot 计数、最近 Fault、heap、任务栈余量、SYS/HCLK/LTDC/FMC 时钟摘要。
```

启动串口新增日志格式：

```text
diag: boot_count=<n> reset_rsr=0xXXXXXXXX reason=<reason>
diag: last_fault=none

如果曾发生 Fault：
diag: last_fault=HardFault count=<n> pc=0xXXXXXXXX lr=0xXXXXXXXX cfsr=0xXXXXXXXX hfsr=0xXXXXXXXX bfar=0xXXXXXXXX mmfar=0xXXXXXXXX
```

HMI 首页新增显示内容：

```text
Reset: <reason>   Boot: <n>   Fault: <name> count=<n>
Heap: <free>/<min_free> B
Stack G/S/L/I/Idle: <gui>/<storage>/<log>/<iap>/<idle>
Clock SYS/HCLK/LTDC/FMC: 400/200/33/220 MHz
```

本地构建验证：

```text
cmake --build --preset gcc-debug                        PASS

Bootloader FLASH used = 47336 bytes / 128 KiB
Bootloader RAM_D1 used = 8456 bytes / 512 KiB
Bootloader DTCMRAM used = 0 bytes / 128 KiB

AppA FLASH used       = 320280 bytes / 895 KiB
AppA DTCMRAM used     = 96 bytes / 128 KiB
AppA RAM_D1 used      = 316640 bytes / 512 KiB
AppA body CRC32       = 0x9480DDEA

AppB FLASH used       = 320928 bytes / 1023 KiB
AppB DTCMRAM used     = 96 bytes / 128 KiB
AppB RAM_D1 used      = 316640 bytes / 512 KiB
AppB body CRC32       = 0x4582521D
```

当前结论：

```text
系统诊断底座已完成本地构建验证。
本轮未重新下载到实板；下一次实板验证重点看串口 diag 日志、HMI 首页诊断字段、周期栈余量日志是否仍正常。
```

## 2026-06-27 Phase 11 USB CDC / SD 卡 IAP 回归验证

目标：在 SD 卡 `SD mount fail` / `SDMMC_ERROR_RX_OVERRUN` 修复后，重新执行完整回归，确认 Boot/AppA/AppB 烧录、USB CDC IAP、SD 卡 IAP、Boot 安装 AppB 和 AppB confirmed 均闭环。

本轮代码状态：

```text
1. SDMMC 配置改为稳定优先：1-bit、APP_SD_CLOCK_DIV=80U、开启 SDMMC 硬件流控。
2. HAL_SD_ReadBlocks / HAL_SD_WriteBlocks 轮询传输期间使用 vTaskSuspendAll() 暂停任务切换，但不关闭中断。
3. app_sd.c 增加 SD init/read/write/timeout 诊断日志。
4. diskio.c 增加 FatFs 底层 sector/count/error/state 诊断日志。
5. app_sd.h 增加 app_sd_get_last_error() 和 app_sd_get_card_state()。
```

当前构建产物：

```text
cmake --build --preset gcc-debug                        PASS, ninja: no work to do

Bootloader text+data = 47372 bytes
AppA text+data       = 322900 bytes
AppB text+data       = 323532 bytes
AppA slot image      = 323932 bytes
AppB slot image      = 324564 bytes

AppB run address      = 0x08100400
AppB version          = 2
AppB body CRC32       = 0xBD293B71
AppB slot package CRC = 0xAF740721
```

完整烧录：

```text
openocd -c "set DUAL_BANK 1" -f interface/stlink.cfg -f target/stm32h7x.cfg -c "transport select swd" -c "adapter speed 1000" -c "program build/gcc-debug/apollo_h743_bootloader.hex verify" -c "program build/gcc-debug/app_a_slot.hex verify" -c "program build/gcc-debug/app_b_slot.hex verify reset exit"

Bootloader programming + verify    PASS
AppA slot programming + verify     PASS
AppB slot programming + verify     PASS
```

启动回归：

```text
BOOT IAP: no valid pending package
BOOT IAP: boot state=confirmed active=1 trial=1 version=2 attempts=0/3 err=0
BOOT IAP: confirmed AppB selected
BOOT: selected slot=AppB
BOOT: tag magic=0x41505447 size=64 run=0x08100400 app_size=323540 version=2 crc=0xBD293B71
BOOT: AppB crc calc=0xBD293B71 expect=0xBD293B71
BOOT: AppB valid, version=2
APP: AppB start from 0x08100400 version=2
IAP confirm: AppB already confirmed version=2
diag: last_fault=none
```

SD 卡 IAP 回归：

```text
触发方式：COM3 USART1 发送 iap sd
SD 卡：4GB FAT32，根目录 0:/app_b_slot.bin

IAP SD: file staging requested: 0:/app_b_slot.bin
SD: mounted at 0:, blocks=7744512 block_size=512 capacity=3781 MB
IAP SD: file tag magic=0x41505447 run=0x08100400 app_size=323540 version=2 app_crc=0xBD293B71
IAP SD: recv begin size=324564 crc=0x00000000 version=2
IAP SD: file progress 324564/324564
IAP SD: recv done size=324564 crc=0xAF740721
IAP SD: package verified, pending flag set version=2 size=324564 crc=0xAF740721
iap status: flash=1 pending=1 version=2 size=324564 crc=0xAF740721 recv=0 324564/324564 state=SD staged, pending set
```

复位后 Boot 安装 SD staged 包：

```text
BOOT IAP: pending magic=0x50414950 version=1 target=1 size=324564 app_ver=2 crc=0xAF740721 staging=0x01A02000
BOOT IAP: staging crc calc=0xAF740721 expect=0xAF740721
BOOT IAP: staged tag magic=0x41505447 size=64 run=0x08100400 app_size=323540 version=2 crc=0xBD293B71
BOOT IAP: staged AppB package CRC OK, version=2 app_size=323540
BOOT IAP: AppB erase OK sectors=8
BOOT IAP: AppB program progress 324564/324564
BOOT IAP: AppB flash readback OK bytes=324564
BOOT IAP: AppB crc calc=0xBD293B71 expect=0xBD293B71
BOOT IAP: pending flag cleared
BOOT IAP: boot state written: trial AppB version=2 max_attempts=3
BOOT IAP: AppB update applied and verified, trial pending, AppA untouched
APP: AppB start from 0x08100400 version=2
IAP confirm: AppB version=2 confirmed
iap status: flash=1 pending=0 version=0 size=0 crc=0x00000000 recv=0 0/0 state=AppB confirmed
```

观察项：

```text
本轮 SD 安装后的第一次 Boot flash readback 出现一次单字节 mismatch，随后 retry recovered：
BOOT IAP: verify mismatch at flash=0x0811D7A4 actual=0x98 expect=0x99 attempt=1
BOOT IAP: verify retry recovered at flash=0x0811D600 attempt=2

最终 readback、AppB CRC 和 confirmed 均通过。该现象由既有 Boot 稳态读回重试机制恢复，后续可作为重复升级/供电/Flash 读回稳定性观察项。
```

USB CDC 枚举和状态命令回归：

```text
Windows 串口列表：COM3、COM4
COM4 = USB 串行设备 (COM4), USB\VID_0483&PID_5740&MI_00

COM4 发送 iap status：
IAP status: flash=1 pending=0 version=0 size=0 crc=0x00000000 recv=0 0/0 state=AppB confirmed
IAP boot state: valid=1 state=confirmed active=1 trial=1 attempts=0/3 err=0
```

USB CDC IAP 完整传输回归：

```text
.\Tools\iap_serial_cli\bin\Release\net10.0-windows\ApolloIapSerialCli.exe --port COM4 --baud 115200 --file build\gcc-debug\app_b_slot.bin --version 2 --chunk 128 --delay-ms 2 --app-timeout-ms 30000 --ready-timeout-ms 10000 --log-tail-ms 8000 --no-wait-app

file=build\gcc-debug\app_b_slot.bin size=324564 crc32=0xAF740721 version=2
iap recv 324564 0xAF740721 2
IAP USB: ready for binary size=324564
sent=324564
IAP USB: package verified, pending flag set version=2 size=324564 crc=0xAF740721
发送完成: 324564/324564 100.0%
```

复位后 Boot 安装 USB staged 包：

```text
BOOT IAP: pending magic=0x50414950 version=1 target=1 size=324564 app_ver=2 crc=0xAF740721 staging=0x01A02000
BOOT IAP: staging crc calc=0xAF740721 expect=0xAF740721
BOOT IAP: staged AppB package CRC OK, version=2 app_size=323540
BOOT IAP: AppB flash readback OK bytes=324564
BOOT IAP: AppB crc calc=0xBD293B71 expect=0xBD293B71
BOOT IAP: pending flag cleared
BOOT IAP: boot state written: trial AppB version=2 max_attempts=3
BOOT IAP: AppB update applied and verified, trial pending, AppA untouched
APP: AppB start from 0x08100400 version=2
IAP confirm: AppB version=2 confirmed
iap status: flash=1 pending=0 version=0 size=0 crc=0x00000000 recv=0 0/0 state=AppB confirmed
```

本轮结论：

```text
1. Bootloader + AppA + AppB 完整烧录 verify OK。
2. 当前实际运行槽位为 AppB，AppB tag CRC 与当前构建一致。
3. USB CDC 枚举、iap status、完整 app_b_slot.bin 传输、pending、Boot 安装和 AppB confirmed 均通过。
4. SD 卡 mount、文件读取、W25Q staging、pending、Boot 安装和 AppB confirmed 均通过。
5. 此前 SD mount fail 根因确认是 SDMMC 轮询读 RX overrun，当前稳定性修复在 4GB FAT32 卡上通过。
6. 后续建议继续做多卡、多次重复升级、断电场景、以及 SD DMA/中断模式优化评估。
```

## 2026-06-28 Phase 13 Step 2 通信总线和系统状态模型

目标：按 `Docs/phase13_comm_architecture_plan.md` 的 Step 2，先落地通信解耦基础设施，不接真实 DSP/CAN/Modbus 硬件，不改 Boot/IAP 分区。

本轮修改：

```text
1. 新增 app_comm_bus.c/app_comm_bus.h。
   - 提供 app_comm_publish() / app_comm_receive()。
   - 第一版内部使用 FreeRTOS queue。
   - 定义 DSP/BMS/Modbus 事件类型和事件载荷结构。

2. 新增 app_system_model.c/app_system_model.h。
   - 集中维护 DSP/BMS/Modbus 快照。
   - 提供 app_system_model_process_event() 和 app_system_model_get_snapshot()。
   - 预留 DSP/BMS online/offline、CRC 错误、超时计数、Modbus 请求/异常计数。

3. main.c 在 app_log_init() 后初始化 comm bus 和 system model。

4. app_tasks 增加 comm/core 栈水位字段预留。
   - 当前 Step 2 尚未创建 task_comm/task_core，因此显示为 0。
   - task_log 临时承担第一版 core 事件泵职责，周期从 comm bus 取事件并调用 app_system_model_process_event()。
   - task_log 周期调用 app_system_model_poll() 维护超时状态。

5. HMI 首页改为通过 app_system_model_get_snapshot() 读取通信状态。
   - 当前 DSP/BMS 默认 offline。
   - Modbus 请求/异常/CRC 计数默认 0。
   - UI 不直接读取协议模块内部变量，为后续模拟通信接入预留。
```

本地构建验证：

```text
cmake --build --preset gcc-debug                        PASS

AppA FLASH used       = 325328 bytes / 895 KiB
AppA DTCMRAM used     = 96 bytes / 128 KiB
AppA RAM_D1 used      = 316744 bytes / 512 KiB
AppA slot image       = 326352 bytes
AppA slot package CRC = 0xD61EC491
AppA body CRC32       = 0xB0762534

AppB FLASH used       = 325976 bytes / 1023 KiB
AppB DTCMRAM used     = 96 bytes / 128 KiB
AppB RAM_D1 used      = 316744 bytes / 512 KiB
AppB slot image       = 327000 bytes
AppB slot package CRC = 0xE9933D5F
AppB body CRC32       = 0xBF4916A6
```

当前结论：

```text
Phase 13 Step 2 已完成本地构建验证。
本轮建立通信总线、系统状态模型和临时事件消费入口；后续 DSP/BMS/Modbus 模拟模块可先 publish 到 comm bus，再由 system model 汇总。
本轮未接入 DSP/BMS/Modbus 模拟帧生成和协议解析。
本轮未实板烧录，也未重新跑 USB/SD IAP 闭环。
下一步建议实现 Step 3：app_dsp_link + 虚拟 DSP 帧生成，先让 DSP 状态通过 comm bus 进入 system model，再由 HMI 首页显示 online/数值/CRC/timeout。
```

## 2026-06-28 Phase 13 Step 3 DSP 模拟链路

目标：按 `Docs/phase13_comm_architecture_plan.md` 的 Step 3，新增 ARM-DSP 内部通信的纯软件模拟链路，不接真实 SPI，不占用 USART1/USB CDC，不修改 Boot/IAP/Flash 分区。

本轮修改：

```text
1. 新增 app_dsp_link.c/app_dsp_link.h。
   - 定义固定 20 字节 DSP 虚拟帧。
   - 状态帧包含 Vbus、Vgrid、Iout、温度、run_state、warn、fault。
   - 使用 CRC16(Modbus 多项式) 校验虚拟帧。
   - 有效状态帧转换为 APP_COMM_EVENT_DSP_STATUS。
   - CRC 错误帧丢弃，并发布 APP_COMM_EVENT_DSP_CRC_ERROR。
   - 支持构造 ARM -> DSP 命令帧，并模拟 DSP ACK。
   - 维护 rx/valid/crc_err/invalid/timeout/cmd/ack 诊断计数。

2. app_comm_bus.h 增加事件 source 枚举。
   - APP_COMM_SOURCE_DSP_LINK
   - APP_COMM_SOURCE_BMS_CAN
   - APP_COMM_SOURCE_MODBUS_RTU

3. main.c 初始化 DSP link。

4. app_tasks.c 临时在 task_log 中以 100ms 周期轮询 app_dsp_link_poll()。
   - 第一版不新增正式 task_comm/task_core，避免提前进入 Step 6。
   - task_log 继续消费 comm bus 事件并调用 app_system_model_process_event()。
   - 周期日志新增 dsp link 诊断计数。

5. app_system_model 增加 DSP ACK 计数。

6. HMI 首页 DSP 行增加 ack 计数显示。
```

本地构建验证：

```text
cmake --build --preset gcc-debug                        PASS

AppA FLASH used       = 328348 bytes / 895 KiB
AppA DTCMRAM used     = 96 bytes / 128 KiB
AppA RAM_D1 used      = 316808 bytes / 512 KiB
AppA slot image       = 329372 bytes
AppA slot package CRC = 0x5456EBEF
AppA body CRC32       = 0x543FFDC1

AppB FLASH used       = 328980 bytes / 1023 KiB
AppB DTCMRAM used     = 96 bytes / 128 KiB
AppB RAM_D1 used      = 316808 bytes / 512 KiB
AppB slot image       = 330004 bytes
AppB slot package CRC = 0x62CE4EEB
AppB body CRC32       = 0x6EB7A4CD
```

当前结论：

```text
Phase 13 Step 3 已完成本地构建验证。
当前模拟 DSP 状态链路为：app_dsp_link_poll() -> APP_COMM_EVENT_DSP_STATUS/ACK/CRC_ERROR -> comm bus -> system model -> HMI 首页。
本轮未接真实 SPI，未新增正式 task_comm/task_core，未实板烧录，也未重新跑 USB/SD IAP 闭环。
下一步建议实现 Step 4：app_bms_can + 虚拟 CAN BMS 报文，先模拟 SOC、电压、电流、温度、限值、告警和 offline。
```

## 2026-06-28 Phase 13 Step 4 BMS CAN 模拟链路

目标：按 `Docs/phase13_comm_architecture_plan.md` 的 Step 4，新增 BMS CAN 纯软件模拟链路，不接真实 FDCAN/CAN 收发器，不修改 Boot/IAP/Flash 分区。

本轮修改：

```text
1. 新增 app_bms_can.c/app_bms_can.h。
   - 定义 app_can_frame_t，模拟标准 CAN 帧：id + dlc + data[8]。
   - 使用 0x351 作为 BMS 状态帧，包含 SOC、SOH、pack voltage、pack current。
   - 使用 0x355 作为 BMS 限值帧，包含 max temp、charge limit、discharge limit。
   - 使用 0x359 作为 BMS 告警帧，包含 alarm_flags、fault_flags。
   - 解析结果统一转换为 APP_COMM_EVENT_BMS_STATUS / LIMITS / ALARM。
   - 支持掉线窗口，超时后发布 APP_COMM_EVENT_BMS_TIMEOUT。
   - 维护 rx/status/limits/alarm/invalid/timeout/alarm_change/last_id 诊断计数。

2. main.c 初始化 BMS CAN 模拟链路。

3. app_tasks.c 临时在 task_log 中轮询 app_bms_can_poll()。
   - 第一版仍不新增正式 task_comm/task_core。
   - BMS 事件继续通过 comm bus 进入 system model。
   - 周期日志新增 bms can 诊断计数。

4. HMI 首页已通过 Step 2 的 system model 快照显示 BMS online、SOC、电压、电流、frame、timeout。
```

本地构建验证：

```text
cmake --build --preset gcc-debug                        PASS

AppA FLASH used       = 330908 bytes / 895 KiB
AppA DTCMRAM used     = 96 bytes / 128 KiB
AppA RAM_D1 used      = 316888 bytes / 512 KiB
AppA slot image       = 331932 bytes
AppA slot package CRC = 0xFB35D5D3
AppA body CRC32       = 0x835CB12D

AppB FLASH used       = 331540 bytes / 1023 KiB
AppB DTCMRAM used     = 96 bytes / 128 KiB
AppB RAM_D1 used      = 316888 bytes / 512 KiB
AppB slot image       = 332564 bytes
AppB slot package CRC = 0x623745B4
AppB body CRC32       = 0x2437AAEF
```

当前结论：

```text
Phase 13 Step 4 已完成本地构建验证。
当前模拟 BMS 链路为：app_bms_can_poll() -> APP_COMM_EVENT_BMS_STATUS/LIMITS/ALARM/TIMEOUT -> comm bus -> system model -> HMI 首页。
本轮未接真实 CAN 硬件，未新增正式 task_comm/task_core，未实板烧录，也未重新跑 USB/SD IAP 闭环。
下一步建议实现 Step 5：app_modbus_rtu + 寄存器映射，支持 0x03/0x04/0x06、CRC16、异常码和从 system model 读取 DSP/BMS 快照。
```

## 2026-06-28 Phase 13 Step 5 Modbus RTU 模拟协议

目标：按 `Docs/phase13_comm_architecture_plan.md` 的 Step 5，新增 Modbus RTU 纯软件模拟协议，不接真实 RS485，不占用 USART1/USB CDC，不修改 Boot/IAP/Flash 分区。

本轮修改：

```text
1. 新增 app_modbus_rtu.c/app_modbus_rtu.h。
   - 支持 0x03 Read Holding Registers。
   - 支持 0x04 Read Input Registers。
   - 支持 0x06 Write Single Register。
   - 实现 Modbus RTU CRC16。
   - 支持异常响应：0x01 Illegal Function、0x02 Illegal Data Address、0x03 Illegal Data Value。

2. 寄存器映射读取 system model 快照。
   - 0x0000-0x0007 映射 DSP online、Vbus、Vgrid、Iout、温度、状态、告警、故障。
   - 0x0100-0x0109 映射 BMS online、SOC、SOH、电压、电流、温度、限值、告警、故障。
   - 0x0200-0x0203 作为 ARM 命令/参数写寄存器。

3. app_comm_bus 增加 APP_COMM_EVENT_MODBUS_REQUEST。
   - 普通读请求发布 MODBUS_REQUEST。
   - 0x06 写请求发布 MODBUS_WRITE。
   - CRC 错误或异常发布 MODBUS_ERROR。

4. app_system_model 增加 Modbus 请求/异常/CRC 统计。

5. main.c 初始化 Modbus RTU 模拟协议，slave address = 1。

6. app_tasks.c 临时在 task_log 中轮询 app_modbus_rtu_poll()。
   - 虚拟请求覆盖读 DSP、读 BMS、写命令、非法地址、CRC 错误。
   - 周期日志新增 modbus 诊断计数。
```

本地构建验证：

```text
cmake --build --preset gcc-debug                        PASS

AppA FLASH used       = 334068 bytes / 895 KiB
AppA DTCMRAM used     = 96 bytes / 128 KiB
AppA RAM_D1 used      = 316928 bytes / 512 KiB
AppA slot image       = 335092 bytes
AppA slot package CRC = 0x731D99B7
AppA body CRC32       = 0x46119F37

AppB FLASH used       = 334716 bytes / 1023 KiB
AppB DTCMRAM used     = 96 bytes / 128 KiB
AppB RAM_D1 used      = 316928 bytes / 512 KiB
AppB slot image       = 335740 bytes
AppB slot package CRC = 0x8B672E63
AppB body CRC32       = 0xE60E0724
```

当前结论：

```text
Phase 13 Step 5 已完成本地构建验证。
当前模拟 Modbus 链路为：app_modbus_rtu_poll() -> 解析虚拟 RTU 请求 -> 读写 system model 寄存器映射 -> 构造响应/异常 -> comm bus -> HMI 统计。
本轮未接真实 RS485，未新增正式 task_comm/task_core，未实板烧录，也未重新跑 USB/SD IAP 闭环。
下一步建议实现 Step 6：把当前临时 task_log 轮询迁移为正式 task_comm / task_core，补齐任务栈水位统计，并确认不影响 task_iap、USB CDC、SD IAP。
```

## 2026-06-28 Phase 13 Step 6 task_comm / task_core 任务拆分

目标：按 `Docs/phase13_comm_architecture_plan.md` 的 Step 6，把 Step 3-5 临时放在 `task_log` 里的通信轮询和事件消费拆到正式任务中，形成面试可讲清楚的 RTOS 任务解耦结构。本轮不接真实 SPI/CAN/RS485，不修改 Boot/IAP/Flash 分区。

本轮修改：

```text
1. app_tasks.c 新增 task_comm。
   - 100ms 周期调用 app_dsp_link_poll()。
   - 100ms 周期调用 app_bms_can_poll()。
   - 100ms 周期调用 app_modbus_rtu_poll()。
   - 只负责通信模拟和协议状态机推进，不直接修改 LVGL UI。

2. app_tasks.c 新增 task_core。
   - 50ms 周期从 app_comm_bus 消费最多 16 个事件。
   - 统一调用 app_system_model_process_event() 汇总 DSP/BMS/Modbus 状态。
   - 周期调用 app_system_model_poll() 维护 online/offline 超时状态。

3. task_log 恢复为日志和心跳职责。
   - 不再轮询 DSP/BMS/Modbus。
   - 不再消费 comm bus 事件。
   - 1s 翻转 LED0。
   - 5s 打印一次诊断日志。

4. app_tasks_get_runtime_status() 补齐 task_comm / task_core 栈水位。
   - 周期日志中的 comm/core 不再是 0。
   - HMI 首页任务水位字段也能读到真实 comm/core 余量。

5. 新增任务栈配置。
   - TASK_COMM_STACK_WORDS = 2048 words。
   - TASK_CORE_STACK_WORDS = 2048 words。
   - 当前 FreeRTOS heap 配置为 96KB，本轮新增两个任务会增加 heap 占用，后续实板需观察 heap_free 和 comm/core watermark。
```

本地构建验证：

```text
cmake --build --preset gcc-debug                        PASS

AppA FLASH used       = 334412 bytes / 895 KiB
AppA DTCMRAM used     = 96 bytes / 128 KiB
AppA RAM_D1 used      = 316936 bytes / 512 KiB
AppA slot image       = 335436 bytes
AppA slot package CRC = 0xBE05A5B6
AppA body CRC32       = 0x5758CE70

AppB FLASH used       = 335060 bytes / 1023 KiB
AppB DTCMRAM used     = 96 bytes / 128 KiB
AppB RAM_D1 used      = 316936 bytes / 512 KiB
AppB slot image       = 336084 bytes
AppB slot package CRC = 0xEC11517C
AppB body CRC32       = 0x43C74B33
```

预期串口周期日志：

```text
task_comm started
task_core started
stack watermark words: gui=... storage=... log=... iap=... comm=... core=... idle=... heap_free=...
dsp link: online=... rx=... valid=... crc_err=... invalid=... timeout=... cmd=... ack=...
bms can: online=... rx=... status=... limits=... alarm=... invalid=... timeout=...
modbus: req=... resp=... exc=... crc=... write=...
```

当前结论：

```text
Phase 13 Step 6 已完成本地构建验证。
当前 RTOS 任务边界为：task_comm 负责通信协议轮询，task_core 负责事件消费和 system model 汇总，task_gui 只读 system model 快照刷新界面，task_log 只做心跳和诊断日志。
本轮未实板烧录，未重新跑 USB/SD IAP 闭环；因为没有修改 Boot/IAP/分区/USB/SD/FatFs，按验证边界本地构建通过即可作为本 Step 的代码验收。
下一步建议做 Step 7：把 HMI 首页或诊断页的通信状态显示再整理一轮，重点确认 DSP/BMS/Modbus 的 online、计数、异常和任务水位字段在屏幕上可读，并准备面试讲述稿。
```

## 2026-06-28 Phase 13 Step 7 HMI 通信诊断显示整理

目标：按 `Docs/phase13_comm_architecture_plan.md` 新增的 Step 7，把 Phase 13 的模拟通信闭环整理到 HMI 首页诊断显示中，让面试时能直接说明 `task_comm -> comm bus -> task_core -> system_model -> task_gui` 的数据流。本轮不接真实 SPI/CAN/RS485，不修改 Boot/IAP/USB/SD 链路。

本轮修改：

```text
1. app_system_model.h/app_system_model.c 扩展 Modbus 快照。
   - 增加 write_count。
   - 增加 last_write_reg / last_write_value。
   - 增加 last_error_code。

2. app_modbus_rtu.c 修正 0x06 写命令事件路径。
   - 读请求继续发布 APP_COMM_EVENT_MODBUS_REQUEST。
   - 写请求只有在 app_modbus_map_write() 成功后才发布 APP_COMM_EVENT_MODBUS_WRITE。
   - 非法写地址只进入异常路径，不进入写命令快照。
   - 写事件携带寄存器地址和值，task_core 能完整写入 system_model。

3. app_ui_hmi.c 整理首页通信摘要。
   - DSP 显示 online、Vbus、Grid、Iout、Temp、run/warn/fault、fw、frame、ack、CRC/timeout。
   - BMS 显示 online、SOC/SOH、电压、电流、温度、充放电限值、alarm/fault、frame、timeout。
   - Modbus 显示 req/write/exception/CRC、最后一次写寄存器和值、最后错误类型。
   - UI 仍只读取 app_system_model_get_snapshot()，不直接读取协议模块内部状态，也不直接调用协议解析接口。

4. Docs/phase13_comm_architecture_plan.md 增加 Step 7 计划和验收标准。
```

本地构建验证：

```text
cmake --build --preset gcc-debug                        PASS

AppA FLASH used       = 334952 bytes / 895 KiB
AppA DTCMRAM used     = 96 bytes / 128 KiB
AppA RAM_D1 used      = 316952 bytes / 512 KiB
AppA slot image       = 335976 bytes
AppA slot package CRC = 0x3C495828
AppA body CRC32       = 0xAA9A9734

AppB FLASH used       = 335600 bytes / 1023 KiB
AppB DTCMRAM used     = 96 bytes / 128 KiB
AppB RAM_D1 used      = 316952 bytes / 512 KiB
AppB slot image       = 336624 bytes
AppB slot package CRC = 0x9E5F1BEC
AppB body CRC32       = 0xDC6531E2
```

当前结论：

```text
Phase 13 Step 7 已完成本地构建验证。
当前首页通信诊断已经能覆盖三条模拟链路的关键状态和异常计数，且 UI 仍保持只读 system_model 快照的边界。
本轮未实板烧录，未肉眼确认屏幕排版，未重新跑 USB/SD IAP 闭环；后续上板时需要重点看首页文本是否完整可读、comm/core 栈水位是否正常、模拟计数是否随时间变化。
下一步建议做 Step 8：整理一份 Phase 13 面试讲述稿，把 SPI-DSP、CAN-BMS、Modbus、FreeRTOS 任务解耦、队列/快照和后续替换真实硬件的路径讲成一套 3-5 分钟项目表达。
```

## 2026-06-28 Phase 13 Step 8 面试讲述稿整理

目标：把 Phase 13 通信框架整理成可直接用于技术一面和项目深挖的讲述材料，重点讲清 SPI-DSP、CAN-BMS、Modbus RTU、FreeRTOS 任务解耦、comm bus、system model、GUI snapshot，以及后续替换真实硬件 transport 的路径。本轮只改文档，不改固件逻辑。

本轮修改：

```text
1. 新增 Docs/phase13_interview_script.md。
   - 提供一句话项目定位。
   - 提供 90 秒版本。
   - 提供 3-5 分钟版本。
   - 分别整理 ARM-DSP、BMS CAN、Modbus RTU 的讲述方式。
   - 整理资深面试官追问和回答边界。
   - 明确哪些是当前软件模拟闭环，哪些是真实硬件后续接入项。

2. 更新 Docs/phase13_comm_architecture_plan.md。
   - 增加 Step 8：整理 Phase 13 面试讲述稿。
   - 明确本 Step 只改文档，不改固件逻辑。
```

本地验证：

```text
cmake --build --preset gcc-debug                        PASS, ninja: no work to do
```

当前结论：

```text
Phase 13 Step 8 已完成。
当前 Phase 13 已经具备“代码闭环 + HMI 诊断 + 面试讲述稿”三部分材料：既能展示 task_comm/task_core/system_model 的实现，也能讲清楚为什么当前先做模拟、后续怎么替换真实 SPI/CAN/RS485。
本轮未实板烧录，未重新跑 USB/SD IAP 闭环；因为只修改文档，按验证边界本地构建确认无代码重编即可。
下一步建议不要继续堆模拟功能，优先把 Step 8 面试稿口述 2-3 遍；如继续开发，建议只选一个真实外设做小闭环，例如真实 CAN RX queue 或真实 RS485 Modbus RTU。
```
