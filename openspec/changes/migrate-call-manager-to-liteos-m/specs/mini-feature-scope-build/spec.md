# Spec Delta

## Purpose

定义 Mini 迁移的功能分类、裁剪边界和构建分派契约，确保裁剪能力不进入 Mini 运行路径、占位实现可被识别、标准源码不被改动、Mini 实现可在 host 上自测，且安全构建选项不被静默关闭。

## ADDED Requirements

### Requirement: 功能分类有据可查
每个模块和对外调用面 SHALL 在设计文档中归入以下分类之一：兼容保留、能力保留、适配、降级、兼容占位、裁剪、待定。每个条目 MUST 记录设计依据、Mini 侧处理方式、与标准侧的行为差异和验证方式。未分类的源文件 MUST NOT 进入 Mini 构建。

#### Scenario: 新增源文件进入 Mini 构建
- **WHEN** 某个源文件被加入 Mini 源文件表
- **THEN** 该文件或其所属模块 SHALL 在分类表中有对应条目

### Requirement: 裁剪能力不进入 Mini 运行路径
被裁剪能力的源码 SHALL NOT 被链接进 Mini 产物，其初始化项、订阅者、任务、队列和定时器 MUST NOT 在 Mini 上创建。裁剪表示从 Mini 目标排除，MUST NOT 从仓库中物理删除标准实现。

#### Scenario: 产物检查
- **WHEN** 检查 Mini 产物的符号表或链接映射
- **THEN** 其中 SHALL 不含分布式、防诈、骚扰拦截、视频、VoIP、卫星、RTT、通话记录等被裁剪模块的实现符号，也不含 IPC Stub/Proxy 符号

### Requirement: 省略分支对外可识别
Mini 同名实现省略被裁剪能力的分支时，对外可观察的结果 SHALL 与“该能力不存在”一致：查询类方法返回“未启用/不支持”语义的值；动作类方法返回非成功错误码（默认 `CALL_ERR_FUNCTION_NOT_SUPPORTED`，视频类为 `CALL_ERR_VIDEO_NOT_SUPPORTED`）。任何省略分支 MUST NOT 伪造成功。同一符号的标准实现与 Mini 实现 MUST 互斥编译。

#### Scenario: 复用头文件中声明的视频方法
- **WHEN** 在 Mini 上调用通话对象的视频相关方法（该方法由复用的标准头文件声明）
- **THEN** 调用 SHALL 返回 `CALL_ERR_VIDEO_NOT_SUPPORTED`，通话状态不变

#### Scenario: 被省略的能力查询
- **WHEN** Mini 业务流程需要判断超级隐私或卫星模式是否开启
- **THEN** 判定结果 SHALL 为“未开启”，流程按未开启分支继续，且该决策已记录在分类表中

### Requirement: 依赖闭包无隐式失效
保留功能 SHALL NOT 依赖被裁剪功能的初始化、状态、数据、回调或错误恢复才能正确运行。依赖闭包检查发现冲突时，MUST 通过调整分类、提供设计认可的最小替代或记录阻塞项来解决，不得留下运行时的隐式失效链路。

#### Scenario: 来电不经拦截
- **WHEN** 标准侧来电流程会先经过骚扰拦截、防诈和 EDM 过滤，而这些能力在 Mini 上被裁剪
- **THEN** Mini 来电 SHALL 直接进入 INCOMING 状态并上报，该行为差异已记录在分类表中

### Requirement: 标准仓零改动，Mini 实现位于独立仓
本变更 SHALL NOT 修改 telephony_call_manager 仓中的任何文件。Mini 实现 MUST 位于独立仓 `telephony_call_manager_mini`，作为独立的 OHOS 部件（`call_manager_mini`），拥有自己的 `BUILD.gn`、`bundle.json` 和 `callmanager_mini.gni`。标准构建的目标、源文件表、外部依赖、编译宏和编译选项因此保持不变。

#### Scenario: 标准仓工作区
- **WHEN** 本变更全部完成后，在 telephony_call_manager 仓执行 `git status`
- **THEN** 输出 SHALL 为空

### Requirement: 独立仓沿用标准仓的目录布局
Mini 仓 SHALL 沿用标准仓的相对目录布局：Mini 实现放在各模块的 `src/mini/` 与 `include/mini/` 目录，以及 `utils/mini/`、`interfaces/innerkits/mini/`、`test/mini/` 中；原样复用的标准文件放在与标准仓相同的相对路径。Mini 的 include 顺序 MUST 让 Mini 目录先于复用的标准目录，原样复用的标准文件中 MUST NOT 出现 Mini 条件编译。Mini 专用宏为 `TELEPHONY_MINI_SYSTEM`。

#### Scenario: 复用文件中无 Mini 宏
- **WHEN** 在 Mini 仓的全部原样复用文件中搜索 `TELEPHONY_MINI_`
- **THEN** 搜索结果 SHALL 为空

#### Scenario: harness 直接使用独立仓
- **WHEN** 以 `CALL_MANAGER_ROOT` 指向 Mini 仓运行 harness 的 `run_mini_cmsis.sh`
- **THEN** 门禁 SHALL 在不修改 harness、也不依赖 telephony_call_manager 仓的情况下完成编译和自测

### Requirement: Mini 构建可独立构建和链接
Mini 构建 SHALL 生成静态库目标，并在 LiteOS-M 工具链下完成编译和链接，不依赖共享库或动态加载。关闭所有可选能力后，Mini 构建 MUST 仍能构建和链接。新增的 Mini 构建参数 MUST 同时登记到构建参数声明和部件描述中。

#### Scenario: 最小配置构建
- **WHEN** 以关闭全部可选能力的产品配置构建 Mini 目标
- **THEN** 构建 SHALL 成功，且产物不含未定义符号

### Requirement: Mini 自测门禁
Mini 源码（samgr_lite 绑定文件除外） SHALL 能在 host 上以 `-Wall -Wextra -Werror` 零告警编译为自测程序，并通过两种内核适配运行：随组件提供的 `test/mini/mini_selftest.sh`（POSIX 适配），以及 harness 的 `run_mini_cmsis.sh`（CMSIS-RTOS2 适配与 LiteOS-M 内核桩，默认内核对象上限）。两个门禁 MUST 全部用例通过，且连续两次运行结果一致。

#### Scenario: CMSIS 门禁
- **WHEN** 以 LiteOS-M 默认配置（Tick 100 Hz、信号量 6、互斥锁 6、软件定时器 5）运行 `run_mini_cmsis.sh`
- **THEN** 自测程序 SHALL 编译无告警，全部用例通过

#### Scenario: POSIX 门禁
- **WHEN** 运行 `test/mini/mini_selftest.sh`
- **THEN** 自测程序 SHALL 编译无告警，全部用例通过

### Requirement: 复用与外部文件来源可追溯
Mini 仓中从其他仓库原样拷贝的文件（标准仓的 innerkits 头、`call_manager_client.cpp`、`LICENSE` 等，以及上游部件的头文件）SHALL 与来源逐字节一致，并在 `vendor_manifest.json` 中登记来源仓、来源路径和提交。拷贝文件 MUST NOT 在仓内被修改，只能从来源刷新。目标产品另有提供的头文件（例如 core_service 的 `telephony_errors.h`，仅作为头文件使用）在产品构建中直接使用上游；host 自测所需的副本 MUST 只放在 `test/mini/stub/` 中，不得进入 Mini 产物的 include 路径。Mini 目录中与上游同名但内容不同的替身 MUST 在文件头说明它替代的上游头文件。

#### Scenario: 拷贝文件被改动
- **WHEN** 某个拷贝文件与清单登记的来源提交不一致
- **THEN** 来源核对 SHALL 失败，并指出该文件

#### Scenario: 错误码取值一致
- **WHEN** host 自测使用 `test/mini/stub/` 中的错误码头文件
- **THEN** `TELEPHONY_ERR_UNINIT`、`TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL`、`CALL_ERR_FUNCTION_NOT_SUPPORTED` 等错误码的取值 SHALL 与上游头文件逐一相等

### Requirement: 安全构建选项不被静默关闭
Mini 构建 SHALL 保留目标工具链支持的安全与约束选项，包括栈保护、`_FORTIFY_SOURCE`、禁用异常和 RTTI。工具链或产品不支持的选项（例如 CFI、PAC）MUST 在设计文档中记录差异和风险，MUST NOT 被静默移除。

#### Scenario: 不支持的选项
- **WHEN** 目标工具链不支持某个标准侧安全选项
- **THEN** 设计文档中 SHALL 有该选项的差异记录，Mini 构建脚本中也有对应说明
