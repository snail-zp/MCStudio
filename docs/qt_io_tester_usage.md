# ACS IO 测试功能说明

## 已实现范围

当前版本先实现了 ACS 控制器的 IO 测试基础功能：

- Qt Widgets 桌面界面
- 连接 ACS Simulator / Ethernet TCP / Ethernet UDP
- 周期性轮询数字输入 `acsc_GetInputPort`
- 周期性轮询数字输出 `acsc_GetOutputPort`
- 点击按钮切换 DO `acsc_SetOutput`
- 配置文件驱动 IO 点表
- 运行日志显示与落盘

## 目录

```text
MCStudio/
├─ CMakeLists.txt
├─ Config/
│  └─ acs_io_config.json
├─ src/
│  ├─ main.cpp
│  ├─ mainwindow.h/.cpp
│  ├─ acscontroller.h/.cpp
│  ├─ ioconfig.h/.cpp
│  └─ filelogger.h/.cpp
└─ docs/
   └─ qt_io_tester_usage.md
```

## 构建方式

由于当前终端没有检测到 `cmake` 和 `qmake` 环境变量，建议直接用 Qt Creator 打开项目：

1. 用 Qt Creator 打开 `CMakeLists.txt`
2. 选择与你 ACS 库匹配的编译器位数
3. 推荐优先使用 `MSVC x64 + Qt6 x64`
4. 构建并运行 `MCStudioIoTester`

## 依赖说明

`CMakeLists.txt` 已经写入 ACS SDK 路径：

- 头文件：`C:/Program Files (x86)/ACS Motion Control/SPiiPlus ADK Suite v3.13.01/ACSC/C_CPP/ACSC.h`
- 导入库：`ACSCL_x64.LIB` 或 `ACSCL_x86.LIB`
- 运行时 DLL：`SPiiPlus Runtime Kit/Redist/x64/ACSCL_x64.dll` 或 `x86/ACSCL_x86.dll`

## 配置文件

IO 点表来自：

`Config/acs_io_config.json`

字段说明：

- `direction`: `input` 或 `output`
- `port`: ACS IO 端口号
- `bit`: 位号
- `name`: UI 显示名称
- `description`: 说明

## 联机建议

第一轮联调建议先做：

1. 先用 `Simulator` 验证界面逻辑
2. 再切到 `Ethernet TCP`
3. 再按你的实际 I/O 模块，把 `acs_io_config.json` 改成现场点表

## 下一步最值得继续补的功能

- 支持 AI/AO
- 增加批量 DO 测试
- 增加输入变化沿触发日志
- 增加测试步骤录制
- 增加 IO 点分组和检索
