# Spec Delta

## Purpose

定义 Mini 侧 `CallManagerClient` 这一兼容调用面的契约：哪些方法保留标准语义、哪些显式失败，以及去掉 IPC 后仍必须保留的参数过滤、鉴权、同步等待上限和回调分发规则。

## ADDED Requirements

### Requirement: 范围内方法保持标准语义
Mini 构建 SHALL 提供与标准侧同名、同参数顺序、同返回类型的 `CallManagerClient`。以下方法 MUST 连接到真实的通话管理实现，并保持标准侧的参数语义、返回语义和既有错误码：
- 生命周期与回调：`Init`、`UnInit`、`RegisterCallBack`、`UnRegisterCallBack`、`ObserverOnCallDetailsChange`
- 通话控制：`DialCall`；`AnswerCall` 的两个重载（带参数版和无参版）；`RejectCall(callId, isSendSms, content)` 和无参 `RejectCall()`；`HangUpCall` 的两个重载（带参数版和无参版）；`HoldCall`、`UnHoldCall`、`SwitchCall`
- 状态查询：`GetCallState`、`HasCall`、`IsRinging`、`IsNewCallAllowed`、`IsEmergencyPhoneNumber`、`HasVoiceCapability`、`EndCall`

#### Scenario: 正常拨号返回
- **WHEN** 服务已就绪、协议栈可用，调用方以合法号码和语音参数调用 `DialCall`
- **THEN** 调用 SHALL 返回 `TELEPHONY_SUCCESS`，随后通过已注册回调收到该通话的状态变化

#### Scenario: 错误码与标准侧一致
- **WHEN** 调用方以空号码调用 `DialCall`
- **THEN** 调用 SHALL 返回 `CALL_ERR_PHONE_NUMBER_EMPTY`，与标准侧一致

### Requirement: 未初始化时返回未就绪
在 `Init` 之前、`UnInit` 之后，或通话管理服务未就绪时，所有返回 `int32_t` 的范围内方法 MUST 返回 `TELEPHONY_ERR_UNINIT`，不产生副作用。

#### Scenario: 未调用 Init
- **WHEN** 调用方未调用 `Init` 就调用 `HangUpCall`
- **THEN** 调用 SHALL 返回 `TELEPHONY_ERR_UNINIT`

### Requirement: 范围外方法显式失败
不在上述范围内、返回 `int32_t` 的公开方法 SHALL 在 Mini 构建中保留入口，并 MUST 返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`，不产生任何副作用。涉及的能力包括会议、DTMF、静音、音频设备选择、视频与摄像头、补充业务、IMS/VoNR 开关、OTT、VoIP、RTT、MMI/USSD、语音信箱、号码格式化、蓝牙与跨端能力。非 `int32_t` 返回类型的范围外方法 MUST 返回能表示“不可用”的值：返回 `sptr` 的方法返回空指针，返回 `bool` 的权限类判定返回 `false`。任何占位入口 MUST NOT 返回成功值。

#### Scenario: 调用会议接口
- **WHEN** 调用方在 Mini 构建中调用 `CombineConference`
- **THEN** 调用 SHALL 返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`，通话状态不变，且不向协议栈发送命令

#### Scenario: 注册蓝牙回调
- **WHEN** 调用方调用 `RegisterBluetoothCallManagerCallbackPtr`
- **THEN** 调用 SHALL 返回空指针

### Requirement: 不支持的可选参数显式失败
带可选行为的范围内方法，如果请求了 Mini 不支持的可选行为，MUST 在产生任何副作用之前返回明确错误：
- `RejectCall` 的 `isSendSms` 为 `true` 时，返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`，来电保持振铃；
- `AnswerCall` 的 `videoState` 不是语音时，返回 `CALL_ERR_VIDEO_NOT_SUPPORTED`，来电保持振铃。

`DialCall` 的视频参数 SHALL 保持标准侧“设备不支持视频时按语音拨出”的语义。

#### Scenario: 拒接并发送短信
- **WHEN** 调用方以 `isSendSms = true` 调用 `RejectCall`
- **THEN** 调用 SHALL 返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`，通话仍处于振铃状态，且不向协议栈发送拒接命令

#### Scenario: 以视频方式接听
- **WHEN** 调用方以视频 `videoState` 调用 `AnswerCall`
- **THEN** 调用 SHALL 返回 `CALL_ERR_VIDEO_NOT_SUPPORTED`，通话仍处于振铃状态

### Requirement: 拨号参数按字段白名单传递
`DialCall` SHALL 保留原 IPC 边界的过滤语义：只有 `accountId`、`videoState`、`dialScene`、`dialType`、`callType`、`isRTT`、`phoneIndex`、`extraParams`、`btSlotIdUnknown` 这些字段从调用方传入业务层，其他键 MUST 被丢弃。`bundleName` MUST 由服务侧的身份来源决定，不采用调用方提供的值。号码长度超过 255 时 MUST 在产生任何副作用之前返回 `CALL_ERR_NUMBER_OUT_OF_RANGE`。

#### Scenario: 调用方注入非白名单字段
- **WHEN** 调用方在 extras 中放入 `token` 或 `isCustomAccessibility`
- **THEN** 业务层 SHALL 看不到这些字段，拨号行为与未放入这些字段时一致

#### Scenario: 调用方伪造 bundleName
- **WHEN** 调用方在 extras 中放入任意 `bundleName`
- **THEN** 业务层使用的 `bundleName` SHALL 来自服务侧身份来源

#### Scenario: 号码超长
- **WHEN** 调用方传入长度为 256 的号码
- **THEN** 调用 SHALL 返回 `CALL_ERR_NUMBER_OUT_OF_RANGE`，不创建通话对象

### Requirement: 鉴权判定点保留且默认拒绝
每个范围内入口 SHALL 在执行业务之前，经过与标准侧相同的权限判定点（包括系统应用判定和各入口的权限组合）。在 Mini 上，判定结果由身份与权限适配给出。产品没有配置信任模型时，判定 MUST 视为拒绝，返回 `TELEPHONY_ERR_PERMISSION_ERR` 或 `TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API`，与标准侧对应入口一致。Mini 适配 MUST NOT 放宽任何入口所要求的权限组合。

#### Scenario: 未配置信任模型
- **WHEN** 产品未配置信任模型，调用方调用 `DialCall`
- **THEN** 调用 SHALL 返回权限错误，不创建通话对象

#### Scenario: 已配置信任模型
- **WHEN** 产品将调用方声明为具备 `PLACE_CALL` 权限的受信组件后，调用方调用 `DialCall`
- **THEN** 鉴权 SHALL 通过，流程进入拨号策略判定

### Requirement: 同步调用有界等待
从非主循环上下文发起的同步调用，等待时间 SHALL 有上限（命名配置，默认 3000 ms）。超时时调用 MUST 返回 `TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL`。超时只表示停止等待，不表示请求被取消；请求上下文和结果存储 MUST 保持有效到任务执行结束，任务 MUST NOT 向已返回的调用方栈内存写入结果。ISR 上下文 MUST NOT 调用同步接口。

#### Scenario: 服务处理超时
- **WHEN** 主循环被占用，超过同步等待上限仍未处理请求
- **THEN** 调用方 SHALL 在上限时间附近返回 `TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL`；请求随后被执行时，不访问调用方已失效的内存

### Requirement: 回调注册的唯一性与生命周期
`RegisterCallBack` SHALL 保持标准侧“同一客户端同时只允许一个回调”的语义，重复注册返回 `TELEPHONY_ERR_REGISTER_CALLBACK_FAIL`。空回调 MUST 被拒绝，注册状态不变。`UnRegisterCallBack` 返回后，服务 MUST NOT 再向该回调发起新的分发；正在进行的分发 MUST 安全完成，回调对象在分发完成前不被销毁。

#### Scenario: 重复注册
- **WHEN** 已注册回调的情况下再次调用 `RegisterCallBack`
- **THEN** 调用 SHALL 返回 `TELEPHONY_ERR_REGISTER_CALLBACK_FAIL`，原回调继续有效

#### Scenario: 注销后无回调
- **WHEN** 调用方注销回调，之后有通话状态变化
- **THEN** 该回调 SHALL NOT 再被调用

### Requirement: 回调分发上下文
对已注册回调的分发 SHALL 在通话状态提交之后进行，并在不持有通话管理内部锁的情况下执行。同一通话的状态回调顺序 MUST 与状态迁移顺序一致。回调中调用范围内的同步方法 MUST 能直接执行，不发生死锁。

#### Scenario: 在回调中接听
- **WHEN** 调用方在 `OnCallDetailsChange` 收到来电后，在回调内部调用 `AnswerCall`
- **THEN** 调用 SHALL 正常返回，不死锁，且后续状态回调按顺序到达
