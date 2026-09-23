# Spec Delta

## Purpose

定义 Mini 侧最小语音集的下行控制契约，覆盖拨号、接听、拒接、挂断、保持、解除保持、切换和紧急呼叫，保证其语义与标准侧一致，且策略失败时不留下残留状态。

## ADDED Requirements

### Requirement: 语音通话类型范围
Mini 构建 SHALL 支持 CS 和 IMS 语音通话的完整生命周期。其他通话类型（视频、OTT、VoIP、卫星、蓝牙侧发起）的拨号请求 MUST 返回明确的非成功错误码，且不创建通话对象；协议栈上报的这类来电 MUST 按“来电处理”要求的方式处理。

#### Scenario: 请求卫星通话
- **WHEN** 调用方在 extras 中指定卫星通话类型并调用 `DialCall`
- **THEN** 调用 SHALL 返回非成功错误码，不创建通话对象，也不向协议栈发送命令

### Requirement: 拨号流程顺序与策略收口
拨号 SHALL 依次执行：号码合法性检查、去除分隔符、紧急号码判定、拨号策略判定，然后才能产生任何副作用。在策略判定通过之前，系统 MUST NOT 创建通话对象、修改标志位、申请音频资源或发出状态广播。

#### Scenario: 空号码
- **WHEN** 调用方以空号码拨号
- **THEN** 系统 SHALL 返回 `CALL_ERR_PHONE_NUMBER_EMPTY`，不创建通话对象

#### Scenario: 策略拒绝
- **WHEN** 当前通话数已达到策略上限，调用方发起非紧急拨号
- **THEN** 系统 SHALL 返回 `CALL_ERR_CALL_COUNTS_EXCEED_LIMIT`，对象表、标志位和音频状态都不变化

### Requirement: 紧急呼叫旁路
紧急号码的拨号 SHALL 沿用标准侧的紧急呼叫策略，绕过多路限制、EDM 管控、隐私模式等常规校验。Mini 实现中省略或降级的能力 MUST NOT 阻断紧急呼叫。Mini 新增的任何校验 MUST 显式声明紧急呼叫是否豁免。

#### Scenario: 裁剪能力被省略时拨打紧急号码
- **WHEN** 超级隐私、卫星、EDM 等检查在 Mini 上被省略，调用方拨打紧急号码
- **THEN** 系统 SHALL 向协议栈下发紧急拨号命令，且拨号场景被标记为紧急呼叫

#### Scenario: 通话数已达上限时拨打紧急号码
- **WHEN** 普通通话已占满策略上限，调用方拨打紧急号码
- **THEN** 系统 SHALL 按标准侧紧急策略处理，不因普通通话上限而拒绝

### Requirement: 单一待消费拨号请求
同一时刻 SHALL 只存在一个“已提交但未被消费”的拨号请求。已有拨号请求在处理中时，新的拨号请求 MUST 返回 `CALL_ERR_CALL_COUNTS_EXCEED_LIMIT`，且 MUST NOT 覆盖正在处理的拨号参数。

#### Scenario: 连续两次拨号
- **WHEN** 第一次拨号仍在处理中，调用方再次发起拨号
- **THEN** 第二次调用 SHALL 返回 `CALL_ERR_CALL_COUNTS_EXCEED_LIMIT`，第一次拨号使用的号码和参数不变

### Requirement: 拨号失败不留残留
协议栈拒绝拨号命令时，`DialCall` SHALL 返回非成功错误码，返回值语义与标准侧的拨号失败处理一致。已创建的拨号中通话 MUST 进入断开状态并从对象表移除；拨号处理标志 MUST 被清除，使后续拨号可以正常进行。整个失败处理 MUST 在有界时间内完成，不依赖等待同一主循环中尚未执行的任务。

#### Scenario: 协议栈拒绝拨号
- **WHEN** 协议栈适配对拨号命令返回失败
- **THEN** `DialCall` SHALL 返回非成功错误码；调用方收到该通话的断开状态回调；之后 `HasCall` 返回 `false`，新的拨号请求可以被受理

#### Scenario: 失败处理不自等待
- **WHEN** 拨号请求在主循环中执行，协议栈同步返回失败
- **THEN** 失败处理 SHALL 在不等待主循环后续任务的情况下完成，不出现固定时长的超时等待，也不留下处于拨号中状态的通话对象

### Requirement: 通话控制动作的策略与返回语义
接听、拒接、挂断、保持、解除保持、切换 SHALL 先经过各自的策略判定，再向协议栈下发命令。策略判定失败时返回相应错误码，且不向协议栈发送命令。命令被受理后返回 `TELEPHONY_SUCCESS`，实际结果通过状态回调体现。Mini 实现 MUST 保持标准侧各动作可观察的状态序列，包括接听时的 `ANSWERED` 中间态。

#### Scenario: 挂断不存在的通话
- **WHEN** 调用方对不存在的 callId 调用 `HangUpCall`
- **THEN** 系统 SHALL 返回非成功错误码，且不向协议栈发送挂断命令

#### Scenario: 接听来电
- **WHEN** 存在振铃中的来电，调用方以语音方式调用 `AnswerCall`
- **THEN** 调用方 SHALL 先收到 `ANSWERED` 状态回调；协议栈确认后，再收到 `ACTIVE` 状态回调

#### Scenario: 保持后切换
- **WHEN** 存在一路通话中和一路保持中的通话，调用方对保持中的通话调用 `SwitchCall`
- **THEN** 系统 SHALL 向协议栈下发切换命令，状态回调显示两路通话的保持/通话状态互换

### Requirement: 下行命令唯一出口
所有对协议栈的下行命令 SHALL 经由协议栈适配这一唯一出口发出。协议栈适配不可用时，下行命令 MUST 返回 `TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL`，失败处理与标准侧一致。

#### Scenario: 协议栈适配未注册
- **WHEN** 协议栈适配尚未注册实现，调用方调用 `HoldCall`
- **THEN** 策略判定通过后，下发 SHALL 返回 `TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL`，通话状态不变

### Requirement: 通话对象表容量
通话对象表 SHALL 有明确容量（命名配置，默认 5），其中 1 个槽位 MUST 保留给紧急呼叫。普通拨号在非保留槽位已满时，MUST 返回 `CALL_ERR_CALL_COUNTS_EXCEED_LIMIT`。

#### Scenario: 普通槽位已满
- **WHEN** 对象表中已有 4 个通话对象，调用方发起非紧急拨号
- **THEN** 系统 SHALL 返回 `CALL_ERR_CALL_COUNTS_EXCEED_LIMIT`

#### Scenario: 普通槽位已满时拨打紧急号码
- **WHEN** 对象表中已有 4 个通话对象，调用方拨打紧急号码
- **THEN** 对象表容量 SHALL NOT 成为拒绝该紧急拨号的原因，该拨号使用保留槽位，并继续按紧急呼叫策略处理
