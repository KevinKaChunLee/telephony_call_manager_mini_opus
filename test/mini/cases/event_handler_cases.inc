/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// LosEventHandler cases. Scenario names in the comments are the ones in specs/mini-call-runtime.
// Every case runs unchanged over both kernel adapters; the pool-exhaustion part of
// InitFailureRollsBackEveryObject needs the bounded CMSIS pools and is a no-op on POSIX.

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "call_manager_mini_config.h"
#include "los_event_handler.h"
#include "tel_os_adapter.h"

namespace EventHandlerCases {
using namespace OHOS::Telephony;

constexpr uint32_t WAIT_MS = 2000;
constexpr uint32_t SHORT_SYNC_MS = 50;
// "Returns immediately": no wait on the loop, allowing for scheduling jitter on a loaded host.
constexpr uint32_t IMMEDIATE_MS = 30;
constexpr uint32_t POOL_PROBE_LIMIT = 32;
// Fewer rounds than MINI_MAX_DELAYED_TASKS, so the delayed table never fills up.
constexpr int BUDGET_ROUNDS = 5;

LosEventHandler::Config TestConfig()
{
    LosEventHandler::Config config;
    config.queueSize = 4;
    config.maxDelayedTasks = 2;
    config.maxSyncWaiters = 2;
    config.loopStackSize = 16384;
    config.loopPriority = 20;
    config.loopName = "mini_test_loop";
    return config;
}

bool WaitFor(const std::function<bool()> &done, uint32_t timeoutMs = WAIT_MS)
{
    constexpr uint32_t pollMs = 5;
    for (uint32_t waited = 0; waited < timeoutMs; waited += pollMs) {
        if (done()) {
            return true;
        }
        TelOsTaskDelayMs(pollMs);
    }
    return done();
}

bool SameCount(const TelOsObjectCount &a, const TelOsObjectCount &b)
{
    return a.mutexCount == b.mutexCount && a.semaphoreCount == b.semaphoreCount && a.timerCount == b.timerCount &&
        a.taskCount == b.taskCount;
}

std::atomic<int> g_destroyed { 0 };

struct DestroyProbe {
    ~DestroyProbe()
    {
        g_destroyed.fetch_add(1);
    }
};

// Keeps the loop inside one task until the test opens it (or WAIT_MS passes).
struct Gate {
    TelOsHandle sem = nullptr;
    std::atomic<bool> entered { false };

    bool Create()
    {
        entered.store(false);
        return TelOsSemCreate(0, 1, &sem) == TelOsResult::OK;
    }
    TelMiniTask Blocker()
    {
        return [this]() {
            entered.store(true);
            TelOsSemWait(sem, WAIT_MS);
        };
    }
    bool WaitEntered()
    {
        return WaitFor([this]() { return entered.load(); });
    }
    void Open()
    {
        TelOsSemPost(sem);
    }
    void Destroy()
    {
        TelOsSemDelete(sem);
        sem = nullptr;
    }
};

// Synchronous callers on their own tasks, so that several can wait on the loop at once.
struct SyncCaller {
    LosEventHandler *handler = nullptr;
    uint32_t timeoutMs = WAIT_MS;
    std::atomic<bool> started { false };
    std::atomic<bool> finished { false };
    std::atomic<int32_t> result { -1 };
};

void SyncCallerEntry(void *arg)
{
    SyncCaller *caller = static_cast<SyncCaller *>(arg);
    caller->started.store(true);
    caller->result.store(caller->handler->PostSyncTask([]() {}, caller->timeoutMs));
    caller->finished.store(true);
}

bool StartSyncCaller(SyncCaller &caller, LosEventHandler &handler, uint32_t timeoutMs = WAIT_MS)
{
    caller.handler = &handler;
    caller.timeoutMs = timeoutMs;
    TelOsTaskParam param;
    param.name = "mini_test_sync";
    param.entry = SyncCallerEntry;
    param.arg = &caller;
    param.stackSize = 16384;
    param.priority = 20;
    return TelOsTaskCreate(param) == TelOsResult::OK;
}

// Stand-in for the samgr_lite service task in externally driven mode.
struct ExternalDriver {
    LosEventHandler *handler = nullptr;
    TelOsHandle lock = nullptr;
    TelOsHandle pending = nullptr;
    std::vector<LosEventMessage *> queue;
    std::atomic<bool> refuse { false };
    std::atomic<bool> quit { false };
    std::atomic<bool> exited { false };
};

bool DriverPost(LosEventMessage *message, void *context)
{
    ExternalDriver *driver = static_cast<ExternalDriver *>(context);
    if (driver->refuse.load()) {
        return false;
    }
    TelOsMutexLock(driver->lock);
    driver->queue.push_back(message);
    TelOsMutexUnlock(driver->lock);
    TelOsSemPost(driver->pending);
    return true;
}

LosEventMessage *DriverPop(ExternalDriver &driver)
{
    LosEventMessage *message = nullptr;
    TelOsMutexLock(driver.lock);
    if (!driver.queue.empty()) {
        message = driver.queue.front();
        driver.queue.erase(driver.queue.begin());
    }
    TelOsMutexUnlock(driver.lock);
    return message;
}

void DriverEntry(void *arg)
{
    ExternalDriver *driver = static_cast<ExternalDriver *>(arg);
    driver->handler->BindLoopTask();
    constexpr uint32_t pollMs = 20;
    while (!driver->quit.load()) {
        if (TelOsSemWait(driver->pending, pollMs) == TelOsResult::OK) {
            driver->handler->Dispatch(DriverPop(*driver));
        }
    }
    for (LosEventMessage *message = DriverPop(*driver); message != nullptr; message = DriverPop(*driver)) {
        driver->handler->Dispatch(message);
    }
    driver->exited.store(true);
}

bool StartDriver(ExternalDriver &driver, LosEventHandler &handler)
{
    driver.handler = &handler;
    if (TelOsMutexCreate(&driver.lock) != TelOsResult::OK) {
        return false;
    }
    if (TelOsSemCreate(0, 64, &driver.pending) != TelOsResult::OK) {
        return false;
    }
    TelOsTaskParam param;
    param.name = "mini_test_driver";
    param.entry = DriverEntry;
    param.arg = &driver;
    param.stackSize = 16384;
    param.priority = 20;
    return TelOsTaskCreate(param) == TelOsResult::OK;
}

void StopDriver(ExternalDriver &driver)
{
    driver.quit.store(true);
    WaitFor([&driver]() { return driver.exited.load(); });
    TelOsSemDelete(driver.pending);
    TelOsMutexDelete(driver.lock);
}

void NoopTimer(void *) {}

// Takes every free object of one kind; POSIX pools are unbounded, which returns false.
struct PoolHog {
    std::vector<TelOsHandle> sems;
    std::vector<TelOsHandle> mutexes;
    std::vector<TelOsHandle> timers;

    bool TakeAllSems()
    {
        TelOsHandle handle = nullptr;
        while (sems.size() < POOL_PROBE_LIMIT && TelOsSemCreate(0, 1, &handle) == TelOsResult::OK) {
            sems.push_back(handle);
        }
        return sems.size() < POOL_PROBE_LIMIT;
    }
    bool TakeAllMutexes()
    {
        TelOsHandle handle = nullptr;
        while (mutexes.size() < POOL_PROBE_LIMIT && TelOsMutexCreate(&handle) == TelOsResult::OK) {
            mutexes.push_back(handle);
        }
        return mutexes.size() < POOL_PROBE_LIMIT;
    }
    bool TakeAllTimers()
    {
        TelOsHandle handle = nullptr;
        while (timers.size() < POOL_PROBE_LIMIT && TelOsTimerCreate(NoopTimer, nullptr, &handle) == TelOsResult::OK) {
            timers.push_back(handle);
        }
        return timers.size() < POOL_PROBE_LIMIT;
    }
    void GiveBackOneSem()
    {
        if (!sems.empty()) {
            TelOsSemDelete(sems.back());
            sems.pop_back();
        }
    }
    void Release()
    {
        for (TelOsHandle sem : sems) {
            TelOsSemDelete(sem);
        }
        for (TelOsHandle mutex : mutexes) {
            TelOsMutexDelete(mutex);
        }
        for (TelOsHandle timer : timers) {
            TelOsTimerDelete(timer);
        }
        sems.clear();
        mutexes.clear();
        timers.clear();
    }
};

bool TaskCountSettles(const TelOsObjectCount &expected)
{
    return WaitFor([&expected]() { return SameCount(TelOsGetObjectCount(), expected); });
}
} // namespace EventHandlerCases

MINI_TEST(EventHandler, InitValidatesConfigAndIsIdempotent)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    LosEventHandler handler;
    LosEventHandler::Config bad = TestConfig();
    bad.queueSize = 0;
    EXPECT_EQ(handler.Init(bad), TELEPHONY_ERR_ARGUMENT_INVALID);
    EXPECT_FALSE(handler.IsRunning());
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    TelOsObjectCount running = TelOsGetObjectCount();
    EXPECT_EQ(handler.Init(TestConfig()), TELEPHONY_SUCCESS);
    EXPECT_TRUE(SameCount(TelOsGetObjectCount(), running));
    EXPECT_TRUE(handler.IsRunning());
    handler.Stop();
}

// 其他任务的同步调用 + FIFO of posts from another task.
MINI_TEST(EventHandler, AsyncTasksRunInFifoOrderOnTheLoop)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    LosEventHandler handler;
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    std::vector<int> order;
    int onLoop = 0;
    for (int i = 1; i <= 3; i++) {
        EXPECT_EQ(handler.PostAsyncTask([&order, &onLoop, &handler, i]() {
            order.push_back(i);
            onLoop += handler.IsOnLoop() ? 1 : 0;
        }), TELEPHONY_SUCCESS);
    }
    EXPECT_EQ(handler.PostSyncTask([]() {}, WAIT_MS), TELEPHONY_SUCCESS);
    EXPECT_TRUE(order == std::vector<int>({ 1, 2, 3 }));
    EXPECT_EQ(onLoop, 3);
    EXPECT_FALSE(handler.IsOnLoop());
    int result = 0;
    EXPECT_EQ(handler.PostSyncCall<int>([]() { return 42; }, result, WAIT_MS), TELEPHONY_SUCCESS);
    EXPECT_EQ(result, 42);
    handler.Stop();
}

MINI_TEST(EventHandler, DelayedTasksRunInDueOrder)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    LosEventHandler handler;
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    std::vector<char> order;
    std::atomic<int> ran { 0 };
    std::atomic<uint32_t> lateTick { 0 };
    uint32_t begin = TelOsGetTickCount();
    EXPECT_EQ(handler.PostAsyncTask([&order, &ran, &lateTick]() {
        order.push_back('A');
        lateTick.store(TelOsGetTickCount());
        ran.fetch_add(1);
    }, 80), TELEPHONY_SUCCESS);
    EXPECT_EQ(handler.PostAsyncTask([&order, &ran]() {
        order.push_back('B');
        ran.fetch_add(1);
    }, 20), TELEPHONY_SUCCESS);
    EXPECT_TRUE(WaitFor([&ran]() { return ran.load() == 2; }));
    EXPECT_EQ(handler.PostSyncTask([]() {}, WAIT_MS), TELEPHONY_SUCCESS);
    EXPECT_TRUE(order == std::vector<char>({ 'B', 'A' }));
    EXPECT_TRUE(lateTick.load() - begin >= TelOsMsToTicks(80));
    handler.Stop();
}

// 到期前撤销
MINI_TEST(EventHandler, RemoveBeforeDueCancelsAndReleasesOnce)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    LosEventHandler handler;
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    g_destroyed.store(0);
    std::atomic<int> ran { 0 };
    TelMiniTaskId id = TEL_MINI_INVALID_TASK_ID;
    {
        std::shared_ptr<DestroyProbe> probe = std::make_shared<DestroyProbe>();
        EXPECT_EQ(handler.PostAsyncTask([probe, &ran]() { ran.fetch_add(1); }, 200, &id), TELEPHONY_SUCCESS);
    }
    EXPECT_TRUE(id != TEL_MINI_INVALID_TASK_ID);
    EXPECT_EQ(g_destroyed.load(), 0);
    EXPECT_TRUE(handler.RemoveAsyncTask(id));
    EXPECT_EQ(g_destroyed.load(), 1);
    EXPECT_FALSE(handler.RemoveAsyncTask(id));
    TelOsTaskDelayMs(300);
    EXPECT_EQ(handler.PostSyncTask([]() {}, WAIT_MS), TELEPHONY_SUCCESS);
    EXPECT_EQ(ran.load(), 0);
    EXPECT_EQ(g_destroyed.load(), 1);
    handler.Stop();
}

// 已出队后撤销
MINI_TEST(EventHandler, RemoveAfterDequeueFailsAndTaskCompletes)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    LosEventHandler handler;
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    Gate gate;
    ASSERT_TRUE(gate.Create());
    g_destroyed.store(0);
    std::atomic<int> ran { 0 };
    TelMiniTaskId id = TEL_MINI_INVALID_TASK_ID;
    {
        std::shared_ptr<DestroyProbe> probe = std::make_shared<DestroyProbe>();
        TelMiniTask blocker = gate.Blocker();
        EXPECT_EQ(handler.PostAsyncTask([probe, blocker, &ran]() {
            blocker();
            ran.fetch_add(1);
        }, 20, &id), TELEPHONY_SUCCESS);
    }
    EXPECT_TRUE(gate.WaitEntered());
    EXPECT_FALSE(handler.RemoveAsyncTask(id));
    EXPECT_EQ(g_destroyed.load(), 0);
    gate.Open();
    EXPECT_TRUE(WaitFor([&ran]() { return ran.load() == 1; }));
    EXPECT_EQ(handler.PostSyncTask([]() {}, WAIT_MS), TELEPHONY_SUCCESS);
    EXPECT_EQ(ran.load(), 1);
    EXPECT_EQ(g_destroyed.load(), 1);
    handler.Stop();
    gate.Destroy();
}

MINI_TEST(EventHandler, DelayedTableFullRejectsAndReleasesPayload)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    LosEventHandler handler;
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    g_destroyed.store(0);
    TelMiniTaskId first = TEL_MINI_INVALID_TASK_ID;
    TelMiniTaskId second = TEL_MINI_INVALID_TASK_ID;
    EXPECT_EQ(handler.PostAsyncTask([]() {}, 500, &first), TELEPHONY_SUCCESS);
    EXPECT_EQ(handler.PostAsyncTask([]() {}, 500, &second), TELEPHONY_SUCCESS);
    TelMiniTaskId rejected = TEL_MINI_INVALID_TASK_ID;
    {
        std::shared_ptr<DestroyProbe> probe = std::make_shared<DestroyProbe>();
        EXPECT_EQ(handler.PostAsyncTask([probe]() {}, 500, &rejected), TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    }
    EXPECT_EQ(g_destroyed.load(), 1);
    EXPECT_EQ(rejected, TEL_MINI_INVALID_TASK_ID);
    EXPECT_TRUE(first != second);
    EXPECT_TRUE(handler.RemoveAsyncTask(first));
    EXPECT_TRUE(handler.RemoveAsyncTask(second));
    handler.Stop();
}

// Blocking the loop past the limit: the caller returns on time, the task still runs and never
// writes the caller's result.
MINI_TEST(EventHandler, SyncTimeoutReturnsOnTimeWithoutWritingCaller)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    LosEventHandler handler;
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    Gate gate;
    ASSERT_TRUE(gate.Create());
    EXPECT_EQ(handler.PostAsyncTask(gate.Blocker()), TELEPHONY_SUCCESS);
    EXPECT_TRUE(gate.WaitEntered());
    int result = -1;
    std::atomic<int> ran { 0 };
    uint32_t begin = TelOsGetTickCount();
    EXPECT_EQ(handler.PostSyncCall<int>([&ran]() {
        ran.fetch_add(1);
        return 7;
    }, result, SHORT_SYNC_MS), TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    uint32_t elapsed = TelOsGetTickCount() - begin;
    EXPECT_TRUE(elapsed + 1 >= TelOsMsToTicks(SHORT_SYNC_MS));
    EXPECT_TRUE(elapsed <= TelOsMsToTicks(SHORT_SYNC_MS) + TelOsMsToTicks(200));
    EXPECT_EQ(result, -1);
    gate.Open();
    EXPECT_TRUE(WaitFor([&ran]() { return ran.load() == 1; }));
    EXPECT_EQ(handler.PostSyncTask([]() {}, WAIT_MS), TELEPHONY_SUCCESS);
    EXPECT_EQ(result, -1);
    handler.Stop();
    gate.Destroy();
}

// A slot whose previous caller gave up must not wake its next caller early.
MINI_TEST(EventHandler, AbandonedSlotDoesNotWakeNextCall)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    LosEventHandler handler;
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    Gate gate;
    ASSERT_TRUE(gate.Create());
    std::atomic<int> ran { 0 };
    TelMiniTask count = [&ran]() { ran.fetch_add(1); };
    EXPECT_EQ(handler.PostAsyncTask(gate.Blocker()), TELEPHONY_SUCCESS);
    EXPECT_TRUE(gate.WaitEntered());
    EXPECT_EQ(handler.PostSyncTask(count, SHORT_SYNC_MS), TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    gate.Open();
    EXPECT_TRUE(WaitFor([&ran]() { return ran.load() == 1; }));

    gate.entered.store(false);
    EXPECT_EQ(handler.PostAsyncTask(gate.Blocker()), TELEPHONY_SUCCESS);
    EXPECT_TRUE(gate.WaitEntered());
    uint32_t begin = TelOsGetTickCount();
    EXPECT_EQ(handler.PostSyncTask(count, SHORT_SYNC_MS), TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    EXPECT_TRUE(TelOsGetTickCount() - begin + 1 >= TelOsMsToTicks(SHORT_SYNC_MS));
    gate.Open();
    EXPECT_TRUE(WaitFor([&ran]() { return ran.load() == 2; }));
    EXPECT_EQ(handler.PostSyncTask(count, WAIT_MS), TELEPHONY_SUCCESS);
    EXPECT_EQ(ran.load(), 3);
    handler.Stop();
    gate.Destroy();
}

// 并发同步调用超出等待者上限
MINI_TEST(EventHandler, WaiterLimitFailsImmediately)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    LosEventHandler handler;
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    Gate gate;
    ASSERT_TRUE(gate.Create());
    EXPECT_EQ(handler.PostAsyncTask(gate.Blocker()), TELEPHONY_SUCCESS);
    EXPECT_TRUE(gate.WaitEntered());
    SyncCaller callers[2];
    for (SyncCaller &caller : callers) {
        EXPECT_TRUE(StartSyncCaller(caller, handler));
    }
    EXPECT_TRUE(WaitFor([&callers]() { return callers[0].started.load() && callers[1].started.load(); }));
    TelOsTaskDelayMs(50);
    TelOsObjectCount before = TelOsGetObjectCount();
    uint32_t begin = TelOsGetTickCount();
    EXPECT_EQ(handler.PostSyncTask([]() {}, WAIT_MS), TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    EXPECT_TRUE(TelOsGetTickCount() - begin <= TelOsMsToTicks(IMMEDIATE_MS));
    EXPECT_TRUE(SameCount(TelOsGetObjectCount(), before));
    gate.Open();
    EXPECT_TRUE(WaitFor([&callers]() { return callers[0].finished.load() && callers[1].finished.load(); }));
    EXPECT_EQ(callers[0].result.load(), TELEPHONY_SUCCESS);
    EXPECT_EQ(callers[1].result.load(), TELEPHONY_SUCCESS);
    handler.Stop();
    gate.Destroy();
}

// 主循环内的同步调用
MINI_TEST(EventHandler, SyncCallOnTheLoopRunsInPlace)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    LosEventHandler handler;
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    int32_t innerRet = -1;
    bool ranInPlace = false;
    EXPECT_EQ(handler.PostAsyncTask([&handler, &innerRet, &ranInPlace]() {
        bool ran = false;
        innerRet = handler.PostSyncTask([&ran]() { ran = true; }, SHORT_SYNC_MS);
        ranInPlace = ran;
    }), TELEPHONY_SUCCESS);
    EXPECT_EQ(handler.PostSyncTask([]() {}, WAIT_MS), TELEPHONY_SUCCESS);
    EXPECT_EQ(innerRet, TELEPHONY_SUCCESS);
    EXPECT_TRUE(ranInPlace);
    handler.Stop();
}

// 队列满
MINI_TEST(EventHandler, FullQueueRejectsAndReleasesPayload)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    LosEventHandler handler;
    LosEventHandler::Config config = TestConfig();
    ASSERT_TRUE(handler.Init(config) == TELEPHONY_SUCCESS);
    Gate gate;
    ASSERT_TRUE(gate.Create());
    EXPECT_EQ(handler.PostAsyncTask(gate.Blocker()), TELEPHONY_SUCCESS);
    EXPECT_TRUE(gate.WaitEntered());
    std::atomic<int> ran { 0 };
    for (uint32_t i = 0; i < config.queueSize; i++) {
        EXPECT_EQ(handler.PostAsyncTask([&ran]() { ran.fetch_add(1); }), TELEPHONY_SUCCESS);
    }
    g_destroyed.store(0);
    uint32_t begin = TelOsGetTickCount();
    {
        std::shared_ptr<DestroyProbe> probe = std::make_shared<DestroyProbe>();
        EXPECT_EQ(handler.PostAsyncTask([probe, &ran]() { ran.fetch_add(100); }),
            TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    }
    EXPECT_EQ(g_destroyed.load(), 1);
    EXPECT_EQ(handler.PostSyncTask([&ran]() { ran.fetch_add(100); }, WAIT_MS), TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    EXPECT_TRUE(TelOsGetTickCount() - begin <= TelOsMsToTicks(IMMEDIATE_MS));
    gate.Open();
    EXPECT_TRUE(WaitFor([&ran, &config]() { return ran.load() == static_cast<int>(config.queueSize); }));
    EXPECT_EQ(handler.PostSyncTask([]() {}, WAIT_MS), TELEPHONY_SUCCESS);
    EXPECT_EQ(ran.load(), static_cast<int>(config.queueSize));
    handler.Stop();
    gate.Destroy();
}

MINI_TEST(EventHandler, ExternalDriverCompletesSyncAndDelayedWork)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    ExternalDriver driver;
    LosEventHandler handler;
    LosEventHandler::Config config = TestConfig();
    config.loopName = nullptr;
    config.poster = DriverPost;
    config.posterContext = &driver;
    ASSERT_TRUE(handler.Init(config) == TELEPHONY_SUCCESS);
    ASSERT_TRUE(StartDriver(driver, handler));
    int result = 0;
    EXPECT_EQ(handler.PostSyncCall<int>([&handler]() { return handler.IsOnLoop() ? 42 : -42; }, result, WAIT_MS),
        TELEPHONY_SUCCESS);
    EXPECT_EQ(result, 42);
    EXPECT_FALSE(handler.IsOnLoop());
    std::atomic<int> delayedRan { 0 };
    EXPECT_EQ(handler.PostAsyncTask([&delayedRan]() { delayedRan.fetch_add(1); }, 20), TELEPHONY_SUCCESS);
    EXPECT_TRUE(WaitFor([&delayedRan]() { return delayedRan.load() == 1; }));

    driver.refuse.store(true);
    g_destroyed.store(0);
    {
        std::shared_ptr<DestroyProbe> probe = std::make_shared<DestroyProbe>();
        EXPECT_EQ(handler.PostAsyncTask([probe]() {}), TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    }
    EXPECT_EQ(g_destroyed.load(), 1);
    EXPECT_EQ(handler.PostSyncTask([]() {}, WAIT_MS), TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    driver.refuse.store(false);
    handler.Stop();
    EXPECT_FALSE(handler.IsRunning());
    StopDriver(driver);
}

// 退出期间有请求到达 + 重复退出
MINI_TEST(EventHandler, StopRejectsPostsRecyclesAndIsRepeatable)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    TelOsObjectCount before = TelOsGetObjectCount();
    LosEventHandler handler;
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    std::atomic<bool> entered { false };
    std::atomic<int32_t> postDuringStop { -1 };
    EXPECT_EQ(handler.PostAsyncTask([&handler, &entered, &postDuringStop]() {
        entered.store(true);
        TelOsTaskDelayMs(150);
        postDuringStop.store(handler.PostAsyncTask([]() {}));
    }), TELEPHONY_SUCCESS);
    EXPECT_TRUE(WaitFor([&entered]() { return entered.load(); }));
    g_destroyed.store(0);
    std::atomic<int> ran { 0 };
    {
        std::shared_ptr<DestroyProbe> queued = std::make_shared<DestroyProbe>();
        std::shared_ptr<DestroyProbe> delayed = std::make_shared<DestroyProbe>();
        EXPECT_EQ(handler.PostAsyncTask([queued, &ran]() { ran.fetch_add(1); }), TELEPHONY_SUCCESS);
        EXPECT_EQ(handler.PostAsyncTask([delayed, &ran]() { ran.fetch_add(1); }, 1000), TELEPHONY_SUCCESS);
    }
    SyncCaller caller;
    EXPECT_TRUE(StartSyncCaller(caller, handler));
    EXPECT_TRUE(WaitFor([&caller]() { return caller.started.load(); }));
    TelOsTaskDelayMs(30);

    handler.Stop();
    EXPECT_FALSE(handler.IsRunning());
    EXPECT_EQ(postDuringStop.load(), TELEPHONY_ERR_UNINIT);
    EXPECT_EQ(ran.load(), 0);
    EXPECT_EQ(g_destroyed.load(), 2);
    EXPECT_TRUE(WaitFor([&caller]() { return caller.finished.load(); }));
    EXPECT_EQ(caller.result.load(), TELEPHONY_ERR_UNINIT);
    EXPECT_EQ(handler.PostAsyncTask([]() {}), TELEPHONY_ERR_UNINIT);
    EXPECT_EQ(handler.PostAsyncTask([]() {}, 10), TELEPHONY_ERR_UNINIT);
    EXPECT_EQ(handler.PostSyncTask([]() {}, WAIT_MS), TELEPHONY_ERR_UNINIT);
    EXPECT_FALSE(handler.RemoveAsyncTask(1));
    handler.Stop();
    EXPECT_TRUE(TaskCountSettles(before));
    ASSERT_TRUE(handler.Init(TestConfig()) == TELEPHONY_SUCCESS);
    EXPECT_EQ(handler.PostSyncTask([]() {}, WAIT_MS), TELEPHONY_SUCCESS);
    handler.Stop();
    EXPECT_TRUE(TaskCountSettles(before));
}

// 中途失败 at the kernel layer: SEM_LIMIT=1 bash run_mini_cmsis.sh runs this with a one-semaphore pool.
MINI_TEST(EventHandler, InitFailureRollsBackEveryObject)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    TelOsObjectCount before = TelOsGetObjectCount();
    PoolHog hog;
    if (!hog.TakeAllSems()) {
        hog.Release();
        std::printf("    unbounded kernel pools: pool exhaustion is covered by the CMSIS gate\n");
        return;
    }
    size_t semPool = hog.sems.size();
    hog.GiveBackOneSem();
    TelOsObjectCount starved = TelOsGetObjectCount();
    LosEventHandler handler;
    EXPECT_EQ(handler.Init(TestConfig()), TELEPHONY_ERR_FAIL);
    EXPECT_FALSE(handler.IsRunning());
    EXPECT_TRUE(SameCount(TelOsGetObjectCount(), starved));
    EXPECT_EQ(handler.PostAsyncTask([]() {}), TELEPHONY_ERR_UNINIT);
    hog.Release();

    EXPECT_TRUE(hog.TakeAllMutexes());
    starved = TelOsGetObjectCount();
    EXPECT_EQ(handler.Init(TestConfig()), TELEPHONY_ERR_FAIL);
    EXPECT_TRUE(SameCount(TelOsGetObjectCount(), starved));
    hog.Release();

    EXPECT_TRUE(hog.TakeAllTimers());
    starved = TelOsGetObjectCount();
    EXPECT_EQ(handler.Init(TestConfig()), TELEPHONY_ERR_FAIL);
    EXPECT_TRUE(SameCount(TelOsGetObjectCount(), starved));
    hog.Release();
    EXPECT_TRUE(SameCount(TelOsGetObjectCount(), before));

    constexpr size_t semsForTestConfig = 3;
    if (semPool >= semsForTestConfig) {
        EXPECT_EQ(handler.Init(TestConfig()), TELEPHONY_SUCCESS);
        handler.Stop();
        EXPECT_TRUE(TaskCountSettles(before));
    }
}

// Budget table in design "资源所有权与预算", for LosEventHandler with the product sizes.
MINI_TEST(EventHandler, KernelObjectBudgetMatchesDesign)
{
    using namespace OHOS::Telephony;
    using namespace EventHandlerCases;
    TelOsObjectCount before = TelOsGetObjectCount();
    LosEventHandler selfDriven;
    ASSERT_TRUE(selfDriven.Init(LosEventHandler::DefaultConfig()) == TELEPHONY_SUCCESS);
    TelOsObjectCount running = TelOsGetObjectCount();
    EXPECT_EQ(running.mutexCount - before.mutexCount, 1U);
    EXPECT_EQ(running.semaphoreCount - before.semaphoreCount, MINI_MAX_SYNC_WAITERS + 1);
    EXPECT_EQ(running.timerCount - before.timerCount, 1U);
    EXPECT_EQ(running.taskCount - before.taskCount, 1U);
    std::atomic<int> ran { 0 };
    for (int i = 0; i < BUDGET_ROUNDS; i++) {
        EXPECT_EQ(selfDriven.PostSyncTask([&ran]() { ran.fetch_add(1); }, WAIT_MS), TELEPHONY_SUCCESS);
        EXPECT_EQ(selfDriven.PostAsyncTask([&ran]() { ran.fetch_add(1); }, 10), TELEPHONY_SUCCESS);
    }
    EXPECT_TRUE(WaitFor([&ran]() { return ran.load() == 2 * BUDGET_ROUNDS; }));
    EXPECT_TRUE(SameCount(TelOsGetObjectCount(), running));
    selfDriven.Stop();
    EXPECT_TRUE(TaskCountSettles(before));

    ExternalDriver driver;
    LosEventHandler external;
    LosEventHandler::Config config = LosEventHandler::DefaultConfig();
    config.loopName = nullptr;
    config.poster = DriverPost;
    config.posterContext = &driver;
    ASSERT_TRUE(external.Init(config) == TELEPHONY_SUCCESS);
    running = TelOsGetObjectCount();
    EXPECT_EQ(running.mutexCount - before.mutexCount, 1U);
    EXPECT_EQ(running.semaphoreCount - before.semaphoreCount, MINI_MAX_SYNC_WAITERS);
    EXPECT_EQ(running.timerCount - before.timerCount, 1U);
    EXPECT_EQ(running.taskCount - before.taskCount, 0U);
    ASSERT_TRUE(StartDriver(driver, external));
    TelOsObjectCount withDriver = TelOsGetObjectCount();
    ran.store(0);
    for (int i = 0; i < BUDGET_ROUNDS; i++) {
        EXPECT_EQ(external.PostSyncTask([&ran]() { ran.fetch_add(1); }, WAIT_MS), TELEPHONY_SUCCESS);
        EXPECT_EQ(external.PostAsyncTask([&ran]() { ran.fetch_add(1); }, 10), TELEPHONY_SUCCESS);
    }
    EXPECT_TRUE(WaitFor([&ran]() { return ran.load() == 2 * BUDGET_ROUNDS; }));
    EXPECT_TRUE(SameCount(TelOsGetObjectCount(), withDriver));
    external.Stop();
    StopDriver(driver);
    EXPECT_TRUE(TaskCountSettles(before));
}
