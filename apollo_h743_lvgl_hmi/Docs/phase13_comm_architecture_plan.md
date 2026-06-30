# Phase 13 通信框架和模拟通信设计

日期：2026-06-28

本文档用于 Phase 13：通信框架和模拟 DSP / BMS / Modbus。目标不是立刻接真实外设硬件，而是先做一套面试能讲清楚、后续能落地开发的通信架构设计。

当前主工程的 Boot/IAP/USB/SD/LVGL/FreeRTOS 已经形成闭环。Phase 13 必须保持这个主线稳定，不要为了补通信功能破坏 AppA 保底、IAP staging/pending/Boot 安装/confirmed 流程。

## 目标定位

本阶段服务两个目标：

1. 面试目标：把项目讲成一个接近储能逆变器产品的综合型嵌入式系统，覆盖 ARM-DSP 内部通信、BMS CAN、Modbus 对外协议、FreeRTOS 任务解耦和数据流设计。
2. 开发目标：先搭建“模拟协议栈 + 虚拟数据源/回环测试 + 统一消息接口”，后续再逐步替换成真实 SPI/CAN/RS485 外设。

第一版不追求真实硬件闭环，不接真实 DSP，不要求真实 CAN 收发器或 RS485 收发器在线。重点是：

- 通信协议如何分层。
- 任务之间如何解耦。
- 数据如何从通信任务进入核心状态和 UI。
- 如何避免业务模块直接读写全局变量。
- 如何记录超时、CRC 错误、异常帧和状态变化。

## 协议参考资料

本阶段参考 `Docs/` 下两份协议：

```text
DSP和ARM之间通信协议V2.0.14-20251107.docx
深圳迈格瑞能技术混合逆变器_5K_BMS协议V1.04.xlsx
```

### DSP-ARM 协议关键点

从 DSP-ARM 协议文档提取到的核心约束：

```text
场景：储能变流器 DSP 与 ARM/MCU 之间通信
真实链路：SPI
DSP：TMS320F280049，主设备
ARM/MCU：STM32，SPI 从设备
字节序：LittleEndian
传输方式：DMA 一次性交互固定数据包
SPI 波特率：6.25 Mbps
数据位：8 bit
校验：CRC
```

数据帧格式：

```text
Byte0        帧类别段
Byte1        帧 ID 段
Byte2-17     数据段，共 8 个 Word
Byte18       应答码
Byte19       CRC
```

帧类别：

```text
0x01 控制帧
0x02 告警帧和状态帧
0x03 配置帧
0x05 模拟帧
0x06 升级帧
```

应答码：

```text
0x5A 表示收到对应 Byte0/Byte1 的帧
0xAA 表示发送对应 Byte0/Byte1 的帧
0x7C 表示无效帧
```

时序和容错：

```text
500ms 内无应答或帧格式错误，重发同一命令。
连续 3 次失败，判定下位机通信失败，但仍继续查询直到恢复。
写入命令或执行命令收到 NAK，也按 500ms 最小间隔重试。
```

这些规则很适合面试讲“真实产品里的内部高速控制链路”：帧格式固定、DMA 传输、CRC 校验、ACK/NAK、超时重试、通信 offline。

### BMS 协议关键点

BMS 协议用于模拟电池侧外部通信，面试讲述时按 CAN BMS 方向处理：

```text
对象：BMS / 电池包
典型链路：CAN 或 RS485
本阶段建议：先模拟 CAN BMS 报文，不接真实 CAN 硬件
核心数据：SOC、SOH、电压、电流、温度、充放电限值、告警、故障、BMS 在线状态
```

注意：第一版文档不固定具体 CAN ID 和位域。后续真正实现前再从 `深圳迈格瑞能技术混合逆变器_5K_BMS协议V1.04.xlsx` 抽取实际 ID、字节序、缩放系数和告警位定义，避免手工抄错协议。

## 总体架构

Phase 13 推荐把通信分成四层：

```text
Transport 层
  只关心字节流或报文收发，例如 SPI/CAN/UART/虚拟回环。

Protocol 层
  只关心协议帧解析、打包、CRC、序号、ACK/NAK、异常码。

Service 层
  把协议数据转换为系统业务状态，例如 DSP 状态、BMS 状态、Modbus 寄存器。

App/Core/UI 层
  通过统一接口读取快照或订阅事件，不直接读写通信模块内部变量。
```

推荐数据流：

```text
模拟 DSP/SPI
  -> dsp_link protocol
  -> comm_event_queue
  -> task_core
  -> system_model snapshot
  -> task_gui 刷新 LVGL

模拟 BMS/CAN
  -> bms_can protocol
  -> comm_event_queue
  -> task_core
  -> system_model snapshot
  -> task_gui / task_log

模拟 Modbus RTU
  -> modbus_rtu protocol
  -> 读写 system_model 暴露出的寄存器映射
  -> 生成响应帧或异常码
```

关键原则：

- ISR 或虚拟 RX 回调只搬运数据，不解析复杂协议。
- 协议任务只解析协议，不直接操作 LVGL。
- UI 只能在 `task_gui` 中更新。
- 业务状态集中在 `system_model` 或 `task_core`，不要散落多个全局变量。
- 其他模块通过 API 获取快照，或通过队列接收事件。

## 推荐任务划分

当前工程已有 `task_gui`、`task_storage`、`task_log`、`task_iap`。Phase 13 建议新增：

| 任务 | 第一版栈建议 | 优先级建议 | 说明 |
|---|---:|---:|---|
| `task_comm` | 4096-6144 words | 中 | 通信总调度，接收虚拟帧、驱动协议状态机 |
| `task_core` | 4096-6144 words | 中 | 系统状态汇总，处理 DSP/BMS/Modbus 事件 |

如果第一版想降低实现复杂度，可以先只新增 `task_comm`，把 `task_core` 逻辑做成普通模块 `app_system_model.c`，由 `task_comm` 调用接口更新状态。等接口稳定后，再拆出独立 `task_core`。

推荐任务关系：

```text
task_comm
  - 周期生成模拟 DSP 状态帧
  - 周期生成模拟 BMS CAN 报文
  - 解析 Modbus RTU 测试请求
  - 把协议结果 publish 到 comm event queue

task_core
  - 消费 comm event queue
  - 更新 system_model
  - 判断 DSP/BMS online/offline
  - 记录关键状态变化

task_gui
  - 周期读取 system_model snapshot
  - 显示 DSP/BMS/Modbus 状态
  - 不直接调用协议解析接口
```

## 统一消息接口设计

目标是封装任务间通信，不让模块之间靠全局变量互相控制。推荐新增一个轻量通信总线模块：

```text
app_comm_bus.h
app_comm_bus.c
```

第一版只需要一个 FreeRTOS queue，不必做复杂发布订阅框架。

### 事件类型草案

```c
typedef enum
{
    APP_COMM_EVENT_DSP_STATUS = 0,
    APP_COMM_EVENT_DSP_ACK,
    APP_COMM_EVENT_DSP_TIMEOUT,
    APP_COMM_EVENT_DSP_CRC_ERROR,

    APP_COMM_EVENT_BMS_STATUS,
    APP_COMM_EVENT_BMS_LIMITS,
    APP_COMM_EVENT_BMS_ALARM,
    APP_COMM_EVENT_BMS_TIMEOUT,

    APP_COMM_EVENT_MODBUS_WRITE,
    APP_COMM_EVENT_MODBUS_ERROR,
} app_comm_event_type_t;
```

### 事件结构草案

```c
typedef struct
{
    app_comm_event_type_t type;
    uint32_t timestamp_ms;
    uint32_t source;
    uint32_t sequence;
    union
    {
        app_dsp_status_t dsp_status;
        app_bms_status_t bms_status;
        app_modbus_write_t modbus_write;
        uint32_t error_code;
    } data;
} app_comm_event_t;
```

### 接口草案

```c
bool app_comm_bus_init(void);
bool app_comm_publish(const app_comm_event_t *event, uint32_t timeout_ms);
bool app_comm_receive(app_comm_event_t *event, uint32_t timeout_ms);
```

第一版只允许 `task_core` 消费这个队列。后续如果多个任务都需要订阅同一类事件，再升级为多个 queue 或 event group + snapshot。

## 系统状态模型

建议新增：

```text
app_system_model.h
app_system_model.c
```

它是业务状态的唯一汇总点。通信模块不能随意暴露内部全局变量，UI 也不直接读通信模块内部状态。

### 快照结构草案

```c
typedef struct
{
    bool online;
    uint32_t last_rx_ms;
    uint32_t frame_count;
    uint32_t crc_error_count;
    uint32_t timeout_count;

    int16_t bus_voltage_v_x10;
    int16_t grid_voltage_v_x10;
    int16_t output_current_a_x10;
    int16_t temperature_c_x10;
    uint16_t run_state;
    uint16_t warn_code;
    uint16_t fault_code;
    uint16_t firmware_version;
} app_dsp_snapshot_t;

typedef struct
{
    bool online;
    uint32_t last_rx_ms;
    uint32_t frame_count;
    uint32_t timeout_count;

    uint16_t soc_percent_x10;
    uint16_t soh_percent_x10;
    int16_t pack_voltage_v_x10;
    int16_t pack_current_a_x10;
    int16_t max_temp_c_x10;
    uint16_t charge_limit_a_x10;
    uint16_t discharge_limit_a_x10;
    uint32_t alarm_flags;
    uint32_t fault_flags;
} app_bms_snapshot_t;

typedef struct
{
    uint32_t request_count;
    uint32_t exception_count;
    uint32_t crc_error_count;
    uint32_t last_request_ms;
} app_modbus_snapshot_t;

typedef struct
{
    app_dsp_snapshot_t dsp;
    app_bms_snapshot_t bms;
    app_modbus_snapshot_t modbus;
} app_system_snapshot_t;
```

### 状态接口草案

```c
void app_system_model_init(void);
void app_system_model_process_event(const app_comm_event_t *event);
void app_system_model_poll(uint32_t now_ms);
void app_system_model_get_snapshot(app_system_snapshot_t *snapshot);
```

设计要点：

- `process_event()` 只由 `task_core` 或第一版 `task_comm` 调用。
- `get_snapshot()` 可由 `task_gui`、`task_log` 调用，内部用临界区或互斥保护复制快照。
- online/offline 判断集中在 `app_system_model_poll()`，不要让 GUI 自己判断通信超时。

## SPI 模拟 ARM-DSP 内部通信

### 面试讲述定位

这部分用于证明用户理解“ARM 监控 MCU 与 DSP 实时控制器的职责划分”：

```text
DSP：
  ADC 采样、PWM、快速保护、电流环/电压环、实时控制。

ARM：
  HMI、参数管理、日志、通信协议、文件系统、IAP、和 DSP 交换状态/参数/命令。
```

真实项目里 DSP-ARM 走 SPI 是合理的，因为它是板内高速、周期性、低延迟链路。Modbus 不适合作为 ARM-DSP 内部实时通信协议。

### 第一版模拟方式

不接真实 SPI，从 `task_comm` 内部生成“模拟 SPI 帧”，然后送给 `dsp_link` 解析。这样先验证协议状态机和数据流：

```text
virtual_dsp_generator()
  -> 20-byte DSP frame
  -> app_dsp_link_parse_frame()
  -> APP_COMM_EVENT_DSP_STATUS
  -> system_model
```

### 帧格式建议

第一版贴近协议文档，使用固定 20 字节：

```text
Byte0        frame_type
Byte1        frame_id
Byte2-17     payload, 8 words, little-endian
Byte18       ack_code
Byte19       crc8 or simplified crc placeholder
```

为了学习和面试可讲清楚，建议第一版先用软件 CRC8 或 CRC16，不依赖 STM32 SPI 硬件 CRC。后续接真实 SPI/DMA 时再考虑 HAL SPI CRC 或外设 CRC。

### DSP 数据映射建议

模拟帧可包含：

```text
bus_voltage_v_x10
grid_voltage_v_x10
output_current_a_x10
temperature_c_x10
run_state
warn_code
fault_code
firmware_version
```

### DSP 接口草案

```c
bool app_dsp_link_init(void);
bool app_dsp_link_parse_frame(const uint8_t *frame, uint32_t length, app_comm_event_t *event);
bool app_dsp_link_build_command(uint8_t cmd, const void *payload, uint32_t payload_len, uint8_t *frame, uint32_t frame_size);
void app_dsp_link_poll(uint32_t now_ms);
void app_dsp_link_get_diag(app_dsp_link_diag_t *diag);
```

### DSP 错误处理

必须能统计并讲清：

- CRC 错误：丢帧，错误计数增加。
- 帧类别未知：丢帧，记录 invalid frame。
- 超时：超过 500ms 无有效状态帧，记一次超时。
- 连续 3 次超时：`DSP offline`。
- 恢复：收到有效帧后 `DSP online`。

## CAN 模拟 BMS 通信

### 面试讲述定位

这部分用于证明用户理解 BMS 协议适配和 CAN 总线：

- CAN ID 仲裁，ID 越小优先级越高。
- 相同 ID 不能由多个节点同时作为生产者。
- 总线两端各 120 欧终端电阻，不是每个节点都接。
- BMS 数据需要转换成系统内部统一电池状态，而不是 UI 直接读原始 CAN 帧。

### 第一版模拟方式

不接真实 FDCAN/CAN 硬件。先定义一个虚拟 CAN 帧：

```c
typedef struct
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} app_can_frame_t;
```

由 `task_comm` 周期生成 BMS 报文：

```text
virtual_bms_generator()
  -> app_can_frame_t
  -> app_bms_can_parse_frame()
  -> APP_COMM_EVENT_BMS_STATUS / LIMITS / ALARM
  -> system_model
```

### BMS 数据建议

第一版至少模拟：

```text
SOC
SOH
pack_voltage
pack_current
max_cell_temp
charge_current_limit
discharge_current_limit
alarm_flags
fault_flags
online/offline
```

### BMS 接口草案

```c
bool app_bms_can_init(void);
bool app_bms_can_parse_frame(const app_can_frame_t *frame, app_comm_event_t *event);
void app_bms_can_poll(uint32_t now_ms);
void app_bms_can_get_diag(app_bms_can_diag_t *diag);
```

### 后续真实 CAN 替换点

虚拟 CAN 后续替换成真实 FDCAN/CAN 时，只替换 transport：

```text
virtual_can_rx()
  -> app_bms_can_parse_frame()

替换为：

fdcan_rx_callback()
  -> can_rx_queue
  -> task_comm
  -> app_bms_can_parse_frame()
```

协议层和 system model 不应该因为真实硬件接入而重写。

## Modbus RTU 对外协议

### 面试讲述定位

Modbus 用来模拟上位机、客户平台或外部监控系统，不作为 ARM-DSP 内部通信。它体现：

- 对外寄存器模型。
- CRC16。
- 功能码和异常码。
- RS485 半双工方向控制。
- 帧间隔和超时。
- 协议层与业务状态分离。

### 第一版模拟方式

不接真实 RS485 收发器。可以先复用现有 USART1 命令行或 USB CDC 增加测试命令，也可以在 `task_comm` 中做虚拟请求：

```text
virtual_modbus_request()
  -> app_modbus_rtu_parse()
  -> app_modbus_map_read/write()
  -> build response
  -> log response
```

### 寄存器映射建议

把 system model 映射为 Holding/Input Registers：

```text
0x0000 DSP online
0x0001 DSP bus voltage x10
0x0002 DSP grid voltage x10
0x0003 DSP output current x10
0x0004 DSP temperature x10
0x0005 DSP run state
0x0006 DSP warn code
0x0007 DSP fault code

0x0100 BMS online
0x0101 BMS SOC x10
0x0102 BMS SOH x10
0x0103 BMS pack voltage x10
0x0104 BMS pack current x10
0x0105 BMS max temp x10
0x0106 BMS charge limit x10
0x0107 BMS discharge limit x10
0x0108 BMS alarm low word
0x0109 BMS fault low word

0x0200 ARM command word
0x0201 charge current setting x10
0x0202 discharge current setting x10
0x0203 clear fault command
```

### Modbus 接口草案

```c
bool app_modbus_rtu_init(uint8_t slave_addr);
bool app_modbus_rtu_parse_request(const uint8_t *rx, uint32_t rx_len, app_modbus_request_t *request);
bool app_modbus_rtu_build_response(const app_modbus_request_t *request, uint8_t *tx, uint32_t tx_size, uint32_t *tx_len);
bool app_modbus_map_read(uint16_t reg, uint16_t *value);
bool app_modbus_map_write(uint16_t reg, uint16_t value);
void app_modbus_get_diag(app_modbus_diag_t *diag);
```

### 支持功能码建议

第一版只做最小集合：

```text
0x03 Read Holding Registers
0x04 Read Input Registers
0x06 Write Single Register
```

异常码：

```text
0x01 Illegal Function
0x02 Illegal Data Address
0x03 Illegal Data Value
```

## UI 和日志设计

HMI 不需要大改 UI。建议在现有首页或诊断区增加通信摘要，后续实现时可逐步加入：

```text
DSP: online/offline, Vbus, Vgrid, Iout, Temp, State, Warn, Fault
BMS: online/offline, SOC, Vbat, Ibat, Temp, ChgLimit, DsgLimit
MODBUS: req, err, last_ms
```

日志只记录状态变化和异常，不要每帧刷屏：

```text
DSP online
DSP offline timeout=3
DSP crc error count=...
BMS online
BMS alarm flags=...
Modbus illegal address reg=...
```

## 开发步骤

### 每个 Step 完成后的 Git 和记录要求

Phase 13 后续按 Step 小步推进。每完成一个 Step，必须先完成验证、文档记录和 Git 提交，再进入下一个 Step。

每个 Step 完成后要做：

```text
1. 运行本 Step 要求的验证命令，至少包括工程构建；如果改到 USB/SD/IAP/Boot 边界，按 AGENTS.md 的验证边界补充实板验证。
2. 在 Docs/bringup_log.md 追加本 Step 记录，写清：
   - 修改了哪些模块和文件；
   - 新增了什么接口、状态或任务关系；
   - 执行了什么验证命令；
   - 验证结果是通过、失败还是未实板验证；
   - 如果失败，记录错误日志、现象和下一步处理。
3. Git 提交本 Step 的改动，并上传到远端仓库。
4. 提交说明必须能让下一位 Codex 或面试官看懂这一 Step 的价值。
```

推荐提交信息格式：

```text
phase13 step2: add comm bus and system model

- Add app_comm_bus and app_system_model skeletons
- Route UI diagnostics through system model snapshot
- Build: cmake --build --preset gcc-debug passed
- Board/IAP: not rerun in this step
```

注意：

- 不要把多个 Step 混在一个提交里。
- 不要把无关文件、临时日志、构建产物混进 Step 提交。
- 如果验证失败，不要把该 Step 标记为完成；可以提交为 `wip` 或先记录阻塞原因，但文档里必须写清未通过项。
- 如果修改触碰 Boot/IAP/分区/App tag/IAP record/W25Q256 staging，必须先按工程验证边界完成完整构建、烧录和至少一条 IAP 闭环，再提交完成状态。

### Step 1：只做文档和接口冻结

输出：

```text
Docs/phase13_comm_architecture_plan.md
```

确认：

- 模块边界。
- 数据结构草案。
- 任务和队列关系。
- 第一版只做模拟，不接真实硬件。

### Step 2：新增公共数据模型和通信总线

计划新增：

```text
App/Core/Inc/app_comm_bus.h
App/Core/Src/app_comm_bus.c
App/Core/Inc/app_system_model.h
App/Core/Src/app_system_model.c
```

验收：

- 工程构建通过。
- `task_gui` 可读取 `app_system_model_get_snapshot()`。
- 没有真实通信时，UI 显示 offline/default。

### Step 3：新增 DSP 模拟链路

计划新增：

```text
App/Core/Inc/app_dsp_link.h
App/Core/Src/app_dsp_link.c
```

验收：

- 周期生成模拟 DSP 帧。
- 能解析有效帧。
- CRC 错误帧丢弃。
- 超时后 DSP offline。
- 可以构造一条 ARM -> DSP 参数设置命令并模拟 ACK。

### Step 4：新增 BMS CAN 模拟链路

计划新增：

```text
App/Core/Inc/app_bms_can.h
App/Core/Src/app_bms_can.c
```

验收：

- 周期生成模拟 BMS CAN 帧。
- 解析 SOC、电压、电流、温度、限值、告警。
- 超时后 BMS offline。
- 日志记录 BMS 告警变化。

### Step 5：新增 Modbus RTU 模拟协议

计划新增：

```text
App/Core/Inc/app_modbus_rtu.h
App/Core/Src/app_modbus_rtu.c
```

验收：

- 支持 0x03/0x04/0x06。
- CRC16 正确。
- 非法地址返回异常码。
- 读寄存器能读到 DSP/BMS 快照。
- 写命令能通过 system model 进入 command path，而不是直接改底层全局变量。

### Step 6：统一到 task_comm / task_core

计划修改：

```text
App/Core/Inc/app_tasks.h
App/Core/Src/app_tasks.c
```

验收：

- 新增任务栈水位统计。
- 周期日志能看到 `task_comm` / `task_core` 水位。
- 不影响 `task_iap`、USB CDC、SD IAP。

### Step 7：整理 HMI 通信诊断显示

计划修改：

```text
App/Core/Inc/app_system_model.h
App/Core/Src/app_system_model.c
App/Core/Src/app_modbus_rtu.c
App/Core/Src/app_ui_hmi.c
```

验收：

- HMI 首页能直接看到 DSP online、Vbus、Grid、Iout、Temp、run/warn/fault、frame、ack、CRC/timeout。
- HMI 首页能直接看到 BMS online、SOC/SOH、电压、电流、温度、充放电限值、alarm/fault、frame、timeout。
- HMI 首页能直接看到 Modbus req/write/exception/CRC、最后一次写寄存器和值、最后错误类型。
- Modbus 0x06 写成功后通过 `APP_COMM_EVENT_MODBUS_WRITE` 把寄存器和值送进 `system_model`，非法写地址不进入写命令快照。
- UI 仍只在 `task_gui` 中读取 `app_system_model_get_snapshot()`，不直接调用 DSP/BMS/Modbus 协议解析接口。
- 本 Step 不接真实 SPI/CAN/RS485，不修改 Boot/IAP/USB/SD 链路。

### Step 8：整理 Phase 13 面试讲述稿

计划新增：

```text
Docs/phase13_interview_script.md
```

验收：

- 能用 90 秒讲清 Phase 13 的目标、三条通信链路和 RTOS 任务解耦。
- 能用 3-5 分钟讲清 SPI-DSP、CAN-BMS、Modbus RTU、comm bus、system model、GUI snapshot 的完整数据流。
- 明确区分“当前已完成软件模拟闭环”和“后续才接真实 SPI/CAN/RS485 硬件”，不要把学习项目包装成量产完成品。
- 准备资深面试官追问答案：为什么不用全局变量、queue 长度是否够、为什么 ARM-DSP 不用 Modbus、为什么 UI 不直接读通信模块、后续真实硬件如何替换 transport。
- 本 Step 只改文档，不改固件逻辑。

### Step 9：接入板载真实 RS485 Modbus RTU

计划新增：

```text
App/Core/Inc/app_io_expander.h
App/Core/Src/app_io_expander.c
App/Core/Inc/app_rs485_modbus.h
App/Core/Src/app_rs485_modbus.c
```

计划修改：

```text
App/Core/Inc/uart.h
App/Core/Src/uart.c
App/Core/Inc/app_modbus_rtu.h
App/Core/Src/app_modbus_rtu.c
App/Core/Src/main.c
App/Core/Src/app_tasks.c
CMakeLists.txt
Docs/hardware_checklist.md
Docs/bringup_log.md
```

硬件事实：

```text
板载 RS485 收发器：U9 TPT8485
UART：USART2
MCU TX：PA2 / USART2_TX，经 P7 接 RS485_RX / TPT8485 DI
MCU RX：PA3 / USART2_RX，经 P7 接 RS485_TX / TPT8485 RO
方向控制：RS485_RE -> PCF8574T U11 P6 -> TPT8485 /RE 和 DE
PCF8574 I2C：PH4=SCL，PH5=SDA，7-bit 地址 0x20
跳帽：P7 必须插到 RS485 位置，否则 USART2 会接到 RS232 COM2 路径
```

设计边界：

- 不重写 `app_modbus_rtu` 协议层，只新增 transport。
- `app_modbus_rtu_process_frame()` 统一处理真实帧和虚拟帧，复用 CRC、异常码、寄存器映射和 `system_model` 事件通路。
- `app_rs485_modbus` 只负责 USART2 收发、RTU 帧间隔、半双工方向控制和诊断计数。
- `app_io_expander` 只封装 PCF8574 写输出；默认 shadow 为 `0xFF`，仅控制 P6，避免误拉低其它扩展 IO。
- USART1/CH340 继续保留为日志和串口 IAP，不作为 RS485 Modbus。
- 如果 PCF8574 初始化失败或未检测到板载 IO 扩展器，固件保留原来的虚拟 Modbus 模拟，不阻断主系统启动。

第一版参数：

```text
从站地址：1
串口参数：115200, 8N1
USART2 接收：HAL_UARTEx_ReceiveToIdle_DMA() + DMA1 Stream0 + USART2 IDLE 中断
RTU 帧切分：IDLE 事件只作为“收到一段数据”的触发，真正交给协议层前继续等待按波特率计算的 3.5 字符静默时间
T3.5 计算：8N1 按 10 bit/char，ceil(10 * 3.5 * 1000000 / 115200) = 304 us
支持功能码：0x03、0x04、0x06
```

验收：

- 本地 `cmake --build --preset gcc-debug` 通过。
- 串口日志出现 `Phase 13 RS485 Modbus init OK` 或明确 skipped 原因。
- P7 插到 RS485 位后，用 USB-RS485 转换器连接板载 RS485 A/B。
- PC 作为 Modbus master，MCU 作为 slave，115200 8N1，slave id 1。
- 读 Holding `0x0000-0x0007` 能读到 DSP 快照。
- 读 Input/Holding `0x0100-0x0109` 能读到 BMS 快照。
- 写 Holding `0x0201` 能更新 Modbus 写命令计数，并通过 `APP_COMM_EVENT_MODBUS_WRITE` 进入 `system_model`。
- 非法地址返回 Modbus 异常响应，CRC 错误不回包但增加 crc 错误计数。
- 周期日志能看到 `rs485 modbus: ready/baud/t35_us/rx_bytes/rx_events/rx_frames/tx_frames/overflow/short/rx_err/tx_err`。

## 面试讲述方式

建议这样讲：

```text
这个项目里我把通信分成内部链路、外部电池链路和客户协议链路：

内部链路是 ARM-DSP，用 SPI 模拟，强调周期性、低延迟、CRC、ACK/NAK 和 offline。
外部电池链路是 BMS，用 CAN 模拟，强调 CAN ID、仲裁、终端电阻和 BMS 数据统一建模。
客户协议链路是 Modbus RTU，强调寄存器映射、CRC16、异常码和 RS485 半双工。

三条链路都不直接改 UI，也不随便写全局变量，而是统一 publish 到 comm bus，
由 core/system_model 汇总成快照，GUI 任务只读快照刷新界面。
这样后续把虚拟 transport 换成真实 SPI/CAN/RS485 时，协议层、业务层和 UI 不需要重写。
```

被追问时重点回答：

- 为什么 ARM-DSP 不用 Modbus：内部实时链路更适合 SPI，自定义固定帧效率更高。
- 为什么先模拟：先验证架构和协议状态机，避免真实硬件问题、RTOS 问题、UI 问题混在一起。
- 为什么不用全局变量：全局变量会让任务耦合、竞态和 UI 更新路径失控；快照和队列更容易验证。
- 为什么 UI 不直接读通信模块：通信模块可能处于 ISR/任务上下文，LVGL 只能在 GUI task 中调用。
- 后续怎么接真实硬件：只替换 transport 层，协议和 system model 保持不变。

## 风险和边界

- 不要在 Phase 13 第一版修改 Boot/IAP 分区、App tag、IAP record 或 W25Q256 staging 地址。
- 不要让通信模拟占用 USART1 的现有日志/IAP 命令路径，除非设计清楚命令前缀和互斥。
- 不要在通信任务中直接调用 LVGL。
- 不要在 ISR 中解析复杂协议或写系统状态。
- 不要把 DSP/BMS/Modbus 的数据散落成多个裸全局变量。
- 不要一次性实现真实 SPI + CAN + Modbus RTU 硬件，否则会打断当前 IAP 面试主线。

## 第一版完成标准

```text
1. 文档说明清楚三条通信链路的职责：SPI-DSP、CAN-BMS、Modbus-外部平台。
2. 接口草案能指导后续代码落地。
3. 所有通信数据统一进入 comm bus 或 system model。
4. UI 只读取 snapshot，不直接读写协议模块内部变量。
5. 能用日志或虚拟数据证明 DSP/BMS online/offline、CRC 错误、Modbus 异常码。
6. 不影响现有 Boot/IAP/USB/SD 已验证闭环。
```
