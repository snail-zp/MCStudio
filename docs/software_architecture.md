# 运动控制调试与测试平台软件架构设计

## 1. 目标定位

本软件面向运动控制工程师，用于统一完成以下工作：

- 控制器 IO 调试
- 底层通信接口测试
- 硬件工位标定
- 运动轴性能测试
- 多型号控制器适配
- 测试与标定过程留痕
- 配置驱动的软件部署

建议将系统定位为：

**“面向多控制器的工业调试与测试平台”**

核心设计原则：

- 控制器解耦：上层业务不依赖具体控制器品牌或协议
- 配置驱动：IO、轴、工位、测试流程尽量通过配置定义
- 任务化执行：IO测试、标定、性能测试都抽象为任务
- 全链路日志：操作日志、通信日志、测试日志、异常日志独立管理
- 可扩展：后续新增控制器、新增测试项时尽量不改核心框架

## 2. 总体架构

建议采用分层 + 插件式架构：

```text
+--------------------------------------------------------------+
|                          表现层 UI                           |
|  主界面 / IO面板 / 轴调试 / 标定向导 / 测试页面 / 日志查看   |
+--------------------------------------------------------------+
|                        应用服务层                            |
|  设备管理 / IO服务 / 轴服务 / 标定服务 / 测试服务 / 报告服务 |
+--------------------------------------------------------------+
|                        领域与任务层                          |
|  Controller抽象 / Axis模型 / IO点模型 / Station模型         |
|  TestCase / CalibrationTask / TaskScheduler / RuleEngine     |
+--------------------------------------------------------------+
|                      适配器与驱动层                          |
|  ControllerAdapter插件 / 通信协议封装 / DAQ / 串口 / TCP等   |
+--------------------------------------------------------------+
|                      基础设施层                              |
|  日志系统 / 配置系统 / SQLite / 文件存储 / 权限 / 导出       |
+--------------------------------------------------------------+
```

## 3. 模块划分

### 3.1 UI 层

建议按工作场景拆分，而不是按技术功能拆分：

- 设备连接页面
- IO 调试页面
- 底层接口测试页面
- 工位标定页面
- 运动轴测试页面
- 日志中心
- 配置管理页面
- 测试报告页面

建议 UI 具备以下特点：

- 左侧设备树：控制器、轴、IO模块、工位
- 中间工作区：按功能切换不同调试页
- 底部日志窗：实时显示当前操作和通信信息
- 右侧详情区：显示选中对象属性、状态、诊断信息

### 3.2 应用服务层

这一层负责把 UI 操作转换为可执行业务：

- `DeviceManager`
  - 管理控制器实例
  - 维护连接状态
  - 切换当前活动控制器
- `IOService`
  - 读取/写入 DI/DO/AI/AO
  - 批量刷新 IO 状态
  - 执行 IO 点测试流程
- `MotionService`
  - 轴使能、回零、点动、绝对/相对运动
  - 采集位置、速度、电流、跟随误差等数据
- `CalibrationService`
  - 管理工位标定流程
  - 保存标定结果
  - 提供复测机制
- `TestService`
  - 执行接口测试、轴性能测试、压力测试
  - 统一输出测试结果
- `ReportService`
  - 生成测试记录、标定报告、追溯文档

### 3.3 领域与任务层

这是整个软件最关键的部分，建议重点抽象。

核心领域对象：

- `Controller`
  - 控制器实例
  - 包含型号、连接参数、能力集、状态
- `Axis`
  - 轴号、名称、单位、限位、回零参数、采样参数
- `IOPoint`
  - 点位名称、地址、类型、方向、量程、单位、所属模块
- `Station`
  - 工位定义、关联轴、关联传感器、标定参数
- `TestCase`
  - 测试项定义、步骤、阈值、判定规则
- `CalibrationTask`
  - 标定步骤、采样方式、拟合算法、结果模型

建议把“调试动作”统一抽象为任务：

- `ConnectTask`
- `IOPollTask`
- `IOTestTask`
- `AxisJogTask`
- `AxisPerformanceTestTask`
- `StationCalibrationTask`
- `InterfaceLoopbackTestTask`

再由 `TaskScheduler` 统一调度，带来几个好处：

- 页面之间共享一套执行模型
- 支持任务取消、超时、重试
- 便于记录完整过程日志
- 后续容易扩展自动化测试

## 4. 控制器插件架构

由于你要支持多种控制器，这部分建议做成插件式。

### 4.1 抽象接口

上层不要直接调用某品牌 SDK，而是依赖统一接口：

```csharp
public interface IControllerAdapter
{
    string ControllerType { get; }
    Task ConnectAsync(ConnectionConfig config);
    Task DisconnectAsync();
    Task<bool> IsConnectedAsync();

    Task<IReadOnlyList<IOPointValue>> ReadIOAsync(IEnumerable<string> ioNames);
    Task WriteIOAsync(string ioName, object value);

    Task AxisEnableAsync(int axisId);
    Task AxisHomeAsync(int axisId);
    Task AxisMoveAbsoluteAsync(int axisId, double position, double velocity);
    Task AxisMoveRelativeAsync(int axisId, double distance, double velocity);
    Task AxisStopAsync(int axisId);
    Task<AxisRuntimeState> GetAxisStateAsync(int axisId);

    Task<IDictionary<string, object>> ReadRegistersAsync(IEnumerable<string> addresses);
    Task WriteRegisterAsync(string address, object value);
}
```

### 4.2 插件分层建议

- `Adapter`
  - 提供统一业务接口
- `ProtocolClient`
  - 封装串口、TCP、CAN、EtherCAT 等通信
- `VendorSdkWrapper`
  - 封装厂家 SDK
- `CapabilityDescriptor`
  - 描述该控制器支持哪些功能

例如某些控制器可能没有 AO、没有插补、没有高速采样，能力差异不要写死在 UI 里，而应通过能力描述返回：

```json
{
  "controller_type": "ControllerA",
  "capabilities": {
    "digital_io": true,
    "analog_io": true,
    "multi_axis_motion": true,
    "station_calibration": true,
    "high_speed_sampling": false
  }
}
```

这样页面可以按能力自动显隐功能。

## 5. 配置系统设计

你已经有 `io_config.json`，建议继续扩展成完整配置体系。

### 5.1 配置分类

建议至少拆成以下几类：

- `app_config.json`
  - 软件全局配置
- `controllers.json`
  - 控制器类型与连接参数
- `io_config.json`
  - IO 点位定义
- `axis_config.json`
  - 轴参数、限位、单位、运动默认参数
- `station_config.json`
  - 工位、治具、传感器、标定参数
- `test_config.json`
  - 性能测试、判定阈值、采样周期
- `log_config.json`
  - 日志级别、落盘策略、保留周期

### 5.2 配置加载机制

建议实现一个统一配置中心：

- 启动时加载所有配置
- 提供配置合法性校验
- 支持热重载非关键配置
- 配置变更写入审计日志
- 支持“默认配置 + 项目配置 + 现场覆盖配置”

推荐优先级：

```text
默认配置 < 项目配置 < 现场配置 < 临时运行参数
```

### 5.3 IO 配置建议字段

你当前的 `io_config.json` 已经有基础字段，但建议补充：

- `address`
- `channel`
- `direction`
- `default_value`
- `polling_enabled`
- `alarm_rule`
- `debounce_ms`
- `scaling`
- `controller_type`
- `station`

示例：

```json
{
  "name": "Chuck1 Vacuum DI",
  "variable_name": "chuck1_vacuum_di",
  "controller_type": "MotionCtrlA",
  "module": "chuck1",
  "station": "station_1",
  "io_type": "DI",
  "address": "X0.0",
  "channel": 0,
  "direction": "input",
  "polling_enabled": true,
  "debounce_ms": 20,
  "description": "Chuck1 vacuum sensor input"
}
```

另外，你现在这个 JSON 里中文出现了乱码，说明文件编码可能存在问题，建议统一使用 `UTF-8`。

## 6. 日志系统设计

你特别强调了详细日志，这里建议单独设计，不要只用简单文本输出。

### 6.1 日志分类

建议至少区分 5 类日志：

- 操作日志
  - 谁在什么时候做了什么操作
- 通信日志
  - 发送/接收的报文、寄存器访问、错误码
- 业务日志
  - 测试开始、标定完成、任务失败、参数变更
- 异常日志
  - 异常堆栈、超时、断连、非法参数
- 数据日志
  - 轴位置、速度、电流、采样曲线

### 6.2 日志字段

每条日志建议包含：

- 时间戳
- 日志级别
- 模块名
- 控制器编号
- 任务 ID
- 操作人
- 工位号
- 消息内容
- 原始数据
- 异常堆栈

示例：

```json
{
  "timestamp": "2026-05-25T10:35:21.123+08:00",
  "level": "INFO",
  "module": "MotionService",
  "controller_id": "CTRL_01",
  "task_id": "TASK_AXIS_TEST_0001",
  "operator": "zhangsan",
  "station": "station_1",
  "message": "Axis 2 move absolute start",
  "payload": {
    "target_position": 100.0,
    "velocity": 50.0
  }
}
```

### 6.3 日志落地策略

建议采用“双通道”：

- 文本日志文件
  - 便于快速查看和现场排障
- SQLite 数据库存储
  - 便于检索、筛选、生成报告

推荐策略：

- 普通日志按天滚动
- 通信原始报文单独文件
- 高速采样数据单独 CSV 或二进制文件
- 测试结果、标定结果写入数据库

## 7. 测试与标定执行框架

不要把“测试流程”散落在 UI 按钮事件中，建议统一做成流程引擎。

### 7.1 测试流程模型

一个测试项通常包含：

- 前置条件检查
- 设备初始化
- 测试步骤执行
- 数据采集
- 结果判定
- 报告输出

可抽象为：

```csharp
public interface ITestWorkflow
{
    string Name { get; }
    Task<TestResult> ExecuteAsync(TestContext context, CancellationToken ct);
}
```

### 7.2 典型测试项

- IO 通断测试
- IO 响应延迟测试
- 通信读写稳定性测试
- 轴重复定位精度测试
- 轴最大速度测试
- 加减速平滑性测试
- 跟随误差测试
- 长时间循环疲劳测试

### 7.3 标定流程建议

标定流程建议采用“向导式步骤”：

1. 选择工位
2. 选择控制器与轴
3. 检查传感器状态
4. 执行自动/半自动标定
5. 保存结果
6. 复测确认
7. 生成标定记录

标定结果建议版本化保存，支持：

- 当前生效版本
- 历史版本追溯
- 回滚到上一版本

## 8. 数据存储设计

建议使用：

- `SQLite`
  - 适合单机工程软件
  - 部署简单
  - 足够支撑日志索引、测试结果、标定记录

建议表结构：

- `device_connection_history`
- `operation_logs`
- `communication_logs`
- `test_records`
- `test_steps`
- `calibration_records`
- `axis_sample_files`
- `config_change_history`

高速采样数据不建议整段直接写数据库，可采用：

- 数据库存元数据
- 文件存实际采样数据

这样查询和性能都会更好。

## 9. 推荐目录结构

如果采用 C#/.NET WPF 或 WinUI，可以参考：

```text
MCStudio/
├─ src/
│  ├─ MCStudio.UI/
│  ├─ MCStudio.Application/
│  ├─ MCStudio.Domain/
│  ├─ MCStudio.Infrastructure/
│  ├─ MCStudio.Plugins.Abstractions/
│  ├─ MCStudio.Plugins.ControllerA/
│  ├─ MCStudio.Plugins.ControllerB/
│  └─ MCStudio.Tests/
├─ Config/
│  ├─ app_config.json
│  ├─ controllers.json
│  ├─ io_config.json
│  ├─ axis_config.json
│  ├─ station_config.json
│  ├─ test_config.json
│  └─ log_config.json
├─ Logs/
├─ Data/
├─ Reports/
└─ docs/
```

## 10. 技术选型建议

如果你的目标是 Windows 工程软件，优先推荐：

- UI：`WPF` 或 `WinUI 3`
- 语言：`C#`
- 架构：`MVVM + 分层架构 + 插件机制`
- 日志：`Serilog`
- 本地数据库：`SQLite`
- 配置：`JSON + Options/自定义配置中心`
- 图表：`LiveCharts2` 或 `ScottPlot`

推荐 C# 的原因：

- 工业软件生态较成熟
- 串口、TCP、文件、数据库支持完善
- 对接厂家 DLL/SDK 更方便
- 做桌面 UI 和日志系统效率高

如果你已经有 C++ 控制库，也可以采用：

- 核心驱动与实时部分：`C++`
- 上位机业务与 UI：`C#`
- 通过 `P/Invoke` 或 `C++/CLI` 做桥接

## 11. 非功能性要求

这类软件除了功能，还建议提前定义这些指标：

- 稳定性
  - 单次运行 8 小时以上不崩溃
- 可追溯性
  - 所有测试和标定可查历史记录
- 可恢复性
  - 断线后可重连
- 安全性
  - 危险运动前二次确认
- 可维护性
  - 新增控制器时只新增插件，不改核心业务
- 性能
  - 常规 IO 刷新周期 100 ms 内
  - 高速采样独立线程或独立服务

## 12. 开发阶段建议

建议分 4 个阶段推进，降低风险。

### 阶段 1：最小可用版本

- 控制器连接管理
- 基础 IO 读写
- 基础轴操作
- 基础日志系统
- JSON 配置加载

### 阶段 2：测试平台化

- IO 测试任务
- 接口测试任务
- 轴性能测试任务
- 测试结果保存与查询

### 阶段 3：标定与报告

- 工位标定流程
- 标定版本管理
- 报告导出

### 阶段 4：插件化与工程化

- 多控制器插件
- 权限管理
- 配置热更新
- 自动化回归测试

## 13. 对你当前项目的直接建议

结合你当前目录，建议下一步先做这 5 件事：

1. 定义统一控制器接口 `IControllerAdapter`
2. 重新整理 `Config/` 下的配置分类
3. 修复 `io_config.json` 编码问题并补充地址/方向等字段
4. 搭建统一日志中心
5. 先实现一个控制器插件作为样板

## 14. 推荐的第一版核心类

第一版先把下面这些类立起来，项目会很稳：

- `ControllerRegistry`
- `IControllerAdapter`
- `DeviceManager`
- `IOService`
- `MotionService`
- `CalibrationService`
- `TestService`
- `TaskScheduler`
- `ConfigManager`
- `LogManager`
- `TestResultRepository`

## 15. 一句话总结

这套软件最合适的方向，不是做成“一个大而杂的调试界面”，而是做成：

**“以插件适配多控制器、以任务驱动调试/测试/标定、以配置和日志支撑现场交付”的工业工程平台。**
