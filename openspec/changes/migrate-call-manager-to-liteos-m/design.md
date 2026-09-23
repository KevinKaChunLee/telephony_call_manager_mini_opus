# Design

## Context

迁移动机见 `proposal.md` 的 Why 一节，行为契约见 `specs/` 下的 6 份规格。本节只记录决定方案走向的现状与约束。除特别注明外，数字都来自当前仓内或上游源码的实测。

**仓库形态（用户决定）**

- **独立仓**：Mini 实现位于独立仓 `telephony_call_manager_mini`（本地 git 仓，暂不推送远端），是独立的 OHOS 部件 `call_manager_mini`；telephony_call_manager 仓零改动。
- **原样拷贝**：原样复用的标准文件逐字节拷贝到新仓的同名路径，来源登记在 `vendor_manifest.json` 中（标准仓为上游 `gitcode.com/openharmony/telephony_call_manager` 的 `24e1785f`），由 `test/mini/tools/vendor_files.py` 负责核对和刷新。新仓沿用标准仓的相对布局，harness 只需把 `CALL_MANAGER_ROOT` 指向新仓。
- **事实已复核**：下文依赖的标准实现事实已在 `24e1785f` 上重新核对过，包括鉴权组合、`OnDialCall` 白名单与长度上限，以及 `call_manager_client.cpp` 调用的 proxy 方法集合。

**标准实现现状（已在代码中确认）**

- **运行形态与初始化**：`CallManagerService` 同时继承 `SystemAbility` 和 `CallManagerServiceStub`。`Init()` 依次初始化 `CallControlManager` → `ReportCallInfoHandler` → `CellularCallConnection` → `CallRecordsManager` → `BluetoothConnection` → `DistributedCallManager`，然后订阅 audio/devicemgr 两个 SA。
- **IPC 链**：`CallManagerClient` → `CallManagerProxy`（负责 samgr 查询、DeathRecipient、重连和回调注册）→ `CallManagerServiceProxy` → IPC → `CallManagerServiceStub` → `CallManagerService`（鉴权后转发）→ `CallControlManager`。
- **Stub 承担的非传输职责**：`OnDialCall` 只把 10 个白名单字段重建为新的 `PacMap`，并在调用业务前校验号码长度不超过 `ACCOUNT_NUMBER_MAX_LENGTH`（255）。批量接口限制条目数不超过 `MAX_CALLS_NUM`（5）。
- **并发模型**：
  - 下行动作（`CallRequestHandler`）经 `ffrt::submit` 在并发池中执行，只有 `DialRequest` 在调用方线程同步执行；上行统一进入串行队列 `report_call_info_queue`。
  - 拨号失败时，`CallRequestProcess::HandleDialFail()` 会在 `cv_` 上最多等待 1 秒，等上行队列中的 `DialingHandle` 把通话对象加入表中。
- **保留路径与裁剪模块强耦合**：以 `call_status_manager.cpp` 为例，引用 `AntiFraudService` 21 处、`SettingsDataShareHelper` 17 处、`MotionRecognition` 12 处；`call_control_manager.cpp` 引用 `VoIPCall` 22 处。保留文件直接 include 的外部头文件约 30 个（DataShare、Notification、OsAccount、Want 等）。
- **`call_manager_client.cpp` 的依赖**：只依赖 `call_manager_proxy.h`、`parameter.h`、`telephony_errors.h`，以及头文件间接引入的 `singleton.h`、`pac_map.h`、`call_manager_callback.h`、`i_call_status_callback.h`、`transfer_control.h`。其中后两个 include 了 `iremote_broker.h`。
- **innerkits 类型头基本自成一体**：`call_manager_base/info/inner_type/disconnected_details.h` 只依赖 std、`securec.h` 和 `telephony_errors.h`，`CellularCallInfo` 定义在 `call_manager_info.h`。但 `call_manager_base.h` 本身没有 include `<cstdint>`/`<chrono>`，`call_manager_callback.h` 假设 `pac_map.h` 已经被包含，`call_object_manager.h` 缺少 `<map>`。

**LiteOS harness 门禁契约（`communication_mcu-main-test/test/run_mini_cmsis.sh`）**

- **源文件**：`utils/mini/src/*.cpp`（`tel_os_adapter_posix.cpp` 除外）、`services/*/src/mini/*.cpp`（`call_manager_service_lite.cpp` 除外）、原样的 `frameworks/native/src/call_manager_client.cpp`、`frameworks/native/src/mini/call_manager_proxy.cpp`、`test/mini/mini_selftest.cpp`。
- **include 顺序**：
  1. `utils/mini/include`
  2. `interfaces/innerkits/mini`
  3. `frameworks/native/include/mini`
  4. `services/{audio,call,call_manager_service,call_report,telephony_interaction}/include/mini`
  5. `interfaces/innerkits`
  6. `services/call/include`
  7. harness 的 `stubs/include/liteos`（代替 `//kernel/liteos_m/kal/cmsis`）
  8. `test/mini/stub`（排在最后，只补充 harness 和组件都没有的头文件）
- **宏与选项**：`-DTELEPHONY_MINI_SYSTEM -DTELEPHONY_MINI_LOG_PRINTF -DTELEPHONY_MINI_HOST_TEST`；`-std=c++17 -O1 -fno-exceptions -fno-rtti -fstack-protector-all -Wall -Wextra -Wunused -Wunreachable-code -Wno-unused-parameter -Werror`；链接 `-lpthread`。
- **内核配置**：取 LiteOS-M 默认值，即 Tick 100 Hz、信号量 6、互斥锁 6、软件定时器 5，由 CMSIS 桩强制执行。
- **测试方式**：组件还需要提供 `test/mini/mini_selftest.sh`，用 POSIX 适配运行同一套自测；自测直接驱动 `LosEventHandler`，不经过 samgr_lite。
- **目录前提**：脚本在 `set -e` 下对 `services/` 执行 `find`，因此 Mini 仓中该目录必须存在，否则进程替换会中途退出，自测源文件不会进入编译。第 3 组加入 `services/*/src/mini` 源文件后自然满足；在此之前，本地用一个空目录代替（git 不跟踪空目录）。
- **CMSIS 桩覆盖范围**：提供 `osMutex*`、`osSemaphore*`、`osTimer*`、`osKernel*` 和 `osThreadGetId`；线程、事件标志、消息队列族不在范围内，任务使用 `LOS_TaskCreate`。

**上游核实（OpenHarmony master，已拉取到工作区 `.deps/`）**

- **部件的系统形态**：
  - c_utils（3bdc11e）和 core_service（60b603e）的 `adapted_system_type` 只有 `standard`。因此 `refbase.h`/`singleton.h`/`string_ex.h` 必须由 Mini 提供替身。
  - samgr_lite（c3e127e）、hilog_lite、bounds_checking_function、startup_init、kernel_liteos_m 都有 mini 形态。
- **`telephony_errors.h`**：只 include `<errors.h>`（c_utils，仅包含 `<cerrno>` 和 constexpr 的 `ErrCodeOffset`），可以作为头文件使用；错误码取值由它唯一确定。
- **samgr_lite**：
  - `SAMGR_SendRequest` 按值拷贝 `Request`，以 `DONT_WAIT` 入队，失败时由调用方释放 `data`。
  - 只有 `request.len > 0` 时，框架才会 `free(data)`。
  - `SINGLE_TASK` 服务的 `Initialize` 以 direct request 的形式在服务自身的任务中执行。
  - `TaskConfig.stackSize`/`queueSize` 是 `uint16`；服务名没有长度校验，只检查 4 个回调齐全。
- **kernel_liteos_m `los_config.h` 默认值**：`TSK_LIMIT 5`、`SEM_LIMIT 6`、`MUX_LIMIT 6`、`QUEUE_LIMIT 6`、`SWTMR_LIMIT 5`、`TICK_PER_SECOND 100`。
- **hilog_lite**：`HILOG_*` 宏展开为 `HiLogPrintf(mod, level, argc, fmt, ...)`，走按参数个数记录的延迟格式化路径，不适合非常量 `%s`。
- **syspara**：`int GetParameter(const char *key, const char *def, char *value, uint32_t len)`，与 `call_manager_client.cpp` 的用法一致。

**头文件复用探针（已完成）**

在只提供 Mini 基础替身、且替身先 include 常用 std 头的条件下，以下文件在门禁选项下零告警通过 `-fsyntax-only`：innerkits 类型头、上游 `telephony_errors.h` 与 `errors.h`、`call_base.h`、`call_object_manager.h`、`call_state_listener.h`、`call_request_process.h`、`call_status_policy.h`、`cs_conference.h`。

探针同时暴露一个风险：把 c_utils 的 include 目录加入搜索路径后，它的 `singleton.h`/`nocopyable.h` 会被悄悄引入，与 Mini 替身冲突。

**与既有约定的冲突（需人工复核）**

- **“复用优先于重写”**（AGENTS_71 与用户规则）：受门禁只编译 `src/mini` 目录、以及内核对象池上限的约束，范围内业务逻辑以“复用标准类声明头 + 在 `src/mini` 中按标准控制流移植实现”的方式复用，而不是原样编译标准 `.cpp`。取舍理由见 D2。
- **原样拷贝与“共用逻辑应保留一份”**（AGENTS_71 §2.1）：拷贝文件是标准文件的第二份副本。约束方式：拷贝必须与登记的来源提交逐字节一致，禁止在 Mini 仓修改；标准仓更新后由 `vendor_files.py sync` 刷新，`check` 会把来源 HEAD 已改动的拷贝标为 STALE。同步由谁负责、何时执行，需人工复核。`AGENTS.md` 红线 10 不再涉及：标准 `callmanager.gni` 不变，Mini 仓的 gni 是其自身的唯一真源。
- **标准侧技术债（只记录，本次不修改标准源码）**：
  - `CallControlManager::AnswerCall` 在 `AnswerCallPolicy` 之前就执行了 `SetAnsweredCall(true)` 和 `ANSWERED` 广播。
  - `CallStatusManager::SetContactInfo` 使用 `ffrt::submit([=, &call] ...)` 按引用捕获调用方的 `sptr`。
  - `ConnectCallUiService` 的延迟任务使用 `[&]` 捕获。
  - Mini 移植不引入这些问题：联系人查询与 UI 拉起在 Mini 上被裁剪，接听的可观察序列按标准保留。

**假设（本仓与 harness 无法验证）**

- **A1**：目标产品启用 samgr_lite 与 LiteOS-M 的 C++ 运行时（STL 容器、`std::function`、`std::shared_ptr`、`std::atomic`），并且 CPU 支持无锁的 32 位原子操作（Cortex-M3 及以上）。
- **A2**：cellular_call lite 与 SIM/网络状态、音频的产品实现位于同一映像，并在初始化时通过本设计定义的注册表接入。
- **A3**：产品构建使用 OpenHarmony 轻量构建，`ohos_lite` 与 `ohos_kernel_type == "liteos_m"` 可用于分派，源码树中存在 `//base/telephony/core_service/interfaces/innerkits/include`（仅使用头文件）。

## Goals / Non-Goals

**Goals:**

- 标准仓 telephony_call_manager 零改动；Mini 部件在独立仓中自带 `BUILD.gn`、`bundle.json` 和 gni。
- Mini 源码满足 harness 门禁契约，在 POSIX 和 CMSIS 两种内核适配下以 `-Werror` 零告警编译，自测全部通过。
- 最大化复用标准代码：原样使用（逐字节拷贝）innerkits 类型头与回调接口头、可独立编译的标准类声明头和 `call_manager_client.cpp`；移植实现与标准实现同名、同控制流，便于逐函数对照。
- 内核对象总量固定且装入 LiteOS-M 默认对象池，不随通话数增长。

**Non-Goals:**

- 实现 cellular_call lite、SIM/网络状态或产品音频驱动（本设计只定义注册表接口）。
- LiteOS-A 或小型系统形态，以及 JS/ArkTS/仓颉接口。
- 修复标准侧技术债。
- 在本机运行标准侧 NDK/GN 门禁（`run_tests.sh` 依赖 Windows 侧 NDK；标准源码零改动，回归风险由零改动保证）。

## Decisions

### D1 运行形态：一个 samgr_lite Service，绑定与业务分离

- **服务**：服务名为 `"call_manager"`，`GetTaskConfig` 返回 `SINGLE_TASK`，优先级、栈和队列取自 D12 的命名常量。服务通过 `SYS_SERVICE_INIT` 在 service 阶段注册，并通过默认 Feature 暴露本地 API（IUnknown），供 Mini `CallManagerProxy` 获取服务句柄。
- **绑定隔离**：samgr_lite 相关代码只放在 `services/call_manager_service/src/mini/call_manager_service_lite.cpp`，它把 samgr 的四个回调接到 Mini `CallManagerService` 上：
  - `Initialize`：在服务任务中调用 `LosEventHandler::BindLoopTask()`，然后执行 `CallManagerService::Init()`。
  - `MessageHandle`：调用 `LosEventHandler::Dispatch()`。
  - 投递通道：`SAMGR_SendRequest`，`len = 0`，负载指针的所有权由组件自行管理。
- **host 自测**：自测不编译绑定文件，由 `LosEventHandler` 的自驱动模式创建循环任务来运行同一套业务。
- **初始化顺序**：沿用标准 `Init()` 中的保留步骤，即 `CallControlManager::Init` → `ReportCallInfoHandler::Init` → `CellularCallConnection::Init`（订阅协议栈注册表的就绪通知）→ 音频适配初始化。`CallRecordsManager`、`BluetoothConnection`、`DistributedCallManager`、SA 订阅和 `LocationSubscriber` 是裁剪项，不执行。任一步失败时逆序 `UnInit` 并置为不可用；`SetIdentity` 成功后才置为就绪。
- **选择理由**：用户已选定 samgr_lite 形态。把绑定集中在单个文件中，符合 harness 契约，也让其余代码不依赖 samgr_lite，可以在 host 上测试。
  - 备选“模块自有任务”：需要自行实现服务发现，未采用。
  - 备选“静态集成”：协议栈回调无法与业务状态隔离，未采用。

### D2 目录布局与复用方式

门禁只编译 `src/mini` 目录；同时，在 LiteOS-M 默认 6 个互斥锁的池子下，标准代码里 574 处 `ffrt::mutex` 不可能各自映射为真实的内核锁。因此采用“复用声明、移植实现”：

| 类别 | 内容 | 复用方式 |
|---|---|---|
| 原样复用 | innerkits 类型头与回调接口头（含 `i_call_status_callback.h`、`transfer_control.h`）；`services/call/include` 中的 `call_base.h`、`carrier_call.h`、`cs_call.h`、`conference_base.h`、`cs_conference.h`、`ims_conference.h`、`call_object_manager.h`、`call_state_listener_base.h`、`call_state_listener.h`、`call_status_policy.h`、`call_request_process.h`、`common_type.h`；`frameworks/native/src/call_manager_client.cpp` | 逐字节拷贝到 Mini 仓同名路径并登记来源，不做任何修改，由 Mini 替身提供其依赖。`services/call/include` 中的头在移植对应模块时再拷入 |
| Mini 同名头 | 依赖了裁剪模块或门禁 include 路径之外头文件的标准头：`call_control_manager.h`、`call_policy.h`、`call_request_handler.h`、`call_request_event_handler_helper.h`、`call_status_manager.h`、`ims_call.h`、`cellular_call_connection.h`、`call_status_callback.h`、`report_call_info_handler.h`、`call_ability_report_proxy.h`、`audio_control_manager.h`、`call_manager_service.h`、`call_manager_proxy.h`、`call_ability_callback.h`、`call_number_utils.h`、`core_service_client.h` | 放在对应的 `include/mini/` 下，保留类名、在范围内的方法签名和成员名，去掉裁剪能力的成员和 include |

- **同名头的适用前提**：引号 include 总是先查包含者所在的目录，再按 `-I` 顺序查找。因此，被同目录复用头包含的标准头无法用 Mini 同名头替换。例如 `i_call_status_callback.h`、`transfer_control.h` 与包含它们的 `call_manager_client.h` 同在 `interfaces/innerkits`。这类头改为原样复用，由替身补齐其依赖（Mini `iremote_broker.h`）。上表中的 Mini 同名头都只会从其他目录被包含，由 1.8 的头文件来源检查保证。
| 移植实现 | 上述两类头文件中声明的方法 | 放在 `src/mini/` 下与标准同名的 `.cpp` 中，函数名、参数顺序、控制流、错误码和状态序列与标准一致，省略裁剪分支 |
| Mini 新增 | 内核适配、事件处理器、注册表、权限、日志、基础替身、samgr 绑定、`CallStatusCallbackGuard`、自测 | 放在 `utils/mini/`、`interfaces/innerkits/mini/`、`test/mini/` |

- **移植规则**：
  - 省略的分支按 `specs/mini-feature-scope-build` 中“省略分支对外可识别”的要求返回结果。
  - 复用头中声明、但 Mini 用不到的虚函数，也必须有实现，并返回显式失败（视频类返回 `CALL_ERR_VIDEO_NOT_SUPPORTED`，其余返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`，查询类返回“未启用”值）。
  - 除 AGENTS.md 允许的约束说明外，代码中不写“移植自”之类的注释。对照关系由同名文件和同名函数体现，评审时可以直接 `diff src/X.cpp src/mini/X.cpp`。
- **移植中发现的上游缺陷（标准仓 24e1785f，Mini 按原意实现，标准仓不改）**：
  - `CallObjectManager::AddOneCallObject` 不再把通话插入对象表：3123e36a（2026-09-02）把连接通话 UI 的代码抽成 `ConnectAbilityIfNeed` 时，一并删掉了 `callObjectPtrList_.emplace_back(call)`。Mini 保留插入；照搬会使每一路通话都无法建立（变异测试可复现：5 个流程用例失败）。
  - 对含 `std::string`/`std::vector` 成员的 `CallAttributeInfo`、`CallDetailInfo`、`ContactInfo` 调用 `memset_s`，属于未定义行为。Mini 改为值初始化。
  - `CsConference`/`ImsConference::LeaveFromConference` 在删除最后一个成员后对空集合的 `begin()` 解引用。Mini 先判空。
  - `CallObjectManager::IsNewCallAllowedCreate` 持有 `listMutex_` 时再次调用会加同一把锁的 `GetCarrierCallList`（非递归锁自锁）。Mini 缩小加锁范围（在 Mini 上两者等价，因为业务锁不占内核对象）。
  - `CallRequestEventHandlerHelper::SetDialingCallProcessing` 在投递成功时返回错误（调用方忽略返回值）。Mini 按字面语义返回。
- **Mini 的有意偏离（均有用例覆盖）**：
  - 对象表容量：普通通话最多占 `MAX_CALL_OBJECTS - 1` 个槽位。表满时的来电经协议栈拒接、不入表、不上报；表满时的拨号在本地上报阶段失败，并清除拨号处理标志。
  - `RefreshCall`（CS/IMS 切换）先移除旧对象再加入新对象：新对象沿用旧 callId，表满时先加后删会被容量或去重拒绝。
  - 已有拨号在建立中时，非紧急的第二次拨号在打包拨号参数之前返回 `CALL_ERR_CALL_COUNTS_EXCEED_LIMIT`（标准侧只在 `CALL_MANAGER_CALL_TRANSFER` 下的 `CheckCallLimit` 做此检查），满足“不覆盖正在处理的拨号参数”。紧急呼叫豁免。
  - 会议的合并、拆分、踢出在任何状态变化之前返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`（标准侧先把会议状态置为 CREATING 再下发命令）。
  - 应用回调在主循环中逐条异步投递，不在状态机更新过程中同步回调。
  - 客户端 `UnInit` 走标准侧死亡通知的路径，不经鉴权，只移除自己的回调对象。
  - `CallAbilityCallback` 在头文件中内联实现：harness 门禁在 `frameworks/native/src/mini/` 下只编译 `call_manager_proxy.cpp`。
- **备选方案**：
  - 备选“原样编译标准 `.cpp` + 约 40 个外部头替身 + 约 20 个裁剪模块占位”：门禁不编译标准目录；即使改门禁，也要把 574 处业务锁降级为空锁，并长期维护大量替身，ROM 风险也高。经用户确认，未采用。
  - 备选“在 `src/mini` 中 `#include` 标准 `.cpp`”：会把全部传递依赖带进来，且标准代码要在 `-Werror` 下维持零告警，过于脆弱，未采用。

### D3 内核适配层（`tel_os_adapter`）

- **接口**：`utils/mini/include/tel_os_adapter.h` 只提供通话管理需要的最小集合：
  - 互斥锁：创建、加锁、解锁、删除；
  - 计数信号量：创建、带超时等待、释放、删除；
  - 单次软件定时器：创建、启动（毫秒）、停止、删除；
  - 任务：创建、删除，以及当前任务 ID；
  - Tick：毫秒转 Tick（向上取整，64 位中间值，结果限制在“永久等待”特殊值之下）、当前 Tick。
  - 所有接口返回适配层状态码 `TelOsResult`（`OK`/`INVALID_PARAM`/`NO_RESOURCE`/`TIMEOUT`/`FAIL`），由调用方映射为通话管理错误码（telephony 错误码中没有“超时”）；失败时不留下半创建的对象。
  - 另提供“已创建内核对象计数”查询，供预算用例统计。
- **两套实现互斥编译**：
  - `tel_os_adapter_cmsis.cpp`：目标实现，使用 `osMutexNew`/`osSemaphoreNew`/`osTimerNew`/`osKernelGetTickCount`/`osKernelGetTickFreq`/`osThreadGetId`，任务使用 `LOS_TaskCreate`。
    - LiteOS-M 的 KAL 会把 `osTimerNew` 的参数和 `LOS_TaskCreate` 的 `uwArg` 截断为 32 位，所以定时器和任务的上下文通过静态槽位表的下标传递。
    - LiteOS-M 只对二值信号量强制上限，计数信号量的上限由适配层在释放前检查。
    - 删除定时器时，会等待正在执行的回调结束后再释放槽位。
  - `tel_os_adapter_posix.cpp`：只用于 host 自测，使用 pthread（互斥锁采用 errorcheck 类型，便于在 host 上发现重入）、条件变量实现的计数信号量，以及一个服务全部定时器的线程（模拟 swtmr 任务，回调串行执行）。
  - 两者定义同一组符号，同时链接会报重复定义。
- **ISR**：不支持在 ISR 中调用。LiteOS-M 的 `osThreadGetId` 在中断中返回被抢占的任务，无法据此判断中断上下文，因此只在文档中约束，不做运行时检测。
- **选择理由**：满足 MINI-CONC-01“优先使用产品已启用且语义匹配的 CMSIS/LOS 接口”；POSIX 实现让 host 测试不依赖内核桩，CMSIS 实现则由 harness 在默认对象池下验证。

### D4 事件处理器 `LosEventHandler` 与执行上下文

| 上下文 | 可做 | 不可做 |
|---|---|---|
| 调用方任务（客户端） | 按值拷贝参数，调用 `PostSyncTask` 并在上限时间内等待 | 访问业务状态 |
| 主循环（samgr `MessageHandle` 或自驱动循环任务） | 全部业务状态读写；同步请求直接执行 | 同步投递给自身；阻塞等待 |
| cellular_call lite 回调任务 | 由 `CallStatusCallbackGuard` 校验、按值拷贝、投递 | 修改业务状态 |
| 软件定时器回调 | 只投递 `TIMER_FIRE` | 执行业务逻辑、阻塞 |
| ISR | 不允许调用任何通话管理接口 | — |

- **接口**：
  - `PostAsyncTask(task, delayMs, &taskId)`：立即或延迟投递。
  - `PostSyncTask(task, timeoutMs)`：投递并有限等待；已在主循环时直接执行。
  - `RemoveAsyncTask(taskId)`：只能撤销尚未到期、尚未出队的任务。
  - `IsOnLoop()`：判断当前是否在主循环中。
  - `Dispatch(msg)`：供外部驱动模式调用。
  - 两种模式都由 `Init(config)`/`Stop()` 控制：`Init` 一次性创建全部内核对象，失败时释放已创建的对象；`config.poster` 为空即自驱动。
  - 所有投递入口先登记为“在途调用”再检查状态，`Stop` 在在途调用、等待槽和已投递的定时器消息全部归零后才删除内核对象；有界等待内未归零时保留对象并记录错误，不做可能导致释放后使用的删除。
- **两种驱动模式**：
  - samgr 模式：投递通道为 `SAMGR_SendRequest`（`DONT_WAIT`），由 samgr 的服务任务调用 `Dispatch`。
  - 自驱动模式（host 自测，或没有 samgr 的产品）：环形队列（容量 `CALL_MANAGER_MINI_QUEUE_SIZE`），由互斥锁加计数信号量保护，配 1 个循环任务。
  - 两种模式都是非阻塞投递：队列满时立即返回 `TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL`，并由投递方释放负载。
- **同步请求**：
  - 请求上下文放在堆上，由调用方和主循环共同引用计数；参数按值拷贝进上下文，结果写回上下文。
  - 等待使用初始化时创建的 `CALL_MANAGER_MINI_MAX_SYNC_WAITERS` 个信号量组成的池；池已占满时立即返回 `TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL`。
  - 超时后，调用方在锁内把上下文标记为已放弃；主循环完成任务后，只对未放弃的请求释放信号量，因此放弃的请求不会让信号量多出一个计数、造成下次误唤醒。等待槽在双方都释放后才归还。
- **延迟任务**：最多 `CALL_MANAGER_MINI_MAX_DELAYED_TASKS` 个，按到期时间排序，共用 1 个单次软件定时器，始终对最早到期的任务计时。定时器回调只投递 `TIMER_FIRE`；主循环取出所有已到期的任务，逐个执行，然后重新计时。
- **判定是否在主循环**：绑定时记录主循环任务 ID，判定时与当前任务 ID 比较。
- **退出**：
  - 顺序：置原子“停止中”标志（之后的投递返回 `TELEPHONY_ERR_UNINIT`）→ 停止定时器 → 注销协议栈回调 → 等待在途任务完成 → 回收队列与延迟表中的剩余任务 → 删除内核对象。
  - 退出通过标志位生效，不占用队列槽位，所以不会被业务消息饿死；重复退出直接返回。
- **ffrt 语义的对应关系**：标准中的 `ffrt::submit` 对应 `PostAsyncTask`，`submit_h` 加 `delay` 对应带延迟的 `PostAsyncTask`，`skip` 对应 `RemoveAsyncTask`。
- **`report_call_info_queue` 的处理**：上报本来就由 Guard 投递到主循环，所以 Mini `ReportCallInfoHandler` 在主循环内直接处理。拨号流程在主循环里生成的本地上报也是直接处理，通话对象会在协议栈拨号命令发出之前入表，`HandleDialFail` 因而不需要等待。

### D5 业务状态的主循环限定（替代业务锁）

- **业务状态只在主循环中访问**，这是 AGENTS_71 §4.1“只有确认状态不被其他上下文访问时才可免锁”的前提。覆盖的状态包括：对象表、通话对象、策略状态、拨号槽位、订阅者集合、回调表。所有入口（客户端、上报、定时器）都先投递到主循环，因此业务代码不创建内核对象。
- **Mini `ffrt.h` 的作用**：只为复用的标准头提供成员类型，包括 `ffrt::mutex`、`recursive_mutex`、`shared_mutex`、`condition_variable`、`cv_status`，均不占用内核对象。
  - 加锁操作在 `TELEPHONY_MINI_HOST_TEST` 下断言当前在主循环中，从而在自测中发现违反限定的访问；其余情况下为空操作。
  - `condition_variable::wait_for` 在 Mini 上不阻塞，直接返回 `timeout`；移植实现中不调用它。
- **不使用 std 同步原语**：Mini 代码不使用 `std::mutex`、`std::thread`、`std::condition_variable`。它们在 LiteOS-M 上会隐式消耗内核对象，内核对象只能经 D3 创建。

### D6 服务入口：Mini `CallManagerService`

- **位置**：`services/call_manager_service/{include,src}/mini/call_manager_service.*`。类名、方法名和参数顺序与标准一致，不继承 `SystemAbility`。它只实现范围内的入口和 `Init`/`UnInit`/`SetCallStatusManager`，全部在主循环中执行。
- **入口职责**：
  - 保留标准侧每个入口的判定点和权限组合（下表），经 D9 的权限适配执行；
  - 从标准 `OnDialCall` 迁入 `DialCall` 的白名单重建和号码长度校验，并由身份适配填充 `bundleName`；
  - `RejectCall(isSendSms=true)` 与 `AnswerCall(非语音)` 在任何副作用之前返回错误。

| 入口 | 系统应用判定 | 权限组合（标准侧，Mini 逐字保留） |
|---|---|---|
| `RegisterCallBack` | 否 | `SET_TELEPHONY_STATE` 或 `GET_TELEPHONY_STATE` 或 `GET_CALL_TRANSFER_INFO` |
| `UnRegisterCallBack` | 是 | `SET_TELEPHONY_STATE` |
| `ObserverOnCallDetailsChange` | 是 | `SET_TELEPHONY_STATE` 或 `GET_TELEPHONY_STATE` |
| `DialCall` | 是 | `PLACE_CALL` |
| `AnswerCall(callId, videoState, isRTT)` | 是 | `ANSWER_CALL` |
| `AnswerCall()` | 否 | `ANSWER_CALL` 或 `MANAGE_CALL_FOR_DEVICES` |
| `RejectCall(callId, withMsg, text)` | 是 | （`ANSWER_CALL` 或 `SET_TELEPHONY_STATE`），且 `withMsg` 时追加 `SEND_MESSAGES` |
| `RejectCall()` | 否 | `ANSWER_CALL` 或 `SET_TELEPHONY_STATE` 或 `MANAGE_CALL_FOR_DEVICES` |
| `HangUpCall(callId)` | 是 | `ANSWER_CALL` 或 `SET_TELEPHONY_STATE` |
| `HangUpCall()` | 否 | `ANSWER_CALL` 或 `SET_TELEPHONY_STATE` 或 `MANAGE_CALL_FOR_DEVICES` |
| `HoldCall` / `UnHoldCall` / `SwitchCall` | 是 | `ANSWER_CALL` |
| `EndCall` | 是 | `ANSWER_CALL` 或 `SET_TELEPHONY_STATE` |
| `IsRinging` | 是 | `SET_TELEPHONY_STATE` |
| `IsNewCallAllowed` | 是 | 无 |
| `GetCallState` / `HasCall` / `IsEmergencyPhoneNumber` | 否 | 无 |

- **防分叉措施**：T7.3 用脚本从标准 `call_manager_service.cpp` 中提取每个入口的判定点与权限常量，与 Mini 实现中的同一张表比对。任一侧发生变化都会使检查失败。

### D7 客户端：`call_manager_client.cpp` 原样编译，Mini `CallManagerProxy`

- **`CallManagerProxy`**：`frameworks/native/{include,src}/mini/call_manager_proxy.*` 与标准同名，并声明 `call_manager_client.cpp` 调用到的全部方法。
  - `Init` 通过绑定注册的本地服务句柄（host 自测中由测试注入）连接服务；服务不可用时返回 `TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL`。每次调用前检查服务是否就绪。
  - 范围内方法把参数按值拷贝进同步请求上下文，交给 `LosEventHandler::PostSyncTask` 执行。
  - 范围外方法：返回 `int32_t` 的直接返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`，返回 `sptr` 的返回空指针，权限类 `bool` 返回 `false`。
- **回调**：Mini `CallAbilityCallback` 是由 `sptr` 持有的本地对象，持有 `std::unique_ptr<CallManagerCallback>` 和“已关闭”标志。`RegisterCallBack` 保持标准侧 `registerStatus_` 的单回调语义，拒绝空回调。
- **回调接口头**：`i_call_status_callback.h`、`transfer_control.h` 原样复用（原因见 D2 的“同名头的适用前提”）。Mini `iremote_broker.h` 提供一个继承 `virtual RefBase`、没有 `AsObject()` 的 `IRemoteBroker`，以及接口描述符宏，因此任何代码都无法把这些本地接口当作远端对象发送。

### D8 协议栈、SIM/网络状态与上行入口：注册表式本地接口

- **注册表接口**：`interfaces/innerkits/mini/` 提供三个抽象接口，注册表实现在 `utils/mini/src/`：
  - `cellular_call_lite_interface.h`：下行方法集合与 `specs/mini-platform-adaptation` 一致，签名沿用 `CellularCallInterface` 的对应方法，不含 `IRemoteBroker`/`Surface`。
  - `core_service_lite_interface.h`：Mini 移植中用到的 SIM/网络查询。
  - `call_audio_lite_interface.h`：见 D10。
- **注册表行为**：用原子自旋锁保护（叶子锁，竞争时以 1 ms 延时退避，不占内核对象；注册可能早于通话管理初始化，静态初始化的自旋锁无需创建步骤）；就绪通知在锁内快照、锁外分发。注册表为空时返回 `TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL`；范围外方法返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`。
- **Mini `CellularCallConnection`**：收到就绪通知后，把“注册上行回调”投递到主循环，并幂等地只执行一次。
- **Mini `CoreServiceClient`**（同名，`telephony_interaction/include/mini`）：把 `DelayedRefSingleton<CoreServiceClient>::GetInstance().HasSimCard(...)` 这类调用转发到注册表，因此移植代码中这些调用的写法与标准一致。
- **上行入口**：cellular_call lite 调用的 `ICallStatusCallback` 由 `CallStatusCallbackGuard` 实现，它承接原 `CallStatusCallbackStub` 的反序列化职责：
  - 校验卡槽号、枚举值、号码 NUL 结尾且长度不超过 255、批量条目不超过 5；
  - 校验通过后按值拷贝并投递到主循环，再交给 Mini `CallStatusCallback`（移植实现）处理。
  - Guard 对象由通话管理创建并以 `sptr` 持有，注销完成后才释放。

### D9 身份与权限：保留判定点，默认拒绝

- `utils/mini/{include,src}/call_permission_adapter.*` 提供 `CheckPermission(const char*)`、`IsSystemCaller()` 和 `GetCallerBundleName()`。
- **判定依据**：编译期产品配置 `call_manager_mini_granted_permissions`（字符串列表，默认为空）、`call_manager_mini_caller_is_system_app`（默认 `false`）和 `call_manager_mini_caller_bundle_name`（默认为空字符串），由 gni 转换为 defines。
- **信任粒度**：单映像内不存在进程或应用身份，所以信任粒度是“整个映像内的调用方”，需要产品安全设计确认（需人工复核）。
- **备选方案**：
  - “恒真通过”属于弱安全替代，被规则禁止，未采用。
  - “运行时登记调用方任务 ID”：任务 ID 可伪造，并不更安全，未采用。

### D10 音频：降级为 Mini 同名 `AudioControlManager`

- **位置**：`services/audio/{include,src}/mini/audio_control_manager.*`，类名为 `AudioControlManager`，继承 `CallStateListenerBase`，只实现移植代码中调用到的方法。
- **行为映射**：
  - INCOMING：开始播放铃声。
  - ANSWERED/ACTIVE：停止铃声，并激活通话音频通路。
  - WAITING：播放等待音。
  - 最后一路通话进入 DISCONNECTED：释放通话音频通路，并清零全部标志。
  - `SetMute`、DTMF 和设备选择：返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`。
- **产品音频接口**：`call_audio_lite_interface.h` 提供 `SetCallAudioActive(bool)`、`StartRingtone()`、`StopRingtone()`、`PlayWaitingTone()`、`StopWaitingTone()`。未注册实现时，每个操作返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`，并且只记录一次日志；与标准侧一致，音频失败不阻断状态机。
- **选择理由**：标准音频实现直接使用 AudioStandard 类型共 46+105 处，设备路由、彩振、蓝牙/星闪/分布式音频本身就在裁剪范围内。这是唯一不复用标准控制流的降级模块，需人工复核。

### D11 基础替身与日志（`utils/mini/include`）

- **std 前置包含**：这组 std 头集中在 `tel_mini_std_includes.h` 中，`refbase.h`、`singleton.h`、`ffrt.h`、`pac_map.h` 都先 include 它，以补齐复用头中缺失的包含（见 Context 中的探针结果）。
- **`refbase.h`**：`RefBase` 使用原子强/弱计数，提供 `sptr`/`wptr`/`MakeSptr`，只包含被使用的子集，最后一个强引用释放时销毁对象。
- **`singleton.h`**：`DelayedSingleton`/`DelayedRefSingleton`/`Singleton`。首次构造用原子状态机保证只构造一次；等待方经 D3 让出 CPU，而不是忙等，避免在单核 RTOS 上发生优先级反转导致的活锁。不依赖编译器的线程安全静态局部变量（该机制在部分 MCU 工具链上不提供）。
- **`pac_map.h`**：`AppExecFwk::PacMap` 的 Int/String/Boolean Put/Get（带缺省值）、`Clear` 和拷贝，条目上限为 `CALL_MANAGER_MINI_PACMAP_MAX_ENTRIES`，超出时 Put 失败并记录日志。
- **其他替身**：
  - `want_params_wrapper.h`：提供空的 `AAFwk::WantParams`，只为复用 `common_type.h`。
  - `string_ex.h`：提供 `Str16ToStr8`/`Str8ToStr16`。
  - `nocopyable.h`：`singleton.h` 依赖它。
  - `iremote_broker.h`：见 D7。
- **`errors.h`**：逐字节拷贝上游 c_utils `base/include/errors.h`（3bdc11e），来源登记在 `vendor_manifest.json` 中。这样不必把 c_utils 的 include 目录加入搜索路径，避免它的 `singleton.h` 等头文件被引入。
- **`telephony_log_wrapper.h`**：
  - `TELEPHONY_LOG{D,I,W,E,F}` 调用 `TelMiniLog(level, fmt, ...)`。
  - 该函数在运行时把格式串中的 `{public}`/`{private}` 去掉，再格式化到定长缓冲区后输出。
  - 默认后端为 printf（`TELEPHONY_MINI_LOG_PRINTF`）。由于 hilog_lite 采用延迟格式化，不接受非常量 `%s`，hilog_lite 后端作为待定项。
  - `TelMiniLog` 不带 `format` 属性，因此原样复用代码中的 `%{public}s` 不会触发 `-Wformat` 错误。

### D12 构建与自测

- **`callmanager_mini.gni`**（Mini 仓唯一真源，部件路径 `//base/telephony/call_manager_mini`）：
  - `call_manager_mini_sources`：与门禁的文件集合一致，再加上 `call_manager_service_lite.cpp` 和 `tel_os_adapter_cmsis.cpp`，但不含 `tel_os_adapter_posix.cpp` 和自测。
  - `call_manager_mini_include_dirs`：顺序与门禁完全一致，外部依赖目录排在最后，依次为 `//kernel/liteos_m/kal/cmsis`、`//kernel/liteos_m/kernel/include`（`los_task.h`）、samgr_lite、bounds_checking_function、syspara 和 core_service innerkits include。
  - `call_manager_mini_defines`：`TELEPHONY_MINI_SYSTEM`，以及由 `declare_args()` 转换来的常量和权限配置。
  - `call_manager_mini_cflags_cc`：与门禁一致，但去掉 `-g -O1`，保留 `-Os`。
- **`BUILD.gn`**（Mini 仓根目录）：import `//build/lite/config/component/lite_component.gni`；`ohos_kernel_type == "liteos_m"` 时定义 `static_library("tel_call_manager_mini")`。
- **`bundle.json`**（Mini 仓根目录）：
  - 部件信息：部件 `call_manager_mini`、子系统 `telephony`、`adapted_system_type` 为 `["mini"]`、`destPath` 为 `base/telephony/call_manager_mini`。
  - 依赖：samgr_lite、liteos_m、bounds_checking_function、init。
  - Mini 构建参数登记到 `features`。
- **原样拷贝**：`vendor_manifest.json` 登记每个拷贝文件的来源仓、来源路径、提交，以及是否随产品发布（`test/mini/stub` 下的为 host 专用）。`vendor_files.py check` 负责核对逐字节一致性和关键错误码取值，`sync <source>` 从来源 HEAD 刷新并更新登记的提交。
- **自测**：
  - `test/mini/mini_selftest.cpp`：单一翻译单元，内含最小用例框架，可 include `test/mini/cases/*.inc`。
  - `test/mini/mini_selftest.sh`：与 `run_mini_cmsis.sh` 相同的源文件、include 顺序和选项，换成 POSIX 适配。另外两个阶段：以 gni 生成的 define 形式编译权限宏注入探针；存在 `.deps/samgr_lite`、`.deps/utils_lite`（上游 master 1e88eb4d，提供 `ohos_init.h`、`ohos_errno.h`）和 harness LiteOS 桩时，对 samgr 绑定执行 `-fsyntax-only -Werror`，缺少时明确输出 SKIP。
  - 用例框架支持 `MINI_TEST_FILTER` 环境变量，用于 harness 这类不向自测传参的门禁（例如 `SEM_LIMIT=1` 的回滚用例）。
  - `test/mini/stub/`：只放 host 需要、而目标产品由上游提供的头文件：
    - `securec.h`（host 内联实现）；
    - `telephony_errors.h`（逐字节拷贝上游 core_service 60b603e，登记在清单中，由 `vendor_files.py check` 核对取值）；
    - `parameter.h`（host 版 `GetParameter`）。
- **安全选项**：
  - 保留：`-fno-exceptions -fno-rtti -fstack-protector-all -Wall -Wextra -Werror`。
  - `-D_FORTIFY_SOURCE=2`：未设置。它依赖产品 libc 的强化头文件并要求优化构建；产品 libc 提供时由产品工具链开启。风险：`securec` 之外的 libc 字符串/内存函数没有编译期越界检查；Mini 代码中的拷贝一律经 `memcpy_s`/`memset_s` 并带长度校验，号码长度在 Guard 和服务入口两处限制。
  - 不适用并记录风险：`cfi`/`cfi_cross_dso`（LiteOS-M 静态单映像无 CFI 运行时；间接调用仅限构建期注册的适配接口）、`branch_protector_ret = "pac_ret"`（仅 AArch64，LiteOS-M 目标为 Cortex-M/RISC-V）。
  - 记录位置：`BUILD.gn` 文件头注释与本节；`test/mini/tools/check_mini_rules.sh` 第 5 项确认 gni 和两个门禁脚本都启用保留的选项。

### 功能分类表

| 模块 / 调用面 | 分类 | Mini 处理 | 与标准的差异 | 验证 |
|---|---|---|---|---|
| innerkits 纯类型头 | 兼容保留 | 原样使用 | 无 | 两个门禁编译 |
| `call_manager_client.{h,cpp}` 范围内方法 | 兼容保留 | 逐字节拷贝后原样编译 | 无 | 自测 client 用例 + `vendor_files.py check` |
| `call_manager_client` 范围外方法 | 兼容占位 | Mini `CallManagerProxy` 返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`、空指针或 `false` | 能力不可用 | 自测占位用例 |
| `CallManagerProxy`、`CallAbilityCallback` | 适配 | D7 | 无死亡监听，改为就绪检查 | 自测 |
| `frameworks/native` 其余 IPC、`frameworks/js`/`ets`/`cj`、`interfaces/kits` | 裁剪 | 不进入 Mini | — | 产物符号检查 |
| `call_manager_service.cpp` | 适配 | D6 | 无 SA/dump；鉴权经适配 | 鉴权对照检查 + 自测 |
| `call_manager_service_stub.cpp` | 裁剪（职责迁移） | 白名单与长度校验迁到 D6 | 无 | 白名单用例 |
| `services/call` 核心（对象表、策略、控制、请求、状态机、广播、通话对象与会议类） | 能力保留 | D2 复用声明、移植实现 | 省略裁剪分支；对象表容量为 5（1 个保留给 ECC） | 自测 |
| `call_request_handler`、`call_request_event_handler_helper` | 适配 | 移植到 `LosEventHandler` | 投递失败返回错误 | 队列满用例 |
| OTT/VoIP/卫星/蓝牙/视频通话类型 | 裁剪 | 不移植；拨号入口按类型拒绝，上报的此类来电按表满策略拒接 | 类型不可用 | “请求卫星通话”用例 |
| UI 拉起、广播订阅、有线耳机、超级隐私、`call_state_observer/*`、来电过滤 | 裁剪 | 不移植，不注册订阅者；策略中对应判定按“未开启”处理 | 无 UI、通话记录、通知、隐私模式、拦截 | 分类记录 + ECC/来电用例 |
| `call_status_callback.cpp` | 能力保留 | 移植；基类为 Mini 本地接口 | 无 | 上行用例 |
| `call_status_callback_stub.cpp` | 裁剪（职责迁移） | 校验迁到 `CallStatusCallbackGuard` | 无 | 非法上报用例 |
| `report_call_info_handler.cpp` | 适配 | 移植；在主循环内直接处理 | 无独立 ffrt 队列 | 拨号失败用例 |
| `cellular_call_connection.cpp`、`CoreServiceClient` | 适配 | D8 注册表 | 实现缺失时显式失败 | 适配用例 |
| `call_ability_report_proxy.cpp` | 适配 | 单回调注册表，锁外异步分发，数据按值拷贝 | 无 bundle 维度，只有一个回调 | 回调用例 |
| `call_state_report_proxy` 及其余 `call_report` 文件 | 裁剪 | 不移植、不注册 | 无 state_registry 广播 | 符号检查 |
| `services/audio/*` | 降级 | D10 | 仅通路激活与铃声，无路由 | 音频用例 |
| `services/{bluetooth,distributed_call,interoperable_call,video,display,spam_call,antifraud,number_identity_proxy,satellite_call,call_setting,call_voice_assistant,call_earthquake_alarm,rtt_call,hisysevent,deps_adapter}` | 裁剪 | 不进入 Mini | — | 符号检查 |
| `utils/call_number_utils` | 能力保留 | Mini 同名头与移植实现：去分隔符、紧急号码判定、卡槽校验；格式化、归属地、黄页返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED` | 格式化不可用 | 号码工具用例 |
| `utils` 其余 | 裁剪 | 不进入 Mini | — | 符号检查 |
| `sa_profile`、`services/etc/init` | 裁剪 | 不进入 Mini | 无 SA 配置 | 构建 |
| DTMF、静音 | 兼容占位 | 返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED` | 能力不可用，可在后续变更中提升为保留 | 占位用例 |
| `extraParams` 拨号字段 | 降级 | 按白名单接受，但不解析 | 不支持扩展参数 | 白名单用例 |

### IPC 非传输职责迁移

| 原位置 | 职责 | Mini 承接位置 |
|---|---|---|
| `CallManagerServiceStub::OnDialCall` | extras 白名单重建、号码长度校验 | D6 `DialCall` 入口 |
| `CallManagerServiceStub::OnRegisterCallBack` | 空回调拒绝 | D7 `RegisterCallBack` |
| `CallManagerService` 各入口 | 系统应用判定、权限组合、`bundleName` 来源 | D6 + D9 |
| `CallManagerProxy` 死亡监听与重连 | 服务可用性 | D7 就绪检查 |
| IPC 线程切换 | 调用方与业务状态隔离 | D4 投递规则 + D5 主循环限定 |
| `CallStatusCallbackStub` 反序列化 | 上报字段合法性、数量上限 | D8 `CallStatusCallbackGuard` |
| `CellularCallConnection` 的 SA 监听 | 协议栈可用性与回调重注册 | D8 注册表就绪通知 |

### 资源所有权与预算

| 资源 | 创建方 | 所有者 / 转移 | 释放方 / 失败回收 |
|---|---|---|---|
| samgr 服务任务与队列 | samgr_lite（按 `GetTaskConfig`） | samgr_lite | samgr_lite |
| 异步任务 / 上报负载 | 投递方 | 投递成功后转移给主循环 | 主循环执行后释放；投递失败时由投递方释放；退出时由主循环回收 |
| 同步请求上下文 | 调用方 | 调用方与主循环共享引用计数 | 最后一个引用的释放者 |
| 延迟任务条目 | `PostAsyncTask(delay>0)` | 延迟表 | 执行方或撤销方，只释放一次 |
| 上行回调对象（Guard） | Mini `CellularCallConnection` | 注册表持有 `sptr` 副本 | 注销后由最后一个 `sptr` 释放 |
| 应用回调持有者 | `RegisterCallBack` | 回调表与在途分发共享 `sptr` | 注销并完成在途分发后释放 |

| 内核对象（默认配置） | samgr 模式 | 自驱动模式（host 自测） | 用途 |
|---|---|---|---|
| 互斥锁 | 1 | 1 | 事件处理器状态锁（注册表用原子自旋锁，不占互斥锁） |
| 信号量 | 2 | 3 | 同步等待者池（`MAX_SYNC_WAITERS`=2）+ 自驱动队列计数 |
| 软件定时器 | 1 | 1 | 延迟任务 |
| 任务 | 0（samgr 服务任务另计） | 1 | 自驱动循环 |

| 常量（`callmanager_mini.gni` 的 `declare_args` → defines，`call_manager_mini_config.h` 提供缺省值和 `static_assert`） | 默认值 | 依据 |
|---|---|---|
| `CALL_MANAGER_MINI_TASK_PRIORITY` | `PRI_NORMAL`（24） | samgr_lite `TaskPriority` |
| `CALL_MANAGER_MINI_TASK_STACK_SIZE` | 16384 字节 | 初值；拨号时上报在同一调用链内直接处理，调用链较深；须不大于 `uint16` 上限；T7.6 在目标板实测后定稿 |
| `CALL_MANAGER_MINI_QUEUE_SIZE` | 16 | 在 5 路通话的上报突发之上留余量 |
| `CALL_MANAGER_MINI_SYNC_WAIT_MS` | 3000 | `specs/mini-call-client-api` |
| `CALL_MANAGER_MINI_MAX_SYNC_WAITERS` | 2 | `specs/mini-call-runtime` 的内核对象预算 |
| `CALL_MANAGER_MINI_MAX_CALL_OBJECTS` | 5（1 个保留给 ECC） | `specs/mini-voice-call-control`，与 `MAX_CALLS_NUM` 一致 |
| `CALL_MANAGER_MINI_MAX_DELAYED_TASKS` | 8 | 保留路径上的延迟任务点数量加余量 |
| `CALL_MANAGER_MINI_PACMAP_MAX_ENTRIES` | 32 | D11 |
| `CALL_MANAGER_MINI_SLOT_COUNT` | 2 | 标准侧运行时读取 `const.telephony.slotCount`；Mini 编译期固定，上限为 `CallStatusManager` 的 `SLOT_NUM`（2） |
| `ACCOUNT_NUMBER_MAX_LENGTH`、`kMaxNumberLen`、`MAX_CALLS_NUM` | 255 / 255 / 5 | 沿用标准常量 |
| `PENDINGHANGUP_DELAY_TIME` 等业务时长 | 沿用标准值 | 保持语义 |

### 锁与并发规则

- **业务无锁**：业务状态受 D5 的主循环限定保护，不需要业务锁。
- **适配层锁一律是叶子锁**：包括事件处理器状态锁和注册表自旋锁。持有它们时，不调用业务代码、产品适配实现或回调；持有自旋锁时也不调用任何可能阻塞的内核接口。
- **同步等待的约束**：同步等待只发生在非主循环的调用方任务中，且只等待信号量，不持有任何锁。
- **回调分发**：回调表在主循环中修改；分发时在主循环中取快照，再逐个异步投递，因此应用回调执行时不持有任何锁。

## Risks / Trade-offs

- **[移植实现与标准实现逐渐分叉]**：→ 同名文件、同名函数、同控制流，评审时可以逐函数 `diff`；鉴权表由 T7.3 自动比对；关键不变量（策略先于副作用、ECC 旁路、单拨号槽位、`ANSWERED` 中间态、`DisconnectedHandle` 清位）由自测逐条覆盖；分类表记录每处省略。
- **[拷贝文件与标准仓逐渐不同步]**：标准仓更新 innerkits 头或 `call_manager_client.cpp` 后，Mini 仓仍停留在旧版本。→ `vendor_files.py check` 会把来源 HEAD 已改动的拷贝标为 STALE；刷新只能通过 `sync` 进行，并在同一次提交中重新跑两个门禁。同步节奏需人工复核。
- **[同名头文件导致 ODR 混用]**：→ include 顺序由 gni、`mini_selftest.sh` 和门禁三处一致保证（T1.7 检查）；T1.8 用 `-H` 输出核对头文件来源；不把 c_utils 的 include 目录加入搜索路径。
- **[主循环限定被违反]**：→ host 自测中，Mini `ffrt::mutex` 的加锁操作会断言当前在主循环中；Guard 与代理只做拷贝和投递。
- **[内核对象预算与产品其他部件冲突]**：→ 数量固定且在本文列明；产品在 `target_config.h` 中统一核算；CMSIS 门禁使用默认对象池验证。
- **[默认拒绝导致开箱不可用]**：→ 这是有意的 fail-closed 设计。产品集成需要配置的最小权限集合为 `PLACE_CALL`、`ANSWER_CALL`、`GET_TELEPHONY_STATE`、`SET_TELEPHONY_STATE`，并声明系统应用（需人工复核）。
- **[整映像信任扩大了调用面]**：→ 由产品安全设计明确接受或收紧，本仓不提供更弱的替代。
- **[外部组件不存在或接口不符（A1–A3）]**：→ 接口由本仓定义并提供测试替身；发现不符时记录为阻塞项，不以桩实现冒充。
- **[C++/STL 占用超出 ROM/RAM 预算]**：→ T7.7 在目标上测量；超出时另起裁剪变更。
- **[日志后端]**：printf 后端在部分板子上可能占用串口带宽。→ 热路径不输出 info 日志；hilog_lite 后端列为待定项。
- **[CFI/PAC 不可用]**：→ 在 D12 中记录，由产品安全设计评估。

## Migration Plan

1. **独立仓与自测骨架**：新建 Mini 仓并原样拷贝复用文件；完成 host 桩、内核适配（POSIX 与 CMSIS）、基础替身、自测框架、让 `call_manager_client.cpp` 编译通过的骨架、`callmanager_mini.gni`，以及布局与头文件来源检查。完成后两个门禁即可运行。
2. **`LosEventHandler` 与权限适配**，连同内核对象预算用例。
3. **注册表接口与 Guard**。
4. **通话业务移植**：通话对象 → 对象表 → 广播 → 协议栈连接 → 上行链路 → 策略 → 下行链路，最后执行高危序列用例。
5. **服务入口、客户端与回调**，以及 samgr 绑定。
6. **音频降级**。
7. **`BUILD.gn`/`bundle.json` 与收尾检查**，最后完成两个门禁的复跑。

**回滚**：Mini 是独立部件，产品配置中不再引入 `call_manager_mini` 即可回滚。标准仓从未被改动，标准产物不受影响。

## Open Questions

- 产品安全设计最终授予 Mini 映像的权限集合和系统应用声明。只影响产品配置值。
- 是否为“同步等待超时”和“队列满”申请专用错误码。规格已规定复用 `TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL`，新增错误码属于跨部件变更，需另行评审。
- hilog_lite 日志后端的接入方式（是否支持非常量字符串）。默认 printf 后端不受影响。
- 目标产品的 ROM/RAM 预算和栈大小定稿值，由 T7.6/T7.7 在目标板实测。
- DTMF 和静音是否在后续变更中由兼容占位提升为保留。
