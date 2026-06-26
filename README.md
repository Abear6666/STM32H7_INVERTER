# STM32H7_INVERTER

这是一个内部维护的 STM32H743 + LVGL HMI / IAP 工程仓库。

## 仓库内容

- `apollo_h743_lvgl_hmi/`: 主工程
- `apollo_h743_lvgl_hmi/Boot/`: Bootloader
- `apollo_h743_lvgl_hmi/App/`: AppA / AppB
- `apollo_h743_lvgl_hmi/Common/`: 公共代码
- `apollo_h743_lvgl_hmi/Middlewares/`: FreeRTOS / LVGL / FATFS / USB
- `apollo_h743_lvgl_hmi/Tools/`: 串口 / USB / SD 升级工具与打包脚本
- `apollo_h743_lvgl_hmi/Docs/`: 引导记录、内存规划、IAP 设计文档
- `apollo_h743_lvgl_hmi/build/reference/phase7_9/`: 少量手工保留的参考源码片段，仅用于对照

## 提交范围

建议上传：

- `apollo_h743_lvgl_hmi/App/**`
- `apollo_h743_lvgl_hmi/Boot/**`
- `apollo_h743_lvgl_hmi/Common/**`
- `apollo_h743_lvgl_hmi/Middlewares/**`
- `apollo_h743_lvgl_hmi/Tools/**`
- `apollo_h743_lvgl_hmi/Docs/**`
- `apollo_h743_lvgl_hmi/build/reference/phase7_9/**`
- 根目录 `README.md` 和 `.gitignore`

建议不上传：

- `apollo_h743_lvgl_hmi/build/gcc-debug/**`
- `apollo_h743_lvgl_hmi/build/downloads/**`
- `apollo_h743_lvgl_hmi/build/official_hex/**`
- `apollo_h743_lvgl_hmi/Tools/**/bin/**`
- `apollo_h743_lvgl_hmi/Tools/**/obj/**`
- `apollo_h743_lvgl_hmi/Tools/**/publish/**`
- 根目录 `【正点原子】手把手教你学STM32 HAL库（全系列）资料A盘/`
- `sscomV5131.exe`

## 构建环境

- CMake 3.22+
- Ninja
- ARM GNU Toolchain (`arm-none-eabi-gcc`)
- Python 3
- .NET 10 SDK

## 构建命令

```powershell
cd apollo_h743_lvgl_hmi
cmake --preset gcc-debug
cmake --build --preset gcc-debug
```

## 当前工程目标

- Bootloader + AppA + AppB 分区
- 通过 `app_tag_t` 管理 App 镜像信息
- 通过 `Tools/pack_app/pack_app.py` 生成 slot 镜像
- 用 `Docs/memory_map.md` 记录 Flash / RAM 规划
- 用 `Docs/iap_design.md` 记录本地升级流程

## 工具项目

```powershell
dotnet build Tools/iap_serial_cli/ApolloIapSerialCli.csproj -c Release
dotnet build Tools/iap_serial_gui/ApolloIapSerialGui.csproj -c Release
```
