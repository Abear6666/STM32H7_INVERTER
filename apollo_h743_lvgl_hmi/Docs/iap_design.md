# Apollo H743 本地 IAP 第一版设计

日期：2026-05-30

## 设计约束

- 第一版只做 CRC32。
- 不做 Option Byte Bank Swap。
- App 运行地址使用 `AppStart + 0x400`。
- 失败不能破坏旧 AppA。
- 串口日志必须能看出 `Boot -> App` 的顺序。

## 分区

内部 Flash：

```text
0x08000000 - 0x0801FFFF   Bootloader
0x08020000 - 0x080FFFFF   AppA
0x08100000 - 0x081FFFFF   AppB / 后续升级槽
```

AppA 槽：

```text
0x08020000 - 0x080203BF   预留槽头
0x080203C0 - 0x080203FF   app_tag_t
0x08020400 - ...          AppA 向量表和代码
```

AppB 槽：

```text
0x08100000 - 0x081003BF   预留槽头
0x081003C0 - 0x081003FF   app_tag_t
0x08100400 - ...          AppB 向量表和代码
```

外部 W25Q256 IAP 区：

```text
0x01A00000 - 0x01A00FFF   pending 记录
0x01A01000 - 0x01A01FFF   boot state 记录，保存 trial/confirmed/rollback
0x01A02000 - 0x01FFFFFF   staging 数据区
```

## app_tag_t

`app_tag_t` 固定 64 字节，放在槽头最后 64 字节。

CRC32 范围只覆盖 App 本体：

```text
APP_RUN_ADDR .. APP_RUN_ADDR + app_size - 1
```

不包含 `0x400` 槽头，也不包含 `app_tag_t`。

## Boot 流程

```text
1. Boot 启动并初始化 UART。
2. 打印 BOOT 启动日志和分区信息。
3. 检查 W25Q256 pending 记录。
4. 如果 pending 有效，读取 staging 并复算 CRC32。
5. 读取 staging 包内 `app_tag_t`，确认它是 AppB 包，运行地址为 `0x08100400`。
6. 擦除内部 Flash Bank2 的 AppB 槽。
7. 将 staging 包写入 `0x08100000` 起始的 AppB 槽。
8. 读回 AppB 并校验字节一致性。
9. 校验 AppB 的 `app_tag_t` 和 App 本体 CRC32。
10. 清除 pending 标志。
11. 写入 boot state=`trial`，trial slot 为 AppB，最大试启动次数为 3。
12. 按 boot state 选择启动槽位：trial/confirmed 才允许启动 AppB，否则启动 AppA。
13. 如果 AppB 校验失败或 trial 次数耗尽，写入 rollback 状态并回退 AppA。
```

## App 流程

```text
1. AppA 从 `0x08020400` 启动，AppB 从 `0x08100400` 启动。
2. 初始化 UART 后打印启动地址。
3. 复制 Flash 向量表到 RAM_D1 .ram_vector。
4. 设置 SCB->VTOR 到 RAM 向量表。
5. 初始化 SDRAM、LCD、触摸、W25Q256、FreeRTOS、LVGL。
6. 如果当前运行的是 AppB，并且 boot state 是匹配版本的 trial，则写入 confirmed。
7. task_iap 处理本地 IAP 状态和串口/USB CDC 命令。
```

## 本地 IAP 入口

当前 App 侧提供三个本地入口：

```text
串口：iap status / iap recv <size> <crc32> <version> / iap demo
USB CDC：iap status / iap recv <size> <crc32> <version> / iap demo
SD 卡：iap sd，或升级页点击 SD File，读取 0:/app_b_slot.bin
```

`iap demo` 会写入 1024 字节测试数据到 W25Q256 staging 区并写 pending 记录，用于验证 Boot 读取校验逻辑。

串口 / USB CDC 真实包接收流程：

```text
1. PC 发送文本命令：iap recv <size> <crc32> <version>
2. App 清除旧 pending，校验 size/version，擦除 W25Q256 staging 区。
3. App 回复：IAP serial/USB: ready for binary size=<size>
4. PC 发送固定 size 字节二进制包。
5. App 边接收边写入 staging，并计算流式 CRC32。
6. 接收完成后，App 从 W25Q256 读回 staging 并再次计算 CRC32。
7. App 读取包内 `app_tag_t`，要求该包为 AppB 槽镜像：`app_run_addr=0x08100400`，包大小等于 `0x400 + app_size`。
8. App 校验命令版本与 tag version 一致，并校验 App 本体 CRC32。
9. 流式 CRC32、读回 CRC32、AppB tag 校验都通过后，App 写 pending 记录。
10. 复位后 Boot 读取 pending，复算 staging CRC32，并写入内部 AppB。
11. Boot 写入 trial 状态并启动 AppB；AppB 启动后确认 confirmed。
```

PC 发送工具：

```text
dotnet run --project Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release -- --port COM5 --baud 115200 --file build/gcc-debug/app_b_slot.bin --version 2 --chunk 128 --delay-ms 2
```

USB CDC 升级也复用同一个 C# 工具。区别只是 `--port` 选择板子枚举出的 USB CDC 虚拟 COM 口，而不是板载 CH340 的 USART1 COM 口；baud 参数会被 CDC 类请求记录，但实际 USB 传输不依赖该波特率。

当前推荐发送 `build/gcc-debug/app_b_slot.bin`。该文件包含 `0x400` 槽头和 `app_tag_t`，并且 App 本体链接到 `0x08100400`，可以直接由 Boot 写入 AppB 槽。

SD 卡真实包升级流程：

```text
1. 将 build/gcc-debug/app_b_slot.bin 复制到 SD 卡根目录，文件名保持 app_b_slot.bin。
2. 将 SD 卡插入开发板。
3. 在串口输入 iap sd，或在 HMI 升级页点击 SD File。
4. App 在 task_iap 中按需挂载 FatFs，打开 0:/app_b_slot.bin。
5. App 先读取包头中的 app_tag_t，要求该包为 AppB 槽镜像，run 地址为 0x08100400。
6. App 从 SD 卡分块读取文件，写入 W25Q256 staging，并流式计算 CRC32。
7. 文件读完后，App 用计算出的整包 CRC32 再走现有校验流程，校验 staging 读回 CRC32、AppB tag 和 App 本体 CRC32。
8. 全部通过后写 pending；复位后 Boot 安装到内部 Flash AppB，并进入 trial/confirmed 流程。
```

SD 卡第一版只读 FAT 文件，不在 SD 卡上写状态文件；升级状态仍记录在 W25Q256 pending/boot state 区。无卡、挂载失败、文件不存在、文件不是 AppB 包或 CRC 校验失败时，都不会写入有效 pending，也不会影响 AppA。

### USB CDC 硬件前置条件

Apollo V2 H743 的 USB CDC 使用 MCU 的 USB2 OTG FS：

```text
USB 实例：USB2_OTG_FS
数据脚：PA11 = USB_D-, PA12 = USB_D+
接口：USB_SLAVE / USB OTG Type-C
选择口：P9 CAN/USB 必须选择 USB
```

注意不要把板载 `USB_UART` / CH340 的串口 `COM5` 当成 USB CDC 升级口。`USB_UART` 只连接 USART1，用于日志和串口 IAP；USB CDC 升级需要电脑额外枚举出一个新的虚拟 COM 口。

当前实板状态：

```text
AppB 已启动，USB CDC early init OK，IAP: USB CDC init OK。
Windows 目前只看到 COM5，也就是 CH340 / USART1。
串口日志未出现 USB CDC: connected 或 USB CDC: bus reset。
```

这说明固件侧 USB CDC 入口已经接入，但主机还没有连接到 MCU 的 USB Device 控制器。下一次验证前先确认 USB_SLAVE/USB OTG 物理口、P9 CAN/USB 跳帽和 Type-C 数据线。

## 安全边界

当前版本只把真实升级包写入内部 AppB，不覆盖 AppA，不做 Option Byte Bank Swap。因此：

- 串口/USB CDC 传输中断时，不写 pending。
- CRC32 不匹配时，不写 pending。
- W25Q 写入或读回失败时，不写 pending。
- pending 无效时，Boot 按 boot state 决定启动槽位；没有 confirmed/trial 时只启动 AppA。
- pending 有效但不是 AppB 包时，Boot 不擦 AppB，不影响 AppA。
- pending 有效且 AppB 写入失败时，AppA 不被擦除，Boot 回退 AppA。
- AppB 写入并校验通过后，Boot 清除 pending，写入 trial 状态。
- AppB 试运行启动成功后由 App 写入 confirmed；后续复位 Boot 才会稳定选择 AppB。
- AppB 校验失败或试启动次数耗尽时，Boot 写入 rollback 并启动 AppA。

## 下一步

后续继续增强前，需要补齐：

- 断电安全写入策略。
- USB CDC 实传验证，或后续 USB MSC 文件升级入口。
- SHA256 和签名校验。
