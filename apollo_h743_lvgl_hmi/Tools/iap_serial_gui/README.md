# Apollo H743 串口 IAP 上位机

这是 `iap_send_serial.py` 的 C# WinForms 图形界面版本，协议命令仍为 `iap recv <size> <crc32> <version>`。

## 当前能力

- 枚举串口，默认波特率 115200。
- 打开串口时设置 `DTR=false`、`RTS=false`。
- 选择 `.bin` 文件并自动计算 CRC32。
- 发送命令：

```text
iap recv <size> <crc32> <version>
```

- 等待板端 `ready for binary` 后分块发送二进制。
- 显示发送进度和串口日志。
- 支持发送 `iap status`、清空日志、保存日志。

## 使用方式

构建：

```text
dotnet build Tools/iap_serial_gui/ApolloIapSerialGui.csproj -c Release
```

发布：

```text
dotnet publish Tools/iap_serial_gui/ApolloIapSerialGui.csproj -c Release -r win-x64 --self-contained false -o Tools/iap_serial_gui/publish
```

运行：

```text
Tools/iap_serial_gui/publish/ApolloIapSerialGui.exe
```

推荐选择固件：

```text
build/gcc-debug/app_b_slot.bin
```

版本号填写 `2`，必须与 `app_b_slot.bin` 包内 `app_tag_t.version` 一致。

## 命令行验证工具

为了便于自动验证 C# 传输链路，另有一个控制台版本，复用 GUI 的同一套串口发送代码：

```text
dotnet run --project Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release -- --port COM5 --baud 115200 --file build/gcc-debug/app_b_slot.bin --version 2 --chunk 128 --delay-ms 2
```

当前 App 侧会拒绝错误槽位的包，例如误选 `app_a_slot.bin` 时，包内 `app_tag_t.app_run_addr` 为 `0x08020400`，不是 AppB 运行地址 `0x08100400`，不会写 pending。

预期 App 侧日志：

```text
IAP serial: staged tag magic=0x41505447 run=0x08100400 app_size=... version=2 app_crc=...
IAP serial: package verified, pending flag set version=2 size=... crc=...
```

## 安全边界

当前板端实现把升级包写入 W25Q256 staging 并写 pending。复位后 Boot 只擦写内部 Flash Bank2 的 AppB 槽，不覆盖 AppA，不做 Option Byte Bank Swap。

所以界面中的“发送完成”表示升级包已进入 staging；真正安装 AppB 需要复位进入 Boot，并通过日志确认 `AppB update applied and verified, trial pending`、`trial AppB attempt 1/3 selected`、`jump to App at 0x08100400`。AppB 启动成功后会打印 `IAP confirm: AppB version=2 confirmed`，后续复位 Boot 会按 confirmed 状态直接选择 AppB；如果 AppB 校验失败或试启动次数耗尽，Boot 会写 rollback 并启动 AppA。
