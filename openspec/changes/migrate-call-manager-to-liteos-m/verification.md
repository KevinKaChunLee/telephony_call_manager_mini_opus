# 验证记录（任务 7.8）

对象：独立仓 `telephony_call_manager_mini`（未提交，本地 `git init`），标准仓 `telephony_call_manager` 24e1785f（零改动）。
环境：WSL2 x86-64 host，g++，无 OpenHarmony 源码树、无目标板。

## 门禁运行

| 运行 | 结果 |
|---|---|
| `bash test/mini/mini_selftest.sh`（POSIX 适配）第 1 次 / 第 2 次 | 79/79 通过；权限宏注入探针两组通过；samgr 绑定 `-fsyntax-only -Werror` 通过 |
| `CALL_MANAGER_ROOT=… bash run_mini_cmsis.sh`（CMSIS 适配，默认对象池 sem 6 / mux 6 / swtmr 5）第 1 次 / 第 2 次 | 79/79 通过 |
| `SEM_LIMIT=1 MINI_TEST_FILTER=InitFailureRollsBack bash run_mini_cmsis.sh` | 1/1 通过 |
| `check_mini_layout.py` / `check_header_origin.py` / `vendor_files.py check` | 通过（38 个源文件；11631 次头文件包含全部在仓内解析，40 个 Mini 头优先） |
| `check_mini_rules.sh` / `check_auth_parity.py` / `check_mini_symbols.sh` | 通过；各自的负向用例均报错 |
| 产品形态编译（不带 `TELEPHONY_MINI_HOST_TEST`/`TELEPHONY_MINI_LOG_PRINTF`，CMSIS 适配 + LiteOS 桩 + samgr/utils_lite 头；另一组注入信任模型、`QUEUE_SIZE=8`、`SLOT_COUNT=1`） | 两组配置下 38 个源文件 `-fsyntax-only -Werror` 零告警 |
| `gn gen`（临时 GN 根，`//build/lite` 模板与三个依赖目标为桩） | 通过：`tel_call_manager_mini` 38 个源文件、全部 defines、三个依赖目标连通 |
| 变异测试：删除 `AddOneCallObject` 插入 / 去掉 Guard 批量上限 / `DialCall` 绕过白名单 | 分别有 5 个、1 个、1 个用例失败 |

## `AGENTS.md` 最小验证闭环（DoD），按 Mini 仓适配

| 步骤 | 条目 | 结果 |
|---|---|---|
| 1 静态自检 | 新增源文件、include 目录进入 `callmanager_mini.gni` | 已验证（`check_mini_layout.py`） |
| 1 | 新增 IPC 接口六处改动 | 不适用：Mini 不新增 IPC 接口 |
| 1 | 构建参数同时在 `declare_args()` 与 `bundle.json` `features` | 已验证（13 个参数） |
| 1 | `#ifdef` 代码在宏关闭时仍能编译 | 已验证（产品形态两组配置编译） |
| 2 构建 | OpenHarmony `build.sh` 构建 | 未验证：无 OHOS 源码树；以 host 门禁、`gn gen` 桩与产品形态 `-fsyntax-only` 代替 |
| 3 单测 | zero_gtest | 不适用：Mini 使用 `test/mini` 自测（两个门禁各 79 例） |
| 4 DoD | 最小复现场景 | 已验证（每个 spec 场景一个用例，名称与注释对应） |
| 4 | 每个分支至少一条用例 | 已验证 |
| 4 | `high-risk.md` 变更风险矩阵回归面 | 标准仓零改动，标准侧回归面不受影响；Mini 侧高危序列由 4.8 的 9 个用例覆盖 |
| 4 | 标志位置位与清位成对 | 已验证：拨号处理标志（本地拨号记录建立、策略失败、本地上报失败、3 s 任务四条清位路径）、待定拨号、待定挂断（`ClearPendingState`、`HandleDialWhenHolding`、30 s 保护任务） |
| 4 | 日志无隐私字段、无高频 info 日志 | 已验证（`check_mini_rules.sh` 第 4 项；info 日志只在状态迁移时输出） |

## `docs/agents/build-and-test.md` 静态自检 13 项

| # | 结果 |
|---|---|
| 1 新增 `.cpp` 进源文件表 | 已验证 |
| 2 新增 include 目录 | 已验证 |
| 3 外部依赖同时进 gni 与 `bundle.json` | 已验证：kernel、samgr_lite、utils_lite、bounds_checking_function、init、core_service |
| 4 feature 宏三处齐全 | 已验证 |
| 5 `#ifdef` 内成员的引用点在同一宏内 | 已验证（`TELEPHONY_MINI_HOST_TEST` 专用接口仅被自测引用；产品形态编译通过） |
| 6 IPC 六步 | 不适用 |
| 7 proxy 写入与 stub 读取一致 | 已验证（`DialCall` 白名单 10 个字段、缺省值与上游 `OnDialCall` 逐一对照） |
| 8 无 `try`/`catch`/`dynamic_cast` | 已验证 |
| 9 日志不打印号码/联系人 | 已验证 |
| 10 新增标志位置位与清位 | 已验证（见上） |
| 11 `sptr<CallBase>` 使用点判空 | 已验证（逐个审查 `GetOneCallObject*`/`GetRingCall`/`GetIncomingCall` 调用点） |
| 12 加锁区间内不回调外部函数 | 已验证：真实内核锁只有事件处理器状态锁与注册表自旋锁，持有时不调用业务或回调；`ffrt::mutex` 在 Mini 上不占内核对象 |
| 13 `.d.ts` 签名 | 不适用：未改动 |

## `AGENTS_71.md` 第 7 节完成检查

| 条目 | 结果 |
|---|---|
| 功能分类、设计依据、差异和验证方式已记录，依赖闭包已检查 | 已验证（design 分类表、D2 缺陷与偏离、头文件来源检查、符号检查） |
| 未擅自扩大裁剪范围 | 已验证：范围外方法与 proposal/specs 一致；`RejectCall(RejectType)` 需未接来电通知，按范围外处理（D6 表未列） |
| 标准实现、目录、符号和控制流尽量保留 | 已验证（26 个拷贝文件逐字节一致；移植文件与标准同名同函数） |
| 标准与 Mini 边界清晰，平台差异集中 | 已验证（`src/mini`、`include/mini`、`utils/mini`、`interfaces/innerkits/mini`） |
| 非 Mini 构建行为未改变 | 已验证（标准仓 `git status` 为空） |
| 运行形态符合设计 | 部分验证：samgr 绑定已按上游头文件编译检查；未在 samgr_lite 运行时上执行（无目标板） |
| 不存在假成功 | 已验证（范围外返回 `CALL_ERR_FUNCTION_NOT_SUPPORTED`/`CALL_ERR_VIDEO_NOT_SUPPORTED`；未注册适配显式失败） |
| IPC 非传输职责未丢失 | 已验证（Guard 输入校验、白名单、号码长度、bundle 名来源、回调死亡路径） |
| C 包装与数据结构布局 | 不适用：本变更不提供 C 接口；samgr 默认 Feature API 仅用于按名发现服务 |
| 输入、长度与类型转换 | 已验证（Guard、号码 255 上限、PacMap 上限、配置 `static_assert`） |
| Client 未绕过 Service 线程模型 | 已验证（全部经主循环；主循环限定计数用例） |
| 并发、回调、同步超时、队列、锁与执行上下文 | 已验证（事件处理器 15 例、同步超时、回调内接听） |
| 热路径有界非阻塞 | 已验证（有界队列、非阻塞投递、队列满返回错误） |
| 资源预算 | 内核对象已验证（预算用例、多路通话对象数不变）；任务栈、ROM、堆峰值未验证：需目标板（7.6/7.7） |
| 多步失败与重试，超时任务不提交失效状态 | 已验证（拨号失败、初始化回滚、超时不写调用方内存、延迟任务找不到已移除通话） |
| 平台能力收敛到适配层，第三方源码未改 | 已验证 |
| 身份替换未扩大调用面，敏感数据未泄漏 | 已验证（默认拒绝、鉴权比对）；信任粒度需人工复核 |
| 裁剪功能未进入 Mini 运行路径 | 已验证（`check_mini_symbols.sh`） |
| 初始化、故障、超时、退出无泄漏或悬空访问 | host 上已验证（对象计数恢复、引用计数、Guard 在退出后被拒）；目标板未验证 |
| Mini、非 Mini、关闭可选能力的配置可构建 | Mini host 与产品形态已验证；非 Mini 未构建（无 OHOS 树，标准仓未改） |
| 规格数值落实到调用点，安全选项未关闭 | 已验证（`DefaultConfig`/`GetTaskConfig` 使用配置常量；`check_mini_rules.sh` 第 5 项） |
| 测试覆盖 | 保留语义、降级、失败、边界、裁剪无残留已覆盖；非 Mini 回归未执行（标准仓未改） |

## 需人工复核

- D9 信任模型：信任粒度为“整个映像内的调用方”，由构建期配置授予（`AGENTS.md` Ask before：权限判定逻辑）。
- D2 移植复用：逐函数对照 `src/X.cpp` 与 `src/mini/X.cpp`，重点复核 design D2 列出的上游缺陷处理与有意偏离。
- ECC 路径（`AGENTS.md` Ask before）：保留槽位、紧急呼叫豁免“第二次拨号”检查，以及 `EccDialPolicy` 挂断其余通话。
- D10 音频降级：唯一不复用标准控制流的模块。
- `bundle.json`：新部件的依赖名取自上游 BUILD.gn/bundle.json，未在 OHOS lite 产品中构建；`rom`/`ram` 待 7.7 实测后填写。
- 拷贝文件同步策略：`vendor_files.py sync` 刷新后需重跑三个检查脚本与两个门禁。
- 上游缺陷应向标准仓反馈：3123e36a 删除了 `AddOneCallObject` 的插入语句等五处（design D2）。

## 未执行项

- 7.6 主循环任务栈水位、7.7 ROM 与堆峰值：需目标板，未执行。
- OpenHarmony 整仓构建与真实 samgr_lite 运行：无 OHOS 源码树，未执行。
