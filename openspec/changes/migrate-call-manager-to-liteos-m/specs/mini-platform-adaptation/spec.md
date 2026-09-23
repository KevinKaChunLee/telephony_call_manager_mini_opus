# Spec Delta

## Purpose

定义通话管理在 Mini 上使用的平台能力适配契约，涵盖协议栈、内核适配与事件处理器、身份与权限、音频、日志与观测以及基础类型，确保可用能力返回真实结果、缺失能力显式失败，且平台差异集中在适配层。

## ADDED Requirements

### Requirement: 协议栈适配为本地接口
通话管理 SHALL 通过一个进程内的协议栈适配接口访问 cellular_call lite。该接口 MUST 覆盖范围内的下行能力：拨号、接听、拒接、挂断、保持、解除保持、切换、紧急号码判定、全部挂断，以及上行回调的注册和注销。接口 MUST NOT 暴露 IPC 远端对象、Parcel 或图形 Surface 类型。底层返回的错误码 MUST 原样向上传递，不得转换为成功。

#### Scenario: 底层返回错误
- **WHEN** cellular_call lite 对挂断命令返回某个错误码
- **THEN** 该错误码 SHALL 原样传递给通话管理的调用链

#### Scenario: 实现缺失
- **WHEN** 产品没有注册协议栈适配实现，通话管理发起任意下行命令
- **THEN** 命令 SHALL 返回 `TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL`

### Requirement: 上行回调注册的所有权
通话管理向协议栈适配注册的上行回调对象，SHALL 由通话管理创建和持有。它在注册成功后一直有效，直到注销完成且协议栈适配确认不再调用为止。重复注册 MUST 幂等；注销 MUST 在对象销毁之前完成。

#### Scenario: 退出时注销
- **WHEN** 服务退出
- **THEN** 系统 SHALL 先注销上行回调，然后才释放回调对象；注销之后到达的上报不会访问已释放的对象

### Requirement: 内核适配与事件处理器语义
通话管理 SHALL 只通过一个内核适配层使用互斥锁、信号量、软件定时器、任务和 Tick。内核适配 MUST 提供两套互斥编译的实现：CMSIS-RTOS2 实现随目标产品发布；POSIX 实现只用于 host 自测。事件处理器 SHALL 在内核适配之上提供立即投递、延迟投递、带上限的同步投递和撤销。延迟时间向 Tick 转换时 MUST 向上取整，并处理溢出和“永久等待”特殊值。内核适配和事件处理器 MUST NOT 在 ISR 上下文中使用。业务状态只在主循环中访问，因此业务代码 MUST NOT 为保护业务状态而创建内核对象。

#### Scenario: 延迟精度
- **WHEN** 业务提交一个 1 ms 的延迟任务，而系统 Tick 周期为 10 ms
- **THEN** 该任务 SHALL 不早于 1 ms 执行，也不会因截断为 0 Tick 而立即执行

#### Scenario: 超大延迟值
- **WHEN** 业务提交的延迟值换算后超过 Tick 计数的表示范围
- **THEN** 适配 SHALL 将其限制在最大有限延迟，或返回错误，不发生回绕

#### Scenario: 两套实现互斥
- **WHEN** 同一个构建同时加入 CMSIS 与 POSIX 两套内核适配实现
- **THEN** 构建 SHALL 因符号重复定义而失败，不会静默选中其中一套

#### Scenario: 内核对象创建失败
- **WHEN** 内核对象池已耗尽，事件处理器初始化时无法创建所需的互斥锁、信号量或定时器
- **THEN** 初始化 SHALL 返回失败，已创建的对象被释放，不得以无锁或无定时器的方式继续运行

### Requirement: 身份与权限适配默认拒绝
身份与权限适配 SHALL 为每个权限判定点返回判定结果。判定依据来自产品安全设计给出的信任模型，例如受信组件声明或编译期白名单。产品未配置信任模型时，所有判定 MUST 返回“无权限”。该适配 MUST NOT 以日志桩或恒真实现替代。标准构建的权限实现 MUST 保持不变。

#### Scenario: 默认配置
- **WHEN** Mini 构建未配置信任模型
- **THEN** 任何权限判定 SHALL 返回无权限

#### Scenario: 标准构建不受影响
- **WHEN** 在标准构建中调用任一受权限保护的接口
- **THEN** 权限判定 SHALL 仍由标准侧的 AccessToken 机制完成，行为与迁移前一致

### Requirement: 音频适配的降级语义
通话音频相关能力 SHALL 通过音频适配访问产品音频实现。这些能力包括：通话音频通路随通话状态激活和释放、来电铃声播放和停止、等待音提示。产品提供了实现时，适配 MUST 返回真实结果；产品未提供实现时，适配 MUST 返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED` 并记录一次日志。与标准侧一致，音频操作失败 MUST NOT 阻断通话状态机的推进。音频设备路由选择、彩振、蓝牙/星闪/分布式音频设备 MUST 从 Mini 路径排除。

#### Scenario: 无音频实现时来电
- **WHEN** 产品未提供音频实现，协议栈上报来电
- **THEN** 来电 SHALL 照常创建并上报 INCOMING 状态；铃声请求返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`，且只记录一次日志

#### Scenario: 通话接通
- **WHEN** 产品提供了音频实现，通话进入 ACTIVE
- **THEN** 适配 SHALL 请求激活通话音频通路；通话结束后请求释放

### Requirement: 观测能力与日志
打点（HiSysEvent）、链路跟踪（HiTrace）和 dump 能力 SHALL 从 Mini 产物中排除，Mini 实现 MUST NOT 调用它们。日志 SHALL 通过统一的日志入口输出到产品日志设施或控制台。日志入口 MUST 接受标准侧的 `%{public}`/`%{private}` 格式修饰，并在输出前把它们去掉，不因修饰而产生错误格式化。日志 MUST NOT 包含隐私字段，通话热路径上不输出高频 info 级日志。

#### Scenario: 标准侧格式串
- **WHEN** 原样复用的标准代码以 `%{public}d` 格式输出一条日志
- **THEN** 输出内容 SHALL 与 `%d` 的格式化结果一致，且编译不产生格式告警

#### Scenario: 产物中无打点
- **WHEN** 检查 Mini 产物的符号表
- **THEN** 其中 SHALL 不含 HiSysEvent、HiTrace 和 dump 相关符号

### Requirement: 基础类型适配保持语义
Mini 上为业务代码提供的键值参数容器、引用计数智能指针和延迟单例 SHALL 保持标准侧被使用部分的语义：键值容器缺省值和类型化读写一致；引用计数线程安全，最后一个引用释放时销毁对象；单例首次访问时安全构造，且只构造一次。

#### Scenario: 读取不存在的键
- **WHEN** 业务从参数容器中读取一个不存在的整型键，并指定了缺省值
- **THEN** 返回值 SHALL 为该缺省值，与标准侧一致

#### Scenario: 并发首次访问单例
- **WHEN** 两个任务同时首次访问同一个单例
- **THEN** 该单例 SHALL 只被构造一次，两个任务得到同一实例

### Requirement: 适配实例可替换以便测试
每个适配接口 SHALL 支持在初始化之前注册实现，并支持在测试中替换为测试替身。未注册实现时，行为 MUST 为显式失败。

#### Scenario: 测试替身
- **WHEN** 测试在服务初始化前注册一个协议栈适配测试替身
- **THEN** 通话管理的下行命令 SHALL 到达该替身，替身注入的上报能驱动状态机
