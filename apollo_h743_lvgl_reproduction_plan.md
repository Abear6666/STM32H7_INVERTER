# Apollo STM32H743 + 4.3寸 RGB 800x480 屏 LVGL 从 0 开始复现任务书

本文档是给 Codex 使用的独立任务书。假设执行这份文档的电脑 **没有当前 `STM32H7_LCD` 工程源码**，只拥有：

- 正点原子 Apollo STM32H743 V2 开发板；
- 4.3 寸 RGB 屏，分辨率 `800x480`；
- 正点原子官方资料包、原理图、例程；
- STM32 开发工具链；
- 互联网或本地 LVGL 源码。

目标不是照搬某个已有工程，而是从 0 搭建一个可运行、可调试、可面试讲解的工业 HMI 雏形：

```text
STM32H743
  -> SDRAM 显存
  -> LTDC RGB 屏
  -> LVGL 图形界面
  -> FreeRTOS 多任务
  -> 外部 Flash 参数保存
  -> Bootloader + App 跳转
  -> 本地升级
  -> 后续扩展远程升级
  -> 最后再接入模拟 DSP 通信
```

## 1. 最重要结论

### 1.1 H743 + 800x480 RGB 屏适合跑 LVGL 吗

适合。

STM32H743 有：

- Cortex-M7 高性能内核；
- LTDC，用于 RGB LCD 扫描输出；
- DMA2D，用于图形拷贝、填充、颜色格式转换；
- FMC，可连接 SDRAM；
- 2MB 内部 Flash；
- 多块内部 RAM；
- 常见 Apollo 板还带 32MB SDRAM 和 32MB W25Q256 外部 Flash。

这套硬件非常适合学习和实现 LVGL HMI。

### 1.2 LVGL 占内存大吗

LVGL 本体不是最大内存消耗。真正的大头是：

```text
LCD framebuffer
图片资源
字体资源
LVGL draw buffer
LVGL heap
FreeRTOS heap / task stack
```

800x480 RGB565 一帧显存：

```text
800 * 480 * 2 = 768000 bytes = 750 KiB
```

所以这块屏幕不能随便把显存塞进内部小 RAM，推荐放外部 SDRAM。

### 1.3 推荐显示架构

第一版推荐：

```text
LVGL partial draw buffer
  -> flush_cb 拷贝到 SDRAM framebuffer
  -> LTDC 持续扫描 SDRAM framebuffer
  -> RGB 屏显示
```

也就是：

```text
LVGL 负责画图逻辑
SDRAM 保存最终屏幕像素
LTDC 自动把 SDRAM 像素送到 LCD
```

### 1.4 后续可以远程升级吗

可以。

但远程升级不是单独一个功能，它依赖完整链路：

```text
联网模块 / 以太网 / WiFi / 4G
  -> 下载升级包
  -> 写入外部 Flash 或 AppB 分区
  -> CRC32 检查传输完整性
  -> SHA256 / 签名检查固件可信性
  -> Bootloader 切换或复制新 App
  -> 失败回滚
```

第一版先做本地升级，例如串口、USB、SD 卡。等 Bootloader 和分区稳定后，再接远程下载。

### 1.5 完成界面后固件大概多大

粗略估算：

| 工程内容 | App 固件大小 |
|---|---:|
| HAL + 基础驱动 + LVGL 简单页面 | `400KB - 800KB` |
| FreeRTOS + 多页面 UI + 少量图标 | `700KB - 1.2MB` |
| 中文字体 + 图片资源内置到内部 Flash | `1.5MB+`，容易爆 Flash |
| 图片/字体放外部 Flash，内部只放代码 | `800KB - 1.3MB` 常见 |

建议从一开始就按这个原则设计：

```text
代码放内部 Flash
显存放 SDRAM
图片/字体/升级包放外部 Flash 或 NAND
用户参数放外部 Flash / EEPROM
```

## 2. 本项目最终要做成什么

最终工程建议命名：

```text
apollo_h743_lvgl_hmi/
```

最终具备：

- 上电有串口启动日志；
- SDRAM memtest 通过；
- 800x480 RGB 屏显示纯色、渐变、棋盘格；
- LVGL 正常显示页面；
- 触摸可用；
- FreeRTOS 正常调度；
- 参数可保存，断电/复位后保持；
- Bootloader 可校验并跳转 App；
- App 启动后设置自己的向量表；
- 支持第一版本地 IAP；
- 后续可扩展远程升级。
- 最后增加模拟 DSP 通信，把采集/控制数据接入 UI 和日志。

## 3. 需要准备的资料

Codex 执行前，先确认本地是否有以下资料。

### 3.1 正点原子资料

建议准备：

```text
Apollo STM32H743 V2 开发板资料包
开发板原理图 PDF
核心板原理图 PDF
用户手册 PDF
4.3寸 RGB 屏资料
LCD 例程
SDRAM 例程
触摸例程
W25Q256 / SPI Flash 例程
FreeRTOS 例程
```

### 3.2 工具链

推荐：

```text
STM32CubeMX
STM32CubeIDE 或 Keil MDK
STM32CubeProgrammer
Git
Python 3
串口助手
ST-LINK 驱动
```

如果使用 Keil：

```text
MDK-ARM 5.x
ARM Compiler 5 或 ARM Compiler 6
STM32H7 Device Family Pack
```

如果使用 CubeIDE：

```text
STM32CubeIDE 最新稳定版
GNU Arm Embedded Toolchain
```

### 3.3 LVGL 版本

第一版推荐：

```text
LVGL 8.4.x
```

原因：

- 资料多；
- API 稳定；
- 移植文章多；
- `lv_disp_drv_t` / `flush_cb` 模型适合学习底层显示原理。

也可以使用 LVGL 9.x，但 API 改动较多，第一版不建议一上来用。

## 4. 推荐项目目录

从 0 新建仓库：

```text
apollo_h743_lvgl_hmi/
  README.md
  Docs/
    bringup_log.md
    memory_map.md
    hardware_checklist.md
    iap_design.md
    lvgl_port_notes.md
  Boot/
    Core/
    Drivers/
    Middlewares/
  App/
    Core/
    Drivers/
    Middlewares/
    LVGL/
    UI/
  Common/
    include/
    src/
    flash_layout/
    protocol/
    crc/
  Tools/
    pack_app/
    pack_resource/
    image_convert/
```

如果使用 CubeMX 生成工程，也可以采用 CubeMX 默认目录。最低要求是维护好：

```text
Docs/bringup_log.md
Docs/memory_map.md
Docs/hardware_checklist.md
```

这三个文档必须随着开发同步更新。

## 5. 硬件确认清单

Codex 第一件事不是写代码，而是确认硬件。

在 `Docs/hardware_checklist.md` 中填写：

```text
MCU 型号：
开发板型号：
屏幕尺寸：
屏幕分辨率：
屏幕接口：RGB/LTDC
屏幕色彩格式：RGB565
屏幕像素时钟：
屏幕 HSYNC/VSYNC/DE/CLK 极性：
屏幕时序参数：
触摸芯片：
触摸接口：I2C / SPI
SDRAM 型号：
SDRAM 容量：
SDRAM FMC Bank：
SDRAM 基地址：
外部 Flash 型号：
外部 Flash 接口：SPI / QSPI
是否支持 memory mapped：
串口日志端口：
LED 引脚：
按键引脚：
```

如果资料不完整，Codex 必须停止硬件相关代码编写，先列出缺失项。

## 6. 内存规划

### 6.1 STM32H743 内部 Flash

STM32H743 常见内部 Flash：

```text
起始地址：0x08000000
总大小：2MB
Bank1：0x08000000 - 0x080FFFFF，1MB
Bank2：0x08100000 - 0x081FFFFF，1MB
```

第一版推荐分区：

```text
0x08000000 - 0x0801FFFF   Bootloader，128KB
0x08020000 - 0x080FFFFF   AppA，896KB
0x08100000 - 0x081FFFFF   AppB / Upgrade Slot，1024KB
```

AppA 实际运行地址：

```text
APP_A_START    = 0x08020000
APP_A_RUN_ADDR = APP_A_START + 0x400
               = 0x08020400
```

保留的 `0x400 = 1KB` 用于：

```text
App tag
对齐填充
升级状态
预留信息
```

推荐：

```text
0x08020000 - 0x080203BF   reserved
0x080203C0 - 0x080203FF   app_tag_t，64B
0x08020400 - ...          App vector table + App code
```

### 6.2 STM32H743 内部 RAM

H743 RAM 不是一整块，而是多块不同用途的 RAM。

常见分布如下，具体以芯片手册和 CMSIS 头文件为准：

| 区域 | 典型地址 | 大小 | 适合用途 |
|---|---:|---:|---|
| ITCM RAM | `0x00000000` | 64KB | 关键代码 |
| DTCM RAM | `0x20000000` | 128KB | 关键数据、主栈、中断数据 |
| AXI SRAM | `0x24000000` | 512KB | 普通全局变量、heap、LVGL draw buffer |
| SRAM1/2/3 | `0x30000000` 起 | 约 288KB | DMA buffer、外设通信 |
| SRAM4 | `0x38000000` | 64KB | 低功耗域数据 |
| Backup SRAM | `0x38800000` | 4KB | 复位记录、升级标志 |

第一版建议：

```text
DTCM：
  向量表副本
  MSP 栈
  关键状态变量

AXI SRAM：
  FreeRTOS heap
  LVGL draw buffer
  普通全局变量

SRAM1/2/3：
  SPI/UART/DMA buffer

SDRAM：
  LCD framebuffer
  大图片缓存
  可选 LVGL 大 buffer
```

### 6.3 外部 SDRAM

Apollo H743 常见外部 SDRAM 为 32MB。

基地址要以实际 FMC Bank 配置为准，可能是：

```text
0xC0000000
```

也可能是：

```text
0xD0000000
```

文档和代码中不要硬猜，必须根据正点原子 SDRAM 例程或 CubeMX FMC 配置确认。

假设基地址为 `0xC0000000`，800x480 RGB565 推荐规划：

```text
0xC0000000 - 0xC00BB7FF   framebuffer 0，750KiB
0xC00BB800 - 0xC0176FFF   framebuffer 1，可选双缓冲，750KiB
0xC0177000 - 0xC01FFFFF   LVGL 临时区 / DMA2D 临时区
0xC0200000 - 0xC07FFFFF   图片解码缓存 / GUI cache
0xC0800000 - ...          可选外部 heap / 文件缓存
```

第一版只做单 framebuffer：

```text
LCD_FB0 = SDRAM_BASE
LCD_FB_SIZE = 800 * 480 * 2
```

### 6.4 LVGL 内存预算

第一版建议使用 partial buffer。

示例：

```text
LVGL draw buffer 高度：40 行
单 buffer：800 * 40 * 2 = 64000 bytes，约 62.5KiB
双 buffer：约 125KiB
```

推荐：

```text
第一版：单 framebuffer + 双 draw buffer
第二版：单 framebuffer + DMA2D flush
第三版：双 framebuffer + VSYNC 切换
```

LVGL heap 建议：

```text
简单页面：64KB
多页面：128KB
复杂 UI：256KB 或更多
```

### 6.5 外部 W25Q256 Flash

W25Q256 容量：

```text
32MB
```

如果它是普通 SPI Flash：

```text
没有 CPU 直接访问地址
必须通过 w25q_read / erase / program 操作
```

如果它接在 QSPI 且配置了 memory mapped：

```text
可能映射到 0x90000000
```

但这必须通过实际板级例程确认。

第一版逻辑分区：

```text
0x00000000 - 0x0000FFFF   参数区 Workset，64KB
0x00010000 - 0x0001FFFF   统计区 Statistic，64KB
0x00020000 - 0x001FFFFF   Boot 图片 / 开机图，约 1.875MB
0x00200000 - 0x00DFFFFF   GUI 资源区，12MB
0x00E00000 - 0x019FFFFF   GUI 资源升级区，12MB
0x01A00000 - 0x01FFFFFF   App 升级包 / 日志 / 预留
```

## 7. 从 0 开始的执行阶段

### Phase 0：建立资料和工程骨架

目标：

```text
不写驱动，先建仓库、建文档、确认硬件资料。
```

任务：

- 新建 `apollo_h743_lvgl_hmi/`。
- 新建 `Docs/bringup_log.md`。
- 新建 `Docs/memory_map.md`。
- 新建 `Docs/hardware_checklist.md`。
- 搜索本地正点原子资料包。
- 找到 LCD、SDRAM、触摸、W25Q256 例程。
- 如果没有资料，生成缺失资料清单。

验收：

```text
Docs/hardware_checklist.md 填写完成或列出缺失项。
Docs/memory_map.md 写入第一版内存规划。
Docs/bringup_log.md 记录资料来源。
```

### Phase 1：最小裸机工程

目标：

```text
MCU 能启动，串口有日志，LED 能闪。
```

建议用 CubeMX 创建：

```text
芯片：STM32H743IIT6，或按开发板实际型号
时钟：先使用官方例程参数
Debug：Serial Wire
USART：选择开发板默认调试串口
GPIO：LED 引脚
```

实现：

```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USARTx_UART_Init();

    printf("Apollo H743 LVGL HMI start\r\n");

    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        HAL_Delay(500);
    }
}
```

验收：

```text
串口输出启动日志。
LED 以 1Hz 闪烁。
可以下载、复位、断点调试。
```

### Phase 2：系统时钟、Cache、MPU 基线

目标：

```text
建立稳定高频运行基础，但不急着优化性能。
```

建议：

- 时钟参数优先抄正点原子官方 H743 例程；
- 先开启 ICache；
- DCache 可先关闭，等 LCD/SDRAM 稳定后再开；
- 后续需要 DCache 时必须配置 MPU。

记录：

```text
SYSCLK
HCLK
APB1/APB2/APB3/APB4
LTDC pixel clock 来源
FMC SDRAM clock
```

验收：

```text
系统长时间运行不复位。
串口日志稳定。
```

### Phase 3：SDRAM 初始化和 memtest

目标：

```text
外部 SDRAM 可稳定读写。
```

实现：

- 移植正点原子 SDRAM 初始化；
- 或用 CubeMX 配置 FMC SDRAM；
- 写 `sdram_memtest()`；
- 测试 `0x55555555`、`0xAAAAAAAA`、递增地址、walking bit。

示例：

```c
bool sdram_memtest(uint32_t base, uint32_t size)
{
    volatile uint32_t *p = (volatile uint32_t *)base;
    uint32_t words = size / 4;

    for (uint32_t i = 0; i < words; i++) {
        p[i] = 0x55555555;
    }
    for (uint32_t i = 0; i < words; i++) {
        if (p[i] != 0x55555555) return false;
    }

    for (uint32_t i = 0; i < words; i++) {
        p[i] = 0xAAAAAAAA;
    }
    for (uint32_t i = 0; i < words; i++) {
        if (p[i] != 0xAAAAAAAA) return false;
    }

    return true;
}
```

验收：

```text
串口输出 SDRAM test pass。
连续复位 10 次都通过。
```

### Phase 4：LCD 裸 framebuffer 点亮

目标：

```text
不接 LVGL，只让屏幕显示颜色和图案。
```

RGB/LTDC 屏要做：

- 配置 LTDC GPIO；
- 配置 LTDC timing；
- 配置 pixel clock；
- 配置 layer；
- layer framebuffer 地址指向 SDRAM；
- 控制背光；
- 填充 framebuffer。

示例：

```c
#define LCD_W 800
#define LCD_H 480
#define LCD_BPP 2
#define LCD_FB0 ((uint16_t *)SDRAM_BASE)

void lcd_fill_rgb565(uint16_t color)
{
    for (uint32_t i = 0; i < LCD_W * LCD_H; i++) {
        LCD_FB0[i] = color;
    }
}
```

验收：

```text
屏幕显示红、绿、蓝、白、黑。
屏幕显示棋盘格。
屏幕显示横向渐变。
无明显花屏、错位、闪烁。
```

如果花屏，优先排查：

```text
SDRAM 是否真的稳定
LTDC timing 是否和屏幕一致
pixel clock 是否正确
HSYNC/VSYNC/DE 极性是否正确
framebuffer 地址是否正确
DCache 是否影响显存
```

### Phase 5：触摸裸驱动

目标：

```text
先不接 LVGL，串口能打印触摸坐标。
```

实现：

- 初始化触摸芯片；
- 读取按下/松开状态；
- 读取坐标；
- 做坐标方向校正。

串口示例：

```text
TP down x=123 y=456
TP up
```

验收：

```text
点击左上角，坐标接近 0,0。
点击右下角，坐标接近 799,479。
滑动时坐标连续变化。
```

### Phase 6：LVGL 裸机移植

目标：

```text
不用 FreeRTOS，先让 LVGL 在 while 循环里跑起来。
```

LVGL 配置建议：

```c
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (128U * 1024U)
```

draw buffer：

```c
static lv_color_t lv_buf1[800 * 40];
static lv_color_t lv_buf2[800 * 40];
```

显示 flush：

```c
static void disp_flush_cb(lv_disp_drv_t *drv,
                          const lv_area_t *area,
                          lv_color_t *color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    for (uint32_t y = 0; y < h; y++) {
        uint16_t *dst = LCD_FB0 + (area->y1 + y) * LCD_W + area->x1;
        uint16_t *src = (uint16_t *)color_p + y * w;
        memcpy(dst, src, w * 2);
    }

    lv_disp_flush_ready(drv);
}
```

主循环：

```c
while (1) {
    lv_timer_handler();
    HAL_Delay(5);
}
```

tick：

```c
void SysTick_Handler(void)
{
    HAL_IncTick();
    lv_tick_inc(1);
}
```

验收：

```text
屏幕显示 LVGL label、button、slider。
按钮可点击。
slider 可拖动。
```

### Phase 7：LVGL + FreeRTOS

目标：

```text
让 GUI 在 FreeRTOS 任务里跑。
```

建议任务：

| 任务 | 栈建议 | 优先级 | 用途 |
|---|---:|---:|---|
| `task_gui` | 8KB - 16KB | 中 | LVGL |
| `task_storage` | 4KB | 低 | 参数保存 |
| `task_log` | 4KB | 低 | 串口日志 |
| `task_comm` | 4KB - 8KB | 中 | 通信模拟 |
| `task_iap` | 8KB | 中高 | 升级 |

规则：

- LVGL API 只在 `task_gui` 里直接调用；
- 其他任务通过 queue 通知 GUI；
- 不要多个任务随便调用 LVGL；
- 周期打印 stack high water mark。

验收：

```text
FreeRTOS 正常调度。
GUI 不卡死。
串口打印各任务栈余量。
```

### Phase 8：W25Q256 参数保存

目标：

```text
实现用户设置断电保存。
```

数据结构：

```c
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t length;
    uint32_t crc32;
    uint8_t payload[512];
} workset_block_t;
```

保存逻辑：

```text
1. 上电读取参数区。
2. 检查 magic/version/length/crc32。
3. OK 则加载到 RAM 工作副本。
4. 失败则加载默认参数。
5. UI 修改参数时，只设置 dirty。
6. storage_task 延迟保存，避免频繁擦写。
7. 保存前擦除 sector。
8. page program 写入。
9. 写完读回校验。
```

验收：

```text
UI 修改参数。
复位后参数保持。
擦除参数区后系统恢复默认参数。
```

### Phase 9：正式 UI 小项目

目标：

```text
做一个能展示项目能力的 HMI 页面。
```

建议页面：

```text
首页：
  设备状态
  运行时间
  温度/电压/电流模拟值
  通信状态

参数页：
  语言
  亮度
  阈值
  开关量
  保存按钮

升级页：
  当前版本
  新版本
  下载进度
  校验状态
  重启升级按钮

日志页：
  最近事件
  错误码
```

验收：

```text
UI 可操作。
参数能保存。
升级页能显示模拟进度。
```

### Phase 10：Bootloader + App 跳转

目标：

```text
建立 Boot 和 App 两个独立工程。
```

Boot 做：

```text
初始化最小系统
串口打印 Boot 日志
读取 App tag
校验 App
读取 App 向量表
设置 MSP
设置 VTOR
跳转 Reset_Handler
```

App 做：

```text
链接地址从 APP_A_RUN_ADDR 开始
启动后打印 App 日志
复制向量表到 RAM
设置 SCB->VTOR
启动正常业务
```

跳转代码：

```c
typedef void (*app_entry_t)(void);

void boot_jump_to_app(uint32_t app_run_addr)
{
    uint32_t app_msp = *(volatile uint32_t *)(app_run_addr + 0);
    uint32_t app_reset = *(volatile uint32_t *)(app_run_addr + 4);

    __disable_irq();
    HAL_DeInit();

    SCB->VTOR = app_run_addr;
    __set_MSP(app_msp);

    ((app_entry_t)app_reset)();
}
```

验收：

```text
串口先输出 Boot start。
随后输出 App start。
App GUI 正常运行。
```

### Phase 11：App tag 和本地 IAP

目标：

```text
实现最小升级闭环。
```

tag：

```c
typedef struct {
    uint32_t magic;
    uint32_t tag_size;
    uint32_t app_run_addr;
    uint32_t app_size;
    uint32_t version;
    uint32_t crc32;
    uint8_t  sha256[32];
} app_tag_t;
```

第一版只用 CRC32：

```text
CRC32(APP_RUN_ADDR, app_size) == tag.crc32
```

升级流程：

```text
1. App 通过串口/USB/SD 卡接收 new_app.bin。
2. 写入 AppB 或外部 Flash staging 区。
3. 校验 CRC32。
4. 写升级标志。
5. 重启。
6. Boot 检查升级标志。
7. Boot 校验新 App。
8. OK 则复制或切换。
9. 失败则继续旧 App。
```

验收：

```text
version 1 可升级到 version 2。
错误升级包不会破坏旧版本。
```

### Phase 12：远程升级设计

目标：

```text
在本地 IAP 稳定后，增加远程下载能力。
```

远程升级需要先确认网络来源：

| 方式 | 可行性 | 说明 |
|---|---:|---|
| Ethernet | 高 | 如果板子有 PHY/RJ45，推荐 |
| WiFi 模块 | 高 | ESP8266/ESP32 AT 指令或 SPI/UART 模块 |
| 4G 模块 | 高 | 工业项目常见 |
| USB 连接上位机 | 高 | 不算真正远程，但适合调试 |
| SD 卡 | 高 | 本地升级，不依赖网络 |

推荐远程升级包格式：

```text
manifest.json
app.bin
  app.tag
  signature.bin
```

manifest 示例：

```json
{
  "product": "apollo_h743_lvgl_hmi",
  "version": 2,
  "app_size": 812344,
  "crc32": "0x12345678",
  "sha256": "..."
}
```

安全流程：

```text
1. 下载 manifest。
2. 检查产品型号、硬件版本、版本号。
3. 分块下载 app.bin。
4. 每块写入外部 Flash staging。
5. 每块可做 CRC。
6. 全包做 SHA256。
7. 可选 ECDSA/RSA 签名验证。
8. 校验成功后设置 pending 标志。
9. 重启进入 Boot。
10. Boot 完成切换。
11. App 首次启动后设置 confirmed 标志。
12. 如果 App 没确认，Boot 回滚旧版本。
```

必须支持：

```text
断点续传
掉电保护
失败回滚
版本防降级
升级状态日志
```

第一版不要急着做签名，可以先做：

```text
HTTP 下载 + CRC32 + SHA256 + Boot 切换
```

后续再做：

```text
ECDSA 签名
TLS
服务器版本管理
灰度升级
```

### Phase 13：通信框架和模拟 DSP

目标：

```text
在 HMI、参数保存、Boot/IAP 都稳定之后，再做 DSP 通信。
```

这一阶段放在最后做。原因是 DSP 通信依赖很多基础能力：

```text
UART / CAN / SPI 驱动
FreeRTOS 任务
消息队列
协议解析
参数结构
日志系统
界面状态更新
错误处理
```

如果一开始就做 DSP 通信，会把显示、RTOS、协议、存储、调试问题混在一起，不利于学习。

#### 13.1 先做模拟 DSP

第一版不需要真实 DSP 芯片。

推荐方式：

```text
PC 串口工具 / Python 脚本 / 另一块开发板
  -> 通过 UART 给 H743 发送模拟 DSP 数据
  -> H743 解析协议
  -> 更新 LVGL 首页数据
  -> 记录日志
```

模拟数据：

```text
母线电压
输出电压
输出电流
温度
运行状态
告警码
故障码
DSP 固件版本
```

#### 13.2 通信任务设计

建议增加任务：

| 任务 | 栈建议 | 优先级 | 用途 |
|---|---:|---:|---|
| `task_dsp_comm` | 4KB - 8KB | 中高 | 接收/发送 DSP 协议帧 |
| `task_core` | 8KB | 中 | 处理设备状态和参数逻辑 |
| `task_gui` | 8KB - 16KB | 中 | 显示 DSP 状态 |
| `task_log` | 4KB | 低 | 记录通信异常 |

推荐数据流：

```text
UART RX DMA / IRQ
  -> ring buffer
  -> task_dsp_comm 解析协议帧
  -> 校验 CRC
  -> 转成 dsp_data_t
  -> queue 发给 task_core
  -> task_core 更新系统状态
  -> queue 通知 task_gui
  -> task_gui 刷新 LVGL 页面
```

不要让 UART 中断里直接解析复杂协议，也不要让通信任务直接随便改 LVGL 控件。

#### 13.3 协议帧建议

先设计一个简单稳定的协议：

```text
SOF      2B   0xA5 0x5A
CMD      1B
SEQ      1B
LEN      2B
PAYLOAD  nB
CRC16    2B
```

示例：

```c
typedef struct {
    uint16_t bus_voltage_mv;
    uint16_t out_voltage_mv;
    uint16_t out_current_ma;
    int16_t  temp_c_x10;
    uint16_t run_state;
    uint16_t warn_code;
    uint16_t fault_code;
} dsp_status_t;
```

通信方向：

```text
DSP -> ARM：状态、采样值、告警、故障
ARM -> DSP：参数下发、控制命令、升级命令
```

#### 13.4 对齐真实工程的思想

真实工业项目里，ARM 和 DSP 常见分工是：

```text
DSP：
  实时控制
  PWM
  ADC 采样
  快速保护
  电流环/电压环
  高实时性算法

ARM：
  HMI
  参数管理
  日志
  通信协议
  文件/资源管理
  IAP 升级
  和 DSP 交换状态/参数/命令
```

本复现项目先模拟 DSP，是为了学习 ARM 侧通信框架。等框架稳定后，可以替换为真实 DSP、另一块 MCU，或者继续用 PC 脚本模拟。

#### 13.5 验收

```text
PC 每 100ms 发送一帧模拟 DSP 状态。
H743 能稳定解析。
LVGL 首页显示电压、电流、温度变化。
断开通信后 UI 显示 DSP offline。
CRC 错误帧会被丢弃并记录日志。
ARM 可以发送一条参数设置命令，模拟 DSP 回复 ACK。
```

#### 13.6 后续扩展

```text
DSP 参数同步
DSP 固件版本读取
DSP 固件升级流程
通信超时和重连
协议版本兼容
异常帧统计
真实 CAN / RS485 / SPI 通信替换
```

## 8. Codex 第一轮提示词

把下面这段复制给 Codex：

```text
我要从 0 开始在正点原子 Apollo STM32H743 V2 开发板上做一个 LVGL HMI 工程。

硬件：
- STM32H743 开发板
- 4.3寸 RGB 屏
- 分辨率 800x480
- 目标使用 LVGL

请根据 Document/apollo_h743_lvgl_reproduction_plan.md 执行 Phase 0。

要求：
1. 不要假设已经有任何旧工程源码。
2. 新建 apollo_h743_lvgl_hmi/ 目录。
3. 新建 Docs/bringup_log.md、Docs/memory_map.md、Docs/hardware_checklist.md。
4. 搜索当前电脑是否有正点原子 Apollo H743 V2 资料包和例程。
5. 如果没有资料包，请列出我需要补充的文件清单。
6. 先不要写驱动代码。
7. 先完成硬件确认表和第一版内存规划。
```

## 9. Codex 第二轮提示词

```text
继续执行 Phase 1 和 Phase 2。

目标：
1. 创建最小 STM32H743 工程。
2. 实现 HAL_Init、SystemClock_Config、UART printf、LED 心跳。
3. 时钟参数优先来自正点原子官方例程。
4. 记录 SYSCLK/HCLK/APB/LTDC/FMC 时钟。
5. 更新 Docs/bringup_log.md。

要求：
1. 不接 SDRAM。
2. 不接 LCD。
3. 不接 LVGL。
4. 先确认最小系统稳定。
```

## 10. Codex 第三轮提示词

```text
继续执行 Phase 3。

目标：
1. 移植或生成 FMC SDRAM 初始化。
2. 确认 SDRAM 基地址。
3. 实现 sdram_memtest。
4. 串口输出测试结果。
5. 更新 Docs/memory_map.md。

要求：
1. SDRAM 参数必须来自正点原子例程、原理图、数据手册或 CubeMX。
2. 连续复位多次测试。
3. 暂时不接 LCD。
```

## 11. Codex 第四轮提示词

```text
继续执行 Phase 4 和 Phase 5。

目标：
1. 初始化 800x480 RGB LCD。
2. framebuffer 放到 SDRAM。
3. 显示纯色、渐变、棋盘格。
4. 移植触摸驱动。
5. 串口打印触摸坐标。

要求：
1. 先不用 LVGL。
2. LTDC timing 必须来自屏幕资料或官方例程。
3. 如果花屏，优先排查 SDRAM、LTDC timing、pixel clock、DCache。
4. 更新 bringup_log.md。
```

## 12. Codex 第五轮提示词

```text
继续执行 Phase 6。

目标：
1. 移植 LVGL 8.4.x。
2. 配置 RGB565。
3. 使用 SDRAM framebuffer。
4. 使用 800x40 的 LVGL draw buffer。
5. 实现 disp_flush_cb。
6. 实现 touch indev。
7. 创建 button、label、slider 测试页面。

要求：
1. 暂时不用 FreeRTOS。
2. lv_timer_handler 在 while 循环中调用。
3. SysTick 中调用 lv_tick_inc(1)。
4. 屏幕和触摸都通过后再进入下一阶段。
```

## 13. Codex 第六轮提示词

```text
继续执行 Phase 7、Phase 8、Phase 9。

目标：
1. 接入 FreeRTOS。
2. 建立 task_gui、task_storage、task_log。
3. LVGL 只在 task_gui 中直接调用。
4. 实现 W25Q256 参数保存。
5. 做首页、参数页、升级页、日志页。

要求：
1. 参数结构必须包含 magic/version/length/crc32。
2. 修改参数后延迟保存，避免频繁擦写。
3. 复位后参数要保持。
4. 周期打印任务栈余量。
```

## 14. Codex 第七轮提示词

```text
继续执行 Phase 10 和 Phase 11。

目标：
1. 新建 Bootloader 工程。
2. 新建 App 工程。
3. 设计 Boot/AppA/AppB 分区。
4. App 运行地址使用 AppStart + 0x400。
5. 实现 app_tag_t。
6. Boot 校验 App 后跳转。
7. App 启动后复制向量表到 RAM 并设置 SCB->VTOR。
8. 实现串口/USB/SD 卡本地 IAP。

要求：
1. 第一版只做 CRC32。
2. 不做 Option Byte Bank Swap。
3. 失败不能破坏旧 App。
4. 串口日志必须能看出 Boot -> App 的顺序。
```

## 15. Codex 第八轮提示词

```text
继续设计 Phase 12 远程升级。

目标：
1. 分析当前硬件可用的网络方式：Ethernet、WiFi、4G 或上位机。
2. 设计 manifest + app.bin + tag + signature 的升级包格式。
3. 设计下载、断点续传、校验、写 staging、重启、Boot 切换、回滚流程。
4. 先实现一个模拟远程升级：从本地文件或串口分块接收，流程按远程升级设计走。

要求：
1. 不直接覆盖当前运行 App。
2. 必须有 pending/confirmed/rollback 状态。
3. 必须记录升级日志。
4. 安全校验至少支持 SHA256，后续预留签名验证。
```

## 16. Codex 第九轮提示词

```text
继续执行 Phase 13：通信框架和模拟 DSP。

目标：
1. 不接真实 DSP，先用 PC 串口脚本或另一块开发板模拟 DSP。
2. 新增 task_dsp_comm。
3. 使用 UART DMA/中断 + ring buffer 接收数据。
4. 设计简单协议帧：SOF/CMD/SEQ/LEN/PAYLOAD/CRC16。
5. 实现 dsp_status_t 数据解析。
6. 通过 queue 把解析结果发给 task_core 或 task_gui。
7. LVGL 首页显示模拟电压、电流、温度、运行状态、告警码。
8. 通信超时后显示 DSP offline。
9. CRC 错误帧丢弃并记录日志。
10. ARM 能发送参数设置命令，模拟 DSP 回复 ACK。

要求：
1. 不要在 UART 中断里做复杂解析。
2. 不要在通信任务里直接随意操作 LVGL 控件。
3. 所有 UI 更新走 task_gui。
4. 日志记录通信超时、CRC 错误、协议版本错误。
5. 更新 Docs/bringup_log.md，说明 ARM 与 DSP 的职责划分。
```

## 17. 常见问题排查

| 问题 | 典型原因 | 排查 |
|---|---|---|
| SDRAM memtest 失败 | FMC 参数错误、刷新周期错误 | 对照官方例程和 SDRAM 手册 |
| LCD 白屏 | 背光开了但 LTDC 没输出 | 查 framebuffer、LTDC、时钟 |
| LCD 花屏 | timing 错、SDRAM 不稳、DCache | 先关 DCache，确认 timing |
| LVGL 无显示 | flush 没写到 framebuffer | 在 flush_cb 打日志 |
| LVGL 卡死 | tick 没加、flush_ready 没调用 | 检查 SysTick 和 flush_cb |
| 触摸反向 | 坐标映射错误 | 做 x/y 翻转和旋转 |
| FreeRTOS HardFault | 栈太小、heap 太小、中断优先级错 | 开 high water mark |
| 参数不保存 | Flash 未擦除、页写跨界、CRC 错 | 读回校验 |
| Boot 跳 App 死机 | MSP/Reset_Handler/VTOR 错 | 检查 App 链接地址 |
| 升级失败 | tag 地址或 CRC 范围错 | 打印 app_size 和 CRC |
| DSP 通信丢帧 | 中断处理太重、ring buffer 太小 | 中断只收数据，任务解析 |
| DSP 数据乱 | 帧同步失败、大小端不一致 | 检查 SOF、LEN、CRC、字节序 |
| UI 刷新异常 | 通信任务直接操作 LVGL | 统一通过 queue 给 task_gui |

## 18. 面试讲解目标

最终你要能讲清楚：

```text
1. H743 的内部 RAM 为什么要分 ITCM/DTCM/AXI SRAM/SRAM。
2. 800x480 RGB565 为什么一帧是 750KiB。
3. 为什么 framebuffer 放 SDRAM。
4. LVGL draw buffer 和 framebuffer 的区别。
5. LTDC 是如何自动扫描 framebuffer 的。
6. DCache 为什么会导致 LCD 花屏。
7. 用户参数为什么不能只放 RAM。
8. Bootloader 为什么要读 MSP 和 Reset_Handler。
9. App 为什么要设置 SCB->VTOR。
10. App tag 为什么要放在 App 入口前。
11. IAP 为什么要有校验和回滚。
12. 远程升级为什么不能直接覆盖当前运行 App。
13. ARM 和 DSP 为什么要分工。
14. 为什么 DSP 通信放在系统框架稳定后再做。
15. DSP 数据为什么要通过队列进入 GUI，而不是中断里直接刷新界面。
```

## 19. 最小完成标准

这个项目第一阶段完成标准：

```text
1. 裸机 LED + UART 可运行。
2. SDRAM memtest 通过。
3. RGB LCD framebuffer 点亮。
4. LVGL 页面可显示。
5. 触摸可操作。
6. FreeRTOS 多任务可运行。
7. 参数可保存到 W25Q256。
8. Boot 能跳 App。
9. App 能显示版本号。
10. 本地升级 version 1 -> version 2 成功。
```

第二阶段完成标准：

```text
1. 外部 Flash 资源区可用。
2. 图片/字体资源不占内部 Flash。
3. LVGL 页面更完整。
4. DMA2D 加速 flush。
5. DCache + MPU 策略稳定。
6. 远程升级设计文档完成。
7. 模拟远程升级流程跑通。
```

第三阶段完成标准：

```text
1. PC 或另一块开发板能模拟 DSP。
2. H743 能稳定解析 DSP 状态帧。
3. LVGL 页面实时显示 DSP 模拟数据。
4. 通信断开能显示 offline。
5. CRC 错误和超时能记录日志。
6. ARM 能向模拟 DSP 发送参数设置命令并收到 ACK。
```
