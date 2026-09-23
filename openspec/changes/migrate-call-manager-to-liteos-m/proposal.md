# Proposal

## Why

`telephony_call_manager` 目前只支持标准系统（`bundle.json` 的 `adapted_system_type` 仅为 `standard`）：它以 SA 4005 的形态运行在独立进程 `telecom` 中，产物为 `libtel_call_manager.z.so`，并依赖 IPC、samgr、ffrt、AccessToken、ability_base、audio_framework 等约 40 个标准部件。LiteOS-M（Mini，单进程映像）产品需要基础语音通话能力，但无法直接运行这套实现。本变更依据工作区根目录的 `AGENTS_71.md`《标准鸿蒙通信模块 → Mini（LiteOS-M）迁移规则》，把“最小语音集”迁移到 Mini：在独立仓中得到一个可构建、可链接、能在 LiteOS harness 上自测的 Mini 部件，同时保证标准仓不受任何改动。

## What Changes

- **新增独立的 Mini 部件仓**：新建独立仓 `telephony_call_manager_mini`（OHOS 部件 `call_manager_mini`，`destPath` 为 `base/telephony/call_manager_mini`），提供 LiteOS-M 静态库目标 `tel_call_manager_mini`，以及自己的 `BUILD.gn`、`bundle.json` 和 `callmanager_mini.gni`。
  - **目录布局**：沿用标准仓的相对布局，Mini 实现放在各模块的 `src/mini/`、`include/mini/`，以及 `utils/mini/`、`interfaces/innerkits/mini/` 中，专用宏为 `TELEPHONY_MINI_SYSTEM`。
  - **原样复用方式**：标准文件（innerkits 类型头、`call_manager_client.cpp` 等）从标准仓逐字节拷入同名路径，在 `vendor_manifest.json` 中登记来源提交，并由脚本核对。
  - **标准仓零改动**：telephony_call_manager 仓不做任何改动。
- **运行形态改为 samgr_lite Service**：把 `CallManagerService` 的 SystemAbility 职责迁移到一个 samgr_lite `Service`（`SINGLE_TASK`，`MessageHandle` 作为主事件循环）。samgr_lite 绑定单独放在 `call_manager_service_lite.cpp`，其余 Mini 代码不依赖 samgr_lite，可以在 host 上由事件处理器直接驱动。
- **删除非必要 IPC**：Mini 构建不包含任何 Stub、Proxy、Parcel、DeathRecipient 和 SA 注册代码。`CallManagerClient` 作为兼容调用面，其 `call_manager_client.cpp` 以逐字节拷贝的形式原样编译，下层换成 Mini 版 `CallManagerProxy` 的进程内调用。原 IPC 边界上的参数白名单、长度校验、鉴权收口和线程切换职责迁移到 Mini 服务入口。
- **复用与移植最小语音集的业务**：
  - 复用 innerkits 类型头，以及能独立编译的标准类声明头（如 `call_base.h`、`call_object_manager.h`、`call_state_listener.h`）。
  - 保留 CS/IMS 语音通话的拨号、接听、拒接（不含拒接短信）、挂断、保持、解除保持、切换，以及紧急呼叫（ECC）旁路。
  - 这部分业务逻辑按标准侧的函数名、控制流、错误码和状态序列，移植为 `src/mini/` 下的同名实现，省略裁剪能力的分支。
- **适配平台能力**：
  - 内核适配层：CMSIS-RTOS2 实现随目标发布，POSIX 实现用于 host 自测，两者互斥编译。
  - 事件处理器 `LosEventHandler`：提供立即、延迟、带上限的同步投递和撤销。
  - 协议栈、核心服务（SIM/网络状态）和音频的本地注册表接口。
  - 身份与权限适配，以及日志适配。
  - `PacMap`、`RefBase`/`sptr`、`DelayedSingleton` 等基础类型的 Mini 替身。
  - 内核对象在初始化时一次性创建，总数装入 LiteOS-M 默认对象池。
- **裁剪与兼容占位**：
  - **从 Mini 运行路径排除的能力**：视频、会议、OTT/VoIP、卫星、蓝牙 HFP、分布式/互通、RTT、防诈/骚扰拦截、超级隐私、通话记录与通知、补充业务、语音播报、地震预警，以及 JS/ArkTS/仓颉胶水。
  - **对外表现**：Mini 同名实现省略这些分支后，对外表现为“未启用/不支持”。`CallManagerClient` 中范围外的公开方法返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`，不得伪造成功。
- **Mini 自测**：
  - 组件自带 `test/mini/mini_selftest.{cpp,sh}`（POSIX 适配）；
  - 同一套源码能通过 LiteOS harness 的 `run_mini_cmsis.sh`（CMSIS 适配与 LiteOS-M 内核桩），且两者都以 `-Wall -Wextra -Werror` 编译。
- 不涉及 **BREAKING** 变更：标准构建的公开接口、IPC 枚举值、错误码和 `.d.ts` 均不改动。

## Capabilities

### New Capabilities

- `mini-call-runtime`：Mini 运行形态与生命周期，包括 samgr_lite Service 注册、幂等且可回滚的初始化、就绪状态、主事件循环、上下文投递规则、退出顺序和内核对象预算。
- `mini-call-client-api`：Mini 侧 `CallManagerClient` 兼容调用面，包括本地调用、同步等待上限、回调注册与分发、范围外方法的显式失败，以及鉴权与参数校验职责的保留。
- `mini-voice-call-control`：最小语音集的下行控制，包括拨号/接听/拒接/挂断/保持/解除保持/切换、策略先于副作用、ECC 旁路、单槽位拨号参数交接和拨号失败处理。
- `mini-call-state-reporting`：上行状态处理与广播，包括协议栈回调入口、串行状态机、状态广播、订阅者范围，以及对应用回调的锁外分发。
- `mini-platform-adaptation`：平台能力适配，包括协议栈、内核适配与事件处理器、权限与身份、音频、日志、基础类型的适配，以及它们的失败语义。
- `mini-feature-scope-build`：功能分类与构建分派，包括分类表、依赖闭包、裁剪项排除、省略分支的可识别性、标准源文件零改动、Mini 自测门禁、外部头文件来源，以及安全编译选项的保留。

### Modified Capabilities

（无。`openspec/specs/` 下目前没有已有规格。）

## Impact

- **代码（全部位于新仓 `telephony_call_manager_mini`，telephony_call_manager 仓不改动）**：
  - `vendor_manifest.json` 与原样拷贝的标准文件：`interfaces/innerkits` 中用到的类型头与回调接口头、`frameworks/native/src/call_manager_client.cpp`、`LICENSE`，以及后续移植用到的 `services/call/include` 头。
  - `utils/mini/{include,src}`：基础替身、内核适配、事件处理器、权限与日志。
  - `interfaces/innerkits/mini`：协议栈、核心服务与音频注册表接口。
  - `frameworks/native/{include,src}/mini`：Mini 版 `CallManagerProxy`。
  - `services/call_manager_service/{include,src}/mini`：服务入口与 samgr_lite 绑定。
  - `services/call/{include,src}/mini`：通话业务移植。
  - `services/telephony_interaction/{include,src}/mini`：协议栈连接与上行入口。
  - `services/call_report/{include,src}/mini`：本地回调上报。
  - `services/audio/{include,src}/mini`：音频降级。
  - `test/mini/`：自测程序、脚本和 host 专用桩。
- **构建**：新仓自带 `BUILD.gn`、`bundle.json`（`adapted_system_type` 为 `["mini"]`）和 `callmanager_mini.gni`。标准仓的目标、源文件、依赖和编译选项不受影响。
- **外部依赖（Mini 侧，均有上游 mini 形态）**：samgr_lite、kernel_liteos_m（CMSIS-RTOS2）、bounds_checking_function（`securec.h`）、startup_init 的 syspara（`parameter.h`），以及 core_service 的 `telephony_errors.h`（仅作为头文件使用）。cellular_call lite 和产品音频/SIM 状态实现由产品通过注册表接入。
- **测试**：新仓自带 Mini 自测程序、门禁脚本和拷贝文件核对脚本。标准仓零改动，标准单测不受影响。
- **需人工复核**：
  - Mini 权限与身份信任模型（涉及鉴权收口）；
  - 范围内业务逻辑以移植方式复用，而不是原样编译标准 `.cpp`（受 harness 门禁与内核对象上限约束，偏离“复用优先于重写”）；
  - 音频降级；
  - 新部件 `call_manager_mini` 的 `bundle.json` 与构建参数；
  - 拷贝文件与标准仓之间的同步策略（标准仓更新后，由谁、何时执行 `vendor_files.py sync`）。
