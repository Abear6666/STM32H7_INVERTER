# Phase 13 通信框架面试讲述稿

日期：2026-06-28

本文用于面试讲述 Phase 13：通信框架和模拟 DSP / BMS / Modbus。重点不是把项目包装成量产完成品，而是把架构边界、RTOS 任务解耦、协议分层、数据流和后续真实硬件替换路径讲清楚。

## 一句话项目定位

```text
我在 STM32H743 的 LVGL HMI + IAP 项目上，补了一套通信架构演示层：
内部 ARM-DSP 链路按 SPI 固定帧思路模拟，外部 BMS 按 CAN 报文模拟，客户平台按 Modbus RTU 寄存器协议模拟。
三条链路统一进入 FreeRTOS queue，再由核心任务汇总到 system model，GUI 只读快照刷新，不让通信模块、业务状态和 LVGL 互相耦合。
```

必须讲清楚的边界：

```text
当前 Phase 13 第一版是纯软件模拟通信闭环，还没有接真实 SPI、FDCAN/CAN 收发器和 RS485 硬件。
目的先是验证架构、协议状态机、任务解耦和诊断可视化，后续真实硬件接入时优先替换 transport 层。
Boot/IAP/USB/SD 主链路没有在 Phase 13 中改分区或改升级流程。
```

## 90 秒版本

```text
这个项目原来已经有 Bootloader、AppA/AppB、USB/SD IAP、LVGL HMI 和 FreeRTOS 任务框架。
Phase 13 我补的是通信架构，不是直接堆外设驱动。

我把通信分成三条链路：
第一条是 ARM-DSP 内部链路，真实产品里更适合 SPI，因为它是板内、高速、固定周期、低延迟的控制数据交换；这里先用 20 字节固定帧模拟，带 CRC、ACK、超时和掉线。
第二条是 BMS 链路，按 CAN 报文模拟，用 0x351、0x355、0x359 分别表示状态、限值、告警，强调 CAN ID、仲裁、终端电阻和 BMS 数据建模。
第三条是 Modbus RTU，对外模拟客户平台或上位机，支持 0x03、0x04、0x06、CRC16 和异常码，把 system model 映射成寄存器。

任务上，我没有让这些模块直接改 UI 或互相读写全局变量。
task_comm 100ms 周期推进 DSP/BMS/Modbus 协议模拟，事件 publish 到 app_comm_bus。
task_core 50ms 消费 queue，把事件统一写入 app_system_model。
task_gui 只读 system model 快照刷新 LVGL 首页。
task_log 只做心跳和诊断日志。

这样做的好处是，后续接真实 SPI/CAN/RS485 时，协议层、核心状态和 UI 不需要大改，只替换 transport 输入；同时面试里也能讲清楚 RTOS 任务边界、队列通信、快照模型和异常诊断。
```

## 3-5 分钟版本

### 1. 为什么这样拆

```text
逆变器或储能监控类产品通常不只有一个通信口。
ARM 侧既要和 DSP 实时控制器交换状态和参数，也要接 BMS，还要对外提供客户协议。
如果所有模块都直接改全局变量，再让 UI 到处读，很快会出现竞态、耦合和调试困难。
所以我先设计了一个分层模型：transport、protocol、service/system model、UI。
```

四层含义：

```text
transport：真实 SPI/CAN/UART/RS485 或当前虚拟帧输入，只负责收发字节或报文。
protocol：解析帧格式、CRC、功能码、ACK/NAK、异常码。
service/system model：把协议数据转换成统一业务状态，例如 DSP 快照、BMS 快照、Modbus 统计。
UI：只读取快照，不直接解析协议，也不直接调用 LVGL 以外任务里的接口。
```

### 2. RTOS 任务和数据流

当前任务边界：

```text
task_comm：
  100ms 周期轮询 app_dsp_link_poll()
  100ms 周期轮询 app_bms_can_poll()
  100ms 周期轮询 app_modbus_rtu_poll()
  只负责通信模拟和协议状态机推进

app_comm_bus：
  FreeRTOS queue，长度 16
  事件类型包括 DSP_STATUS、DSP_ACK、DSP_TIMEOUT、DSP_CRC_ERROR、
  BMS_STATUS、BMS_LIMITS、BMS_ALARM、BMS_TIMEOUT、
  MODBUS_REQUEST、MODBUS_WRITE、MODBUS_ERROR

task_core：
  50ms 周期最多消费 16 个事件
  调用 app_system_model_process_event()
  调用 app_system_model_poll() 维护 online/offline 超时

task_gui：
  周期读取 app_system_model_get_snapshot()
  刷新 LVGL 首页通信诊断

task_log：
  1s LED 心跳
  5s 打印 stack watermark、heap、IAP、DSP/BMS/Modbus 诊断
```

面试可以这样总结：

```text
通信任务生产事件，核心任务消费事件，系统模型保存快照，GUI 只读快照。
这样把通信时序、业务状态和界面刷新拆开了。
```

### 3. ARM-DSP 链路怎么讲

当前实现事实：

```text
模块：app_dsp_link
模拟帧长：20 字节
状态帧周期：100ms
超时阈值：500ms
CRC：软件 CRC16，使用 Modbus 多项式
错误注入：每 40 个序列注入一次 CRC 错误
命令注入：每 30 个状态周期模拟一条 ARM -> DSP 命令，并模拟 ACK
掉线窗口：每 15s 有 700ms dropout，用于验证 offline
```

口述方式：

```text
DSP 更适合负责 ADC、PWM、保护和电流环/电压环，ARM 更适合 HMI、参数、日志、文件系统、IAP 和对外通信。
ARM-DSP 是板内链路，所以我不选 Modbus。
Modbus 帧开销大，主从轮询和寄存器模型更适合外部平台；板内周期状态交换更适合 SPI 固定帧。

我用 20 字节固定帧模拟 SPI 数据包：帧类型、序号、16 字节 payload、CRC。
payload 映射 Vbus、Grid、Iout、温度、run_state、warn、fault、firmware version。
解析成功发布 DSP_STATUS；CRC 错误发布 DSP_CRC_ERROR；超时发布 DSP_TIMEOUT。
```

被追问时要承认：

```text
当前还没启用真实 SPI DMA，也没处理真实片选、中断、DMA cache 一致性。
下一步接真实硬件时，会把虚拟帧生成替换成 SPI DMA RX/TX 完成回调，回调只搬运数据到队列，不在 ISR 里解析复杂协议。
```

### 4. BMS CAN 链路怎么讲

当前实现事实：

```text
模块：app_bms_can
虚拟 CAN 帧：id + dlc + data[8]
周期：250ms
超时阈值：1000ms
掉线窗口：每 20s 有 1300ms dropout
0x351：SOC、SOH、电池电压、电池电流
0x355：最高温度、充电限流、放电限流
0x359：alarm_flags、fault_flags
```

口述方式：

```text
BMS 链路我按 CAN 来模拟，因为电池包和逆变器之间常见 CAN 或 RS485。
这里选择 CAN 是为了能讲清楚 CAN ID、仲裁、终端电阻和报文生产者边界。

我没有让 UI 直接读 CAN 原始帧，而是把 0x351、0x355、0x359 解析成统一 BMS 快照。
这样后面就算客户协议换 ID 或换字节定义，只改 BMS 协议适配层，UI 和核心业务不需要改。
```

CAN 追问回答：

```text
CAN ID 越小优先级越高，仲裁发生在总线显性/隐性位上。
相同 ID 不应该由多个节点同时作为生产者，否则仲裁无法区分，数据段不同会导致错误帧。
终端电阻是总线两端各 120 欧，不是每个节点都接。
```

被追问当前边界：

```text
当前没有接真实 FDCAN，也没有配置 bit timing、滤波器和收发器待机脚。
后续接入时会增加 can_rx_queue，FDCAN RX callback 只入队，task_comm 中再调用 app_bms_can_parse_frame()。
```

### 5. Modbus RTU 链路怎么讲

当前实现事实：

```text
模块：app_modbus_rtu
模拟轮询周期：1000ms
支持功能码：0x03 Read Holding Registers，0x04 Read Input Registers，0x06 Write Single Register
CRC：Modbus RTU CRC16
异常码：0x01 Illegal Function，0x02 Illegal Data Address，0x03 Illegal Data Value
最大帧缓存：64 字节
```

寄存器映射：

```text
0x0000-0x0007：DSP online、Vbus、Grid、Iout、Temp、run、warn、fault
0x0100-0x0109：BMS online、SOC、SOH、Vbat、Ibat、Temp、充电限流、放电限流、alarm、fault
0x0200-0x0203：ARM 命令和参数写寄存器
```

口述方式：

```text
Modbus 我定位成外部平台或客户监控协议，不用于 ARM-DSP 内部通信。
它的价值是寄存器模型清楚，容易和上位机或客户系统对接，但实时性和效率不适合板内高速控制链路。

读请求从 system model 快照取值。
写请求只允许 0x0200-0x0203 这些命令寄存器。
0x06 写成功后才发布 MODBUS_WRITE 事件，把寄存器和值交给 task_core 写入 system model。
如果写非法地址，只走异常响应，不进入业务命令快照。
```

被追问时补充：

```text
真实 RS485 接入时还要处理 DE/RE 方向控制、3.5 字符帧间隔、接收超时、DMA/IDLE 中断和半双工冲突。
当前第一版没有接真实 UART/RS485，是为了先验证寄存器映射、CRC、异常码和业务解耦。
```

## 资深面试官追问与回答

### Q1：为什么不用全局变量，队列和快照是不是过度设计？

```text
不是为了复杂而复杂。
通信模块产生的是事件流，UI 需要的是稳定快照。
如果直接用全局变量，协议解析、业务判断和 UI 刷新会耦合在一起，多个任务读写时也不好证明一致性。

现在的设计是：
事件用 queue 表达变化；
system model 用临界区维护快照；
GUI 只读快照；
日志只读诊断。
这能把实时通信和低频显示解耦，也方便后续替换真实硬件。
```

### Q2：queue 长度为什么是 16，task_core 50ms 消费 16 个事件够吗？

```text
这是第一版模拟闭环的保守配置。
DSP 100ms 约 1 个状态事件，BMS 250ms 一次产生 3 帧，Modbus 1000ms 一个请求，事件量很小。
task_core 50ms 最多消费 16 个事件，明显大于当前生产速率。

真实硬件接入后要重新按最坏情况估算：
CAN burst、SPI 周期、Modbus 连续请求、队列溢出计数和 task_core 水位。
当前 app_comm_bus 已有 dropped_count，后续可以把它显示到诊断页。
```

### Q3：为什么 task_comm 和 task_core 优先级都不高？

```text
当前是模拟通信，没有硬实时闭环。
真正快速控制不在 ARM HMI 任务里做，DSP 才负责控制环。
ARM 侧通信和 UI 属于监控与管理，不能影响 IAP、存储和系统稳定性。

后续接真实 SPI/CAN 时，如果有严格周期，会根据数据时限调整优先级，或者把硬件 RX 先放进 ISR/DMA queue，再由任务解析。
```

### Q4：为什么 LVGL 不能在通信任务里直接调用？

```text
LVGL 通常不是线程安全的。
如果多个任务直接调用 LVGL API，容易出现对象状态竞争、绘制异常或崩溃。
所以项目里约束 LVGL 只在 task_gui 调用。
其他任务只能更新数据模型或发事件，GUI 周期读取快照刷新。
```

### Q5：现在的模拟有什么实际价值？

```text
价值不是假装硬件已经完成，而是先把架构风险拆出来。
如果直接同时接 SPI、CAN、RS485、UI，很难判断问题来自硬件、协议、RTOS 还是界面。
先模拟可以验证：
任务边界是否合理；
协议解析和异常计数是否完整；
system model 是否能承载业务状态；
HMI 是否能显示关键诊断；
后续真实 transport 替换点是否清晰。
```

### Q6：后续真实硬件怎么接？

```text
SPI-DSP：
  增加 SPI DMA 双缓冲或固定周期收发。
  DMA 完成中断只投递原始帧或通知 task_comm。
  task_comm 调用 app_dsp_link_parse_frame()。

CAN-BMS：
  配置 FDCAN bit timing、filter、RX FIFO。
  RX callback 把 id/dlc/data 入 can_rx_queue。
  task_comm 从 can_rx_queue 取帧，再调用 app_bms_can_parse_frame()。

RS485-Modbus：
  UART DMA + IDLE 或定时器判断帧结束。
  控制 DE/RE 半双工方向。
  收到完整 RTU 帧后调用 app_modbus_rtu_parse_request()，再 build response。

上层 system model、GUI 和日志基本保持不变。
```

## 不能夸大的地方

面试中不要说：

```text
真实 SPI-DSP 已经跑通。
真实 CAN BMS 已经跑通。
真实 RS485 Modbus 已经跑通。
通信协议已经达到量产稳定。
```

应该说：

```text
当前已经完成软件模拟闭环和架构验证。
Boot/IAP/USB/SD 是实板闭环更完整的主线。
Phase 13 通信用于补齐面试可讲的协议分层和任务解耦，下一步再替换真实 transport。
```

## 3 分钟表达模板

```text
我这个项目的主线是 STM32H743 上的 LVGL HMI 和可靠 IAP，已经有 Bootloader、AppA/AppB、USB/SD 升级和 FreeRTOS 任务。
后面我补了一个通信框架，用来模拟逆变器产品里常见的三类通信。

第一类是 ARM 和 DSP 的内部链路。我按 SPI 固定帧来设计，因为 DSP 负责实时控制，ARM 负责 HMI、参数、日志和升级，两者之间需要周期性、低延迟的数据交换。当前用 20 字节帧模拟，带 CRC、ACK、超时和掉线。

第二类是 BMS 通信。我用 CAN 报文模拟，状态、限值、告警拆成几个 CAN ID，然后解析成统一 BMS 快照。这里重点不是 UI 读原始 CAN，而是协议层把原始报文转换成系统内部统一模型。

第三类是外部客户协议。我用 Modbus RTU 模拟，支持 0x03、0x04、0x06、CRC16 和异常码，把 DSP/BMS/system command 映射到寄存器。写命令只有合法地址才进入业务事件。

RTOS 上我拆了 task_comm 和 task_core。task_comm 推进协议状态机，把事件发到 FreeRTOS queue；task_core 消费事件，更新 system model；task_gui 只读快照刷新 LVGL；task_log 只做心跳和诊断。这样避免通信模块直接改 UI 或互相写全局变量。

现在这部分还是模拟通信，不是说真实 SPI/CAN/RS485 都已经接好了。它的价值是把协议分层、任务解耦、异常计数、HMI 诊断和后续硬件替换路径先验证清楚。后续接真实硬件时，主要替换 transport 层，上面的协议解析、system model 和 UI 可以保持。
```

## 复盘重点

```text
结论：
Phase 13 已经能作为面试里的“通信架构和 RTOS 解耦”亮点，但不能替代实板 SPI/CAN/RS485 经验。

下一步：
1. 上板看 HMI 首页通信诊断是否完整可读。
2. 记录 task_comm/task_core 栈水位和 heap。
3. 做一轮串口日志截图或摘录，证明 DSP/BMS/Modbus 计数会变化。
4. 再决定是否接真实 CAN 或 RS485，优先选一个外设做真实闭环，不要一次性全接。
```
