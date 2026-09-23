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

#ifndef TELEPHONY_MINI_LOS_EVENT_HANDLER_H
#define TELEPHONY_MINI_LOS_EVENT_HANDLER_H

#include "tel_mini_std_includes.h"

#include "refbase.h"
#include "tel_os_adapter.h"
#include "telephony_errors.h"

// The call manager's main loop. All business state is touched only by tasks it runs.
//
// Two ways to drive it:
//  - self-driven (Config::poster == nullptr): the handler owns a bounded ring and one loop task;
//  - externally driven: every message goes to Config::poster (the samgr_lite binding hands it to
//    SAMGR_SendRequest) and the owner of that task calls BindLoopTask() once and Dispatch() for each
//    message it receives.
//
// Posting never blocks. A synchronous post waits at most its timeout; a timeout only stops the
// wait, the task still runs later and its result lands in handler-owned storage. Messages are heap
// objects handed over by pointer: on success the loop owns them, on failure the poster frees them.
// Kernel objects are created by Init only: one mutex, one timer and maxSyncWaiters semaphores,
// plus one semaphore and one task when self-driven.

namespace OHOS {
namespace Telephony {
using TelMiniTask = std::function<void()>;
using TelMiniTaskId = uint32_t;
constexpr TelMiniTaskId TEL_MINI_INVALID_TASK_ID = 0;

struct LosEventMessage;

class LosEventHandler {
public:
    using Poster = bool (*)(LosEventMessage *message, void *context);

    struct Config {
        uint32_t queueSize = 0;
        uint32_t maxDelayedTasks = 0;
        uint32_t maxSyncWaiters = 0;
        uint32_t loopStackSize = 0;
        uint16_t loopPriority = 0;
        const char *loopName = nullptr;
        Poster poster = nullptr;
        void *posterContext = nullptr;
    };

    // Product sizes from call_manager_mini_config.h, self-driven.
    static Config DefaultConfig();

    LosEventHandler() = default;
    ~LosEventHandler();
    LosEventHandler(const LosEventHandler &) = delete;
    LosEventHandler &operator=(const LosEventHandler &) = delete;

    // Idempotent while running; on failure every kernel object created so far is released.
    int32_t Init(const Config &config);
    // Rejects new posts, stops the timer, lets the running task finish and recycles what is left.
    // Safe to repeat. A self-driven handler must not be stopped from its own loop.
    void Stop();
    bool IsRunning() const;

    void BindLoopTask();
    bool IsOnLoop() const;
    void Dispatch(LosEventMessage *message);

    // delayMs == 0 queues immediately (such tasks cannot be removed); otherwise the task waits in
    // the delayed table and taskId receives a handle for RemoveAsyncTask.
    int32_t PostAsyncTask(TelMiniTask task, uint32_t delayMs = 0, TelMiniTaskId *taskId = nullptr);
    // Removes a delayed task that has not been taken out for execution yet.
    bool RemoveAsyncTask(TelMiniTaskId taskId);
    // Runs task on the loop and waits for it; runs it in place when already on the loop.
    // Returns TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL on timeout, a full queue or no free waiter slot,
    // and TELEPHONY_ERR_UNINIT when the handler is not running.
    int32_t PostSyncTask(TelMiniTask task, uint32_t timeoutMs);

    template <typename R>
    int32_t PostSyncCall(std::function<R()> call, R &result, uint32_t timeoutMs)
    {
        std::shared_ptr<R> holder = std::make_shared<R>();
        int32_t ret = PostSyncTask([call, holder]() { *holder = call(); }, timeoutMs);
        if (ret == TELEPHONY_SUCCESS) {
            result = *holder;
        }
        return ret;
    }

private:
    friend struct LosEventMessage;
    struct SyncContext;
    struct DelayedTask {
        TelMiniTaskId id = TEL_MINI_INVALID_TASK_ID;
        uint32_t dueTick = 0;
        TelMiniTask task;
    };

    static void LoopEntry(void *arg);
    static void TimerExpired(void *arg);
    void RunLoop();
    int32_t Enqueue(LosEventMessage *message);
    LosEventMessage *PopLocked();
    void Recycle(LosEventMessage *message);
    void Complete(const sptr<SyncContext> &context, int32_t status);
    int32_t AcquireWaiter();
    void ArmTimerLocked();
    void OnTimerExpired();
    void RunDueTasks();
    bool WaitUntil(const std::function<bool()> &done, uint32_t timeoutMs);
    void ReleaseObjects();

    Config config_;
    std::atomic<uint32_t> state_ { 0 };
    TelOsHandle lock_ = nullptr;
    TelOsHandle timer_ = nullptr;
    TelOsHandle queueSem_ = nullptr;
    std::vector<TelOsHandle> waiterSems_;
    std::vector<bool> waiterInUse_;
    std::vector<LosEventMessage *> ring_;
    uint32_t ringHead_ = 0;
    uint32_t ringCount_ = 0;
    std::list<DelayedTask> delayed_;
    TelMiniTaskId lastTaskId_ = TEL_MINI_INVALID_TASK_ID;
    LosEventMessage *timerMessage_ = nullptr;
    std::atomic<bool> timerMessageQueued_ { false };
    std::atomic<TelOsTaskId> loopTaskId_ { 0 };
    std::atomic<bool> loopRunning_ { false };
    std::atomic<uint32_t> dispatching_ { 0 };
    // Posting calls between their state check and return; Stop frees nothing while any is active.
    std::atomic<uint32_t> activeCalls_ { 0 };
};

// The instance the call manager runs on.
LosEventHandler &TelMiniMainLoop();
bool TelMiniIsOnMainLoop();
} // namespace Telephony
} // namespace OHOS
#endif // TELEPHONY_MINI_LOS_EVENT_HANDLER_H
