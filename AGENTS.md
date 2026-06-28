# Codex 工程交接说明

本文档给后续 Codex / 代码助手进程作为入口。进入本仓库后先读本文件，再按任务需要阅读 `apollo_h743_lvgl_hmi/Docs/` 下的详细文档。

## 项目目标

当前主工程是 `apollo_h743_lvgl_hmi/`，目标是在 STM32H743 开发板上实现一套可实板运行的 LVGL HMI 和本地 IAP 升级系统。

核心目标：

- Bootloader + AppA + AppB 三分区启动架构。
- AppA 作为保底固件，AppB 作为升级槽。
- 支持串口、USB CDC、SD 卡三种本地升级入口。
- 升级包先写入外部 W25Q256 staging 区，再由 Bootloader 安装到内部 Flash AppB 槽。
- Bootloader 必须保证失败不破坏 AppA。
- AppB 试运行成功后写入 confirmed，后续复位稳定启动 AppB。
- HMI 使用 LVGL，当前重点不是业务控制算法，而是显示、诊断、参数保存和 IAP 链路。

## 技术栈

- MCU：STM32H743，Cortex-M7。
- 固件语言：C11 + ARM GCC。
- 构建：CMake 3.22+、Ninja、ARM GNU Toolchain。
- 外设库：STM32 HAL / CMSIS。
- RTOS：FreeRTOS V10.3.1。
- GUI：LVGL 8.4.0。
- 文件系统：FatFs。
- USB：STM32 USB Device Library，CDC 虚拟串口。
- 外部 Flash：W25Q256，QSPI 普通命令模式。
- 显示链路：SDRAM framebuffer + LTDC RGB LCD。
- 工具：Python 打包脚本、.NET 10 串口/USB IAP CLI/GUI、OpenOCD + ST-LINK。

## 目录结构

仓库根目录：

- `README.md`：仓库概览、构建环境、提交范围。
- `AGENTS.md`：当前交接文件。
- `apollo_h743_lvgl_reproduction_plan.md`：早期复现/迁移计划。
- `【正点原子】...资料A盘/`：参考资料，不作为主工程代码直接修改。
- `sscomV5131.exe`：串口工具，不提交新版本。

主工程 `apollo_h743_lvgl_hmi/`：

- `Boot/`：Bootloader，包括启动选择、IAP pending 处理、AppB 安装、跳转 App。
- `App/`：AppA/AppB 共用应用代码，包括 LVGL、FreeRTOS、USB、SD、IAP、诊断。
- `Common/`：Boot 和 App 共用的数据结构与算法，例如 CRC32、IAP record、App tag。
- `Middlewares/`：FreeRTOS、LVGL、FatFs、USB Device Library。
- `Tools/pack_app/`：App slot 镜像打包脚本。
- `Tools/iap_serial_cli/`：串口/USB CDC IAP 命令行工具。
- `Tools/iap_serial_gui/`：串口/USB CDC IAP GUI 工具。
- `Docs/`：工程事实来源，修改重要行为时必须同步。
- `build/gcc-debug/`：本地构建产物，通常不提交。
- `build/reference/phase7_9/`：少量人工保留参考源码片段，只读对照为主。

重要文档：

- `Docs/iap_design.md`：IAP 分区、协议、Boot/App 流程、USB/SD 当前状态。
- `Docs/memory_map.md`：内部 Flash、W25Q256、RAM、AppA/AppB 地址和当前构建产物。
- `Docs/hardware_checklist.md`：硬件接口、跳帽、USB/SD/诊断状态。
- `Docs/bringup_log.md`：阶段性调试记录和实板回归结果。
- `Docs/usb_iap_case_study.md`：USB/SD IAP 问题复盘、面试讲述稿。

## 常用命令

配置和构建：

```powershell
cd apollo_h743_lvgl_hmi
cmake --preset gcc-debug
cmake --build --preset gcc-debug
```

构建 IAP 工具：

```powershell
cd apollo_h743_lvgl_hmi
dotnet build Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release
dotnet build Tools/iap_serial_gui/ApolloIapSerialGui.csproj -c Release
```

完整烧录 Bootloader + AppA + AppB：

```powershell
cd apollo_h743_lvgl_hmi
openocd -c "set DUAL_BANK 1" -f interface/stlink.cfg -f target/stm32h7x.cfg -c "transport select swd" -c "adapter speed 1000" -c "program build/gcc-debug/apollo_h743_bootloader.hex verify" -c "program build/gcc-debug/app_a_slot.hex verify" -c "program build/gcc-debug/app_b_slot.hex verify reset exit"
```

USB CDC IAP 示例，实际端口以 Windows 枚举为准：

```powershell
cd apollo_h743_lvgl_hmi
.\Tools\iap_serial_cli\bin\Release\net10.0-windows\ApolloIapSerialCli.exe --port COM4 --baud 115200 --file build\gcc-debug\app_b_slot.bin --version 2 --chunk 128 --delay-ms 2 --app-timeout-ms 30000 --ready-timeout-ms 10000 --log-tail-ms 8000 --no-wait-app
```

SD 卡 IAP：

```text
1. 将 build/gcc-debug/app_b_slot.bin 复制到 SD 卡根目录。
2. 文件名保持 app_b_slot.bin。
3. 插入开发板。
4. 串口发送 iap sd，或在 HMI 升级页点击 SD File。
5. 复位后观察 Boot 是否安装 AppB，AppB 是否 confirmed。
```

## 关键地址和协议

内部 Flash：

```text
0x08000000 - 0x0801FFFF   Bootloader
0x08020000 - 0x080FFFFF   AppA
0x08100000 - 0x081FFFFF   AppB
```

App 运行地址：

```text
AppA run = 0x08020400
AppB run = 0x08100400
```

App 槽头：

```text
slot base + 0x000 - 0x3BF   reserved
slot base + 0x3C0 - 0x3FF   app_tag_t，固定 64 字节
slot base + 0x400           向量表和 App 本体
```

W25Q256 IAP 区：

```text
0x01A00000 - 0x01A00FFF   pending record
0x01A01000 - 0x01A01FFF   boot state record
0x01A02000 - 0x01FFFFFF   staging data
```

升级包规则：

- 推荐升级包是 `build/gcc-debug/app_b_slot.bin`。
- 该文件包含 `0x400` 槽头、`app_tag_t` 和 AppB 本体。
- Boot 只允许把合法 AppB 包安装到 AppB 槽。
- CRC32 分两层：整包 CRC 用于 staging 校验，App body CRC 用于 App tag 校验。

## 修改代码边界

必须遵守：

- 不要破坏 AppA 保底逻辑。
- 不要让任何失败路径擦除 AppA。
- 不要引入 Option Byte Bank Swap，当前设计明确不使用。
- 不要随意改 Flash/W25Q256 分区地址；如必须改，先同步 `Docs/memory_map.md`、Boot、App、打包脚本和验证命令。
- 不要随意改 `app_tag_t` 或 IAP record 二进制布局；如必须改，需要兼容旧记录或写清迁移策略。
- Boot 跳转 App 前必须清理 CPU 状态，包括 MSP、VTOR、SysTick、中断 pending、NVIC enable、PRIMASK/BASEPRI/FAULTMASK 等会影响 App 的状态。
- FreeRTOS 已启动后，不能用长时间关中断包住慢速外设轮询。
- LVGL 相关调用必须集中在 GUI 任务语境内，不要从任意任务直接调用 LVGL API。
- USB CDC 口和 CH340 串口要区分：CH340/USART1 用于日志和串口 IAP，MCU USB CDC 是另一个虚拟 COM 口。
- SD IAP 失败时只允许报告错误，不允许写 pending。
- 串口、USB、SD 任一路径升级成功后，都必须走同一个 staging/pending/Boot 安装/confirmed 流程。

建议遵守：

- 修改 Boot/IAP/分区/链接脚本后，必须完整烧录 Bootloader + AppA + AppB 验证。
- 修改 AppB 相关代码后，确认当前实际运行槽位，避免只烧 AppA 但观察到旧 AppB 的误判。
- 修改 SD、USB、FatFs、FreeRTOS 调度相关代码后，至少跑一次对应 IAP 闭环。
- 修改诊断日志时保持关键信息可见：slot、version、size、CRC、pending、boot state、错误码。
- 新增重要行为后同步 `Docs/bringup_log.md`；改变设计或当前状态后同步 `Docs/iap_design.md`。

## 当前已完成

截至 2026-06-27：

- Bootloader + AppA + AppB 三分区构建和烧录通过。
- App tag、slot 镜像打包、CRC32 校验链路完成。
- Boot pending 处理、AppB 安装、trial/confirmed/rollback 状态机完成。
- 串口 IAP 到 AppB 已完成。
- USB CDC IAP 已完成实板闭环。
- SD 卡 IAP 已完成实板闭环。
- W25Q256 参数保存和 IAP staging 已接入。
- LVGL HMI、触摸、页面基础链路已接入。
- 系统诊断日志、Fault 快照、任务栈水位、heap 日志已接入。
- SD `mount fail` 的主因已定位为 SDMMC 轮询读 RX overrun，并完成稳定性修复。

2026-06-27 已验证结果：

```text
Bootloader + AppA + AppB OpenOCD verify OK
当前运行 AppB，AppB tag CRC 与构建产物一致
USB CDC COM4 iap status 正常
USB CDC 传输 324564 字节 app_b_slot.bin 成功
SD 读取 324564 字节 app_b_slot.bin 成功
USB/SD 均能写 pending、复位后 Boot 安装 AppB、AppB confirmed
```

## 当前任务和下一步

本工程当前不是稳定量产项目，而是用户用于深入学习嵌入式软件、提升技术水平、准备技术面试并争取更高薪 offer 的个人综合项目。后续 Codex 线程要优先服务这个目标：让用户能把项目讲清楚、讲深入，并能顶住资深嵌入式面试官对底层、RTOS、通信、Boot/IAP 和可靠性的追问。

优先级最高：

1. 把现有 Bootloader + AppA/AppB + FreeRTOS + LVGL + USB/SD IAP 项目整理成可面试讲述的综合型项目，而不是继续盲目堆功能。
2. 做 IAP 稳定性回归和故障注入，形成可量化记录：USB CDC IAP、SD IAP 多轮成功率、异常日志、失败路径、AppA 是否保持安全、AppB 是否 confirmed。
3. 做断电/复位场景测试：传输中断、写 staging 中断、Boot 擦写 AppB 中断、confirmed 前复位；重点验证失败不破坏 AppA。
4. 整理一份项目答辩材料，讲清分区、启动流程、IAP 协议、RTOS 任务划分、外设链路、失败恢复和验证结果。
5. 补齐面试中暴露的底层短板：时钟树、UART 波特率、CAN 仲裁和终端电阻、FreeRTOS 调度和延时、低功耗设计、启动文件、链接脚本、向量表、Flash/Cache/DMA 一致性。
6. 观察 Boot flash readback retry：2026-06-27 曾出现一次单字节 mismatch 后 retry recovered，最终通过。后续要判断它是偶发读回抖动、供电/时序问题，还是 Flash 操作流程需要增强。

中期任务：

- 单独做一个低功耗最小实验，覆盖 Sleep/Stop、RTC 或按键唤醒、唤醒后恢复时钟和串口；不要直接破坏当前 IAP 主工程稳定状态。
- 多张 SD 卡兼容性测试，覆盖不同容量、不同品牌、不同格式化参数。
- SDMMC 从当前保守轮询方案评估切换到 DMA/中断模式。
- USB CDC CLI 增加复位后自动查询状态，自动确认 AppB confirmed。
- IAP 增加 SHA256 和签名校验。
- 完善断电安全写入策略和升级事务状态。
- 整理 HMI 首页诊断字段的实物肉眼确认结果。

低优先级：

- UI 细节打磨。
- 非关键页面功能扩展。
- NAND Flash 等暂未进入第一版闭环的外设。

## 已知阻塞点和风险

- 当前没有实现加密签名，只有 CRC32；CRC32 只能防传输错误，不能防恶意篡改。
- 断电安全策略还不完整，必须通过更多实板测试验证 Boot 写 AppB 的边界行为。
- SD 卡当前用 1-bit、低速、硬件流控、轮询读写期间暂停任务调度来换稳定性，性能不是目标。
- SD 多卡兼容性还不足，只确认过 4GB FAT32 卡。
- Boot flash readback 曾出现一次 retry recovered，需要继续观察。
- HMI 首页诊断字段串口侧已确认，屏幕肉眼显示仍建议复核。

## 重要技术决策和原因

### AppA 保底，AppB 升级

原因：IAP 最重要的目标是失败不变砖。AppA 不被升级流程覆盖，AppB 作为新固件试运行槽。任何 pending 无效、CRC 错误、写入失败、AppB 启动失败，都能回退 AppA。

### 不使用 Option Byte Bank Swap

原因：第一版优先可控和可调试。手工分区 + Boot 跳转更直观，日志能明确看到选择了 AppA 还是 AppB，也避免 Option Byte 配置错误导致恢复困难。

### 升级包先写 W25Q256 staging

原因：串口、USB、SD 三种入口都统一成“先接收完整包并校验，再由 Boot 安装”的模型。App 运行期不直接擦写内部 AppB，降低运行期中断、RTOS、外设并发对内部 Flash 写入的影响。

### trial/confirmed/rollback 状态机

原因：仅写入 AppB 成功还不代表新 App 可运行。Boot 安装后先进入 trial；AppB 真正启动并完成基础初始化后再写 confirmed。若试启动失败或次数耗尽，Boot 回退 AppA。

### Boot 跳转前清理 CPU 状态

原因：Boot 和 App 是两个独立执行上下文。Boot 可能留下中断屏蔽、SysTick、NVIC pending 或 Fault mask 等状态。实测曾出现 App 启动后 `BASEPRI=0x50`，导致 SysTick 或部分中断被屏蔽，表现为 `HAL_Delay()` 或启动阶段卡住。因此跳转前必须清理内核状态，而不是只设置 MSP 和 VTOR。

### SD 使用保守稳定配置

原因：此前 `SD mount fail` 根因是 `HAL_SD_ReadBlocks()` 轮询读触发 `SDMMC_ERROR_RX_OVERRUN=0x00000020`。当前采用 1-bit、降低 SDCLK、开启硬件流控，并在轮询块读写期间用 `vTaskSuspendAll()` 暂停任务切换，但不关闭中断。这样既减少 FIFO overrun，又避免 HAL 超时依赖的 tick 被关中断破坏。

### 不用长时间 taskENTER_CRITICAL 包外设轮询

原因：`taskENTER_CRITICAL()` 底层会通过 `BASEPRI` 屏蔽一部分中断。外设轮询读写可能持续较久，长时间屏蔽中断会影响 SysTick、USB、SD 超时和系统响应。暂停任务调度比关中断更适合当前 SD 轮询读写场景。

## 验证边界

文档或注释修改：

- 至少检查相关文档是否自洽。
- 不需要强制烧录。

普通 App 逻辑修改：

- `cmake --build --preset gcc-debug`
- 看串口启动日志是否正常。

Boot、链接脚本、分区、IAP record、App tag、W25Q256 staging 修改：

- 必须完整构建。
- 必须完整烧录 Bootloader + AppA + AppB。
- 必须确认 Boot 选择槽位、App tag CRC、pending、boot state。
- 建议至少跑 USB 或 SD 一条完整 IAP 链路。

USB CDC 修改：

- 确认 Windows 枚举出的 MCU USB CDC 端口，不要误用 CH340。
- `iap status` 必须有回包。
- 完整传输 `app_b_slot.bin` 后，复位确认 AppB installed + confirmed。

SD/FatFs/SDMMC 修改：

- 至少用 `iap sd` 跑一次完整文件升级。
- 关注 `HAL_SD_GetError()`、card state、FatFs `FRESULT`。
- 失败路径不得写 pending。

## 工作习惯

- 先读文档，再改代码。
- 先确认当前实际运行槽位，再判断现象。
- 保持改动范围小，避免顺手重构 HAL、LVGL、FreeRTOS 或链接脚本。
- 不要删除历史 bring-up 记录；新增阶段结论追加到文档末尾。
- 遇到实板现象，优先增加可证明问题的日志，再下结论。
- 修改完成后，把验证命令、关键日志、结论写回 `Docs/bringup_log.md` 或相关设计文档。

## 面试准备与下一步方向

### 用户背景和真实目标

用户当前求职方向是嵌入式软件工程师，已有近 3 年储能逆变器 ARM 监控固件开发和维护经验，工作内容包括 BMS/Modbus 协议适配、CAN/RS485 通信、HMI、参数保存、IAP/OTA、量产版本维护、客户联调和现场问题闭环。

用户当前正在投递简历，主要卡点在技术一面和项目深挖。目标很直接：通过本项目深入学习嵌入式底层知识，提高技术面试表现，争取更高薪嵌入式软件 offer。后续回答和工作安排都要围绕这个目标，不要把本工程当成稳定量产项目来无限追求功能完整性。

职业方向建议聚焦：

```text
MCU 产品固件 / RTOS 系统软件 / 可靠升级与通信诊断方向
```

不要把用户包装成只会客户协议适配和维护的人。应把项目和简历表达往以下能力靠：

- FreeRTOS 任务架构、调度、队列/信号量、任务栈和 heap 诊断。
- Bootloader、AppA/AppB 分区、IAP 可靠升级、失败恢复、trial/confirmed/rollback。
- UART、USB CDC、SD/FatFs、CAN/RS485、SPI/QSPI 等外设和通信调试。
- Flash/W25Q256 存储布局、CRC32、参数保存、升级包校验、异常恢复。
- STM32H7 底层：启动流程、链接脚本、向量表、时钟树、Cache、DMA、MPU、低功耗。
- 实板问题定位：串口日志、示波器、协议分析、错误码、Fault 快照、任务水位。

### 面试官已经暴露出的薄弱点

用户近期技术面试中被问到但答得不够稳的问题包括：

- 单片机底层是什么，启动、时钟、外设、寄存器、链接脚本和 HAL 之间是什么关系。
- RTOS 任务为什么要延时，什么时候用 `vTaskDelay()`，什么时候用 `vTaskDelayUntil()`，什么时候应该用队列/信号量而不是 delay。
- 低功耗怎么设计，Sleep/Stop/Standby 区别，唤醒源、时钟恢复、外设恢复怎么处理。
- UART 波特率 9600 怎么设计，外设时钟、过采样、BRR、误差如何计算。
- 项目时钟怎么设计，HSE/PLL/SYSCLK/HCLK/APB/USB 48MHz/LTDC pixel clock/SDMMC clock 分别是多少。
- 应用如何分层，BSP、Driver、Middleware、Service、Task、UI、Boot/Common/App 如何划分边界。
- CAN 终端电阻接在哪里，为什么总线两端各 120 欧，不是每个节点都接。
- 两个 CAN 节点发送相同 ID 会怎样仲裁，为什么工程上必须保证同一报文 ID 只有一个生产者。
- 项目逻辑和业务逻辑不一致时，如何抽象模块边界，避免客户定制把系统架构拖乱。

后续 Codex 应帮助用户把这些问题整理成“可口述、可追问、可结合本项目举例”的答案，而不是只给概念定义。

### 本项目的面试包装方式

推荐把本项目讲成一个综合型项目：

```text
基于 STM32H743 的 LVGL HMI 与本地可靠 IAP 系统：
Bootloader + AppA/AppB 双应用槽，FreeRTOS 多任务，LVGL HMI，
串口/USB CDC/SD 卡三入口升级，W25Q256 staging，CRC32 校验，
trial/confirmed/rollback 回退机制，系统诊断和 Fault 快照。
```

面试讲述主线：

1. 为什么要做 AppA 保底 + AppB 升级槽。
2. 为什么升级包先写 W25Q256 staging，而不是 App 运行期直接擦写内部 Flash。
3. Boot 如何验证 pending、staging CRC、AppB tag、App body CRC。
4. Boot 写 AppB 成功后为什么不直接认为升级成功，而是进入 trial。
5. AppB 如何 confirmed，失败时如何 rollback 到 AppA。
6. 串口、USB CDC、SD 卡为什么复用同一套 staging/pending/Boot 安装流程。
7. FreeRTOS 中 GUI、IAP、日志、存储任务如何分工，LVGL 为什么只能在 GUI task 调用。
8. SD `mount fail` 如何定位到 SDMMC 轮询读 RX overrun，为什么改成 1-bit、低速、硬件流控和暂停任务调度。
9. Boot 跳 App 前为什么必须清理 CPU 状态，尤其是 SysTick、NVIC、VTOR、MSP、BASEPRI。
10. 如何用日志、CRC、错误码、任务水位、Fault 快照证明系统行为。

### 接下来最应该做的事

短期不建议继续堆 UI、NAND、复杂业务页面，也不建议立刻大改 SD DMA/中断或引入签名。用户已经在投简历，最重要的是把项目讲透和补齐底层知识。

建议后续任务按优先级执行：

1. 写一份项目答辩文档，建议放在 `apollo_h743_lvgl_hmi/Docs/interview_project_notes.md`，内容包括项目一句话介绍、架构图文字版、启动流程、IAP 流程、RTOS 任务划分、关键失败路径、验证结果、可被追问的问题。
2. 做 IAP 稳定性回归表，记录 USB CDC 和 SD IAP 多轮测试，每轮包括入口、包大小、CRC、pending、Boot 安装、confirmed、异常日志、是否出现 readback retry。
3. 做故障注入测试：错误 CRC、错误版本、传输中断、SD 缺文件、SD 无效包、复位时机异常，确认不写 pending 或能回退 AppA。
4. 单独建低功耗最小实验或文档化低功耗设计，不要破坏当前主工程。目标是能讲清 Sleep/Stop/Standby、唤醒源、时钟恢复、串口恢复和功耗测试方法。
5. 整理底层 Q&A 文档，覆盖 UART 波特率、CAN 仲裁、CAN 终端电阻、FreeRTOS 延时、时钟树、启动文件、链接脚本、向量表、Flash 擦写、Cache/DMA 一致性。
6. 训练项目口述：3 分钟讲整体架构，10 分钟讲 IAP 可靠性，能被追问到底层寄存器、RTOS 调度和异常恢复。

### 面试回答风格要求

后续 Codex 帮用户准备面试时，回答要偏“工程解释 + 可追问细节”，不要只给教科书定义。每个答案最好包含：

```text
结论 -> 原理 -> 本项目里的例子 -> 常见坑 -> 我如何验证
```

例如：

- 问 UART 9600：要讲 APB 外设时钟、过采样、BRR、误差、时钟源稳定性，以及本工程 USART1 日志口为什么用 115200。
- 问 CAN 仲裁：要讲显性/隐性位、ID 越小优先级越高、相同 ID 无法靠仲裁区分、数据不同会导致错误帧，工程上同一报文 ID 必须单生产者。
- 问 RTOS delay：要讲 delay 是让出 CPU 和定义周期，不是同步手段；周期任务用 `vTaskDelayUntil()`，事件触发用队列/信号量/通知。
- 问低功耗：要讲先定义功耗目标和唤醒源，再裁剪外设、保存上下文、进入 Sleep/Stop/Standby、唤醒后恢复时钟和外设，最后用电流数据验证。
- 问 Boot/IAP：要讲 AppA 不动、AppB 升级、W25Q staging、pending、CRC、trial/confirmed/rollback、失败回退和实板验证日志。

### 时间安排建议

用户每天大约能投入 3-4 小时。建议节奏：

- 1 小时：补 MCU/RTOS/CAN/UART/低功耗底层题。
- 1-1.5 小时：整理项目答辩文档、IAP 回归表和故障注入记录。
- 30 分钟：口述项目并复盘，要求讲得出架构、流程、失败路径和验证证据。
- 30 分钟：整理当天投递/面试被问到的问题，加入 Q&A 文档。

核心判断：当前项目已经足够作为面试综合项目，下一步不是扩大功能面，而是把 Boot/IAP/RTOS/通信/底层知识讲到能顶住资深面试官追问。
