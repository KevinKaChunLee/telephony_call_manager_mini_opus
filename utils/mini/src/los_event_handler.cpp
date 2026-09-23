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

#include "los_event_handler.h"

#include <new>

#include "call_manager_mini_config.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
namespace {
constexpr uint32_t STATE_UNINIT = 0;
constexpr uint32_t STATE_RUNNING = 1;
constexpr uint32_t STATE_STOPPING = 2;
constexpr uint32_t MS_PER_SECOND = 1000;
constexpr uint32_t STOP_POLL_MS = 1;
// Stop waits this long for the loop and for synchronous callers still holding a waiter slot.
constexpr uint32_t STOP_WAIT_MS = MINI_SYNC_WAIT_MS + 1000;

enum class MessageKind : uint8_t {
    ASYNC_TASK,
    SYNC_CALL,
    TIMER_FIRE,
};

LosEventHandler g_mainLoop;

class MutexGuard {
public:
    explicit MutexGuard(TelOsHandle mutex) : mutex_(mutex)
    {
        TelOsMutexLock(mutex_);
    }
    ~MutexGuard()
    {
        TelOsMutexUnlock(mutex_);
    }
    MutexGuard(const MutexGuard &) = delete;
    MutexGuard &operator=(const MutexGuard &) = delete;

private:
    TelOsHandle mutex_;
};

// Signed distance handles the 32-bit tick wrap for any delay shorter than 2^31 ticks.
bool IsDue(uint32_t dueTick, uint32_t now)
{
    return static_cast<int32_t>(dueTick - now) <= 0;
}

uint32_t TicksToMs(uint32_t ticks)
{
    uint64_t freq = TelOsGetTickFreq();
    if (freq == 0) {
        return ticks;
    }
    uint64_t ms = (static_cast<uint64_t>(ticks) * MS_PER_SECOND + freq - 1) / freq;
    return (ms >= TEL_OS_WAIT_FOREVER) ? (TEL_OS_WAIT_FOREVER - 1) : static_cast<uint32_t>(ms);
}
} // namespace

class LosEventHandlerCall {
public:
    explicit LosEventHandlerCall(std::atomic<uint32_t> &active, const std::atomic<uint32_t> &state)
        : active_(active)
    {
        active_.fetch_add(1);
        running_ = (state.load() == STATE_RUNNING);
    }
    ~LosEventHandlerCall()
    {
        active_.fetch_sub(1);
    }
    LosEventHandlerCall(const LosEventHandlerCall &) = delete;
    LosEventHandlerCall &operator=(const LosEventHandlerCall &) = delete;
    bool Running() const
    {
        return running_;
    }

private:
    std::atomic<uint32_t> &active_;
    bool running_ = false;
};

struct LosEventHandler::SyncContext : public virtual RefBase {
    TelMiniTask task;
    uint32_t slot = 0;
    bool abandoned = false;
    bool done = false;
    int32_t status = TELEPHONY_SUCCESS;
};

struct LosEventMessage {
    MessageKind kind = MessageKind::ASYNC_TASK;
    TelMiniTask task;
    sptr<LosEventHandler::SyncContext> sync;
};

LosEventHandler::Config LosEventHandler::DefaultConfig()
{
    Config config;
    config.queueSize = MINI_QUEUE_SIZE;
    config.maxDelayedTasks = MINI_MAX_DELAYED_TASKS;
    config.maxSyncWaiters = MINI_MAX_SYNC_WAITERS;
    config.loopStackSize = MINI_TASK_STACK_SIZE;
    config.loopPriority = static_cast<uint16_t>(MINI_TASK_PRIORITY);
    config.loopName = "call_manager";
    return config;
}

LosEventHandler::~LosEventHandler()
{
    Stop();
}

int32_t LosEventHandler::Init(const Config &config)
{
    if (state_.load() == STATE_RUNNING) {
        return TELEPHONY_SUCCESS;
    }
    if (state_.load() != STATE_UNINIT || config.queueSize == 0 || config.maxDelayedTasks == 0 ||
        config.maxSyncWaiters == 0 || (config.poster == nullptr && config.loopName == nullptr)) {
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    config_ = config;
    timerMessage_ = new (std::nothrow) LosEventMessage;
    bool ok = (timerMessage_ != nullptr) && (TelOsMutexCreate(&lock_) == TelOsResult::OK);
    if (ok) {
        timerMessage_->kind = MessageKind::TIMER_FIRE;
        ok = (TelOsTimerCreate(TimerExpired, this, &timer_) == TelOsResult::OK);
    }
    for (uint32_t i = 0; ok && i < config_.maxSyncWaiters; i++) {
        TelOsHandle sem = nullptr;
        ok = (TelOsSemCreate(0, 1, &sem) == TelOsResult::OK);
        if (ok) {
            waiterSems_.push_back(sem);
            waiterInUse_.push_back(false);
        }
    }
    bool selfDriven = (config_.poster == nullptr);
    if (ok && selfDriven) {
        // One extra slot keeps the timer message postable when the queue is full of tasks.
        ring_.assign(config_.queueSize + 1, nullptr);
        ok = (TelOsSemCreate(0, config_.queueSize + 1, &queueSem_) == TelOsResult::OK);
    }
    if (!ok) {
        TELEPHONY_LOGE("event handler init failed: kernel objects exhausted");
        ReleaseObjects();
        return TELEPHONY_ERR_FAIL;
    }
    state_.store(STATE_RUNNING);
    if (selfDriven) {
        TelOsTaskParam param;
        param.name = config_.loopName;
        param.entry = LoopEntry;
        param.arg = this;
        param.stackSize = config_.loopStackSize;
        param.priority = config_.loopPriority;
        loopRunning_.store(true);
        if (TelOsTaskCreate(param) != TelOsResult::OK) {
            TELEPHONY_LOGE("event handler init failed: no loop task");
            loopRunning_.store(false);
            state_.store(STATE_UNINIT);
            ReleaseObjects();
            return TELEPHONY_ERR_FAIL;
        }
    }
    return TELEPHONY_SUCCESS;
}

void LosEventHandler::Stop()
{
    uint32_t expected = STATE_RUNNING;
    if (!state_.compare_exchange_strong(expected, STATE_STOPPING)) {
        return;
    }
    bool selfDriven = (config_.poster == nullptr);
    if (selfDriven && IsOnLoop()) {
        TELEPHONY_LOGE("a self-driven event handler cannot be stopped from its own loop");
        state_.store(STATE_RUNNING);
        return;
    }
    TelOsTimerStop(timer_);
    std::list<DelayedTask> dropped;
    {
        MutexGuard guard(lock_);
        dropped.swap(delayed_);
    }
    dropped.clear();
    if (selfDriven) {
        TelOsSemPost(queueSem_);
        if (!WaitUntil([this]() { return !loopRunning_.load(); }, STOP_WAIT_MS)) {
            TELEPHONY_LOGE("event handler loop did not exit; its kernel objects are kept");
            return;
        }
    } else if (!IsOnLoop()) {
        WaitUntil([this]() { return dispatching_.load() == 0; }, STOP_WAIT_MS);
    }
    bool quiescent = WaitUntil([this]() {
        if (activeCalls_.load() != 0 || timerMessageQueued_.load()) {
            return false;
        }
        MutexGuard guard(lock_);
        for (bool inUse : waiterInUse_) {
            if (inUse) {
                return false;
            }
        }
        return true;
    }, STOP_WAIT_MS);
    if (!quiescent) {
        // A waiter, a poster or a queued timer message could still reach these objects.
        TELEPHONY_LOGE("event handler still referenced; its kernel objects are kept");
        return;
    }
    ReleaseObjects();
    state_.store(STATE_UNINIT);
}

bool LosEventHandler::IsRunning() const
{
    return state_.load() == STATE_RUNNING;
}

void LosEventHandler::BindLoopTask()
{
    loopTaskId_.store(TelOsGetCurrentTaskId());
}

bool LosEventHandler::IsOnLoop() const
{
    TelOsTaskId loop = loopTaskId_.load();
    return loop != 0 && loop == TelOsGetCurrentTaskId();
}

void LosEventHandler::Dispatch(LosEventMessage *message)
{
    if (message == nullptr) {
        return;
    }
    dispatching_.fetch_add(1);
    if (state_.load() != STATE_RUNNING) {
        Recycle(message);
    } else if (message->kind == MessageKind::TIMER_FIRE) {
        timerMessageQueued_.store(false);
        RunDueTasks();
    } else if (message->kind == MessageKind::SYNC_CALL) {
        sptr<SyncContext> context = message->sync;
        delete message;
        if (context->task) {
            context->task();
        }
        Complete(context, TELEPHONY_SUCCESS);
    } else {
        TelMiniTask task = std::move(message->task);
        delete message;
        if (task) {
            task();
        }
    }
    dispatching_.fetch_sub(1);
}

int32_t LosEventHandler::PostAsyncTask(TelMiniTask task, uint32_t delayMs, TelMiniTaskId *taskId)
{
    LosEventHandlerCall call(activeCalls_, state_);
    if (!call.Running()) {
        return TELEPHONY_ERR_UNINIT;
    }
    if (delayMs == 0) {
        LosEventMessage *message = new (std::nothrow) LosEventMessage;
        if (message == nullptr) {
            return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
        }
        message->task = std::move(task);
        int32_t ret = Enqueue(message);
        if (ret != TELEPHONY_SUCCESS) {
            delete message;
        }
        return ret;
    }
    uint32_t ticks = TelOsMsToTicks(delayMs);
    MutexGuard guard(lock_);
    if (delayed_.size() >= config_.maxDelayedTasks) {
        TELEPHONY_LOGE("delayed task table full (%{public}u)", config_.maxDelayedTasks);
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    if (++lastTaskId_ == TEL_MINI_INVALID_TASK_ID) {
        ++lastTaskId_;
    }
    DelayedTask entry;
    entry.id = lastTaskId_;
    entry.dueTick = TelOsGetTickCount() + ((ticks == 0) ? 1 : ticks);
    entry.task = std::move(task);
    auto pos = delayed_.begin();
    while (pos != delayed_.end() && !IsDue(entry.dueTick, pos->dueTick)) {
        ++pos;
    }
    while (pos != delayed_.end() && pos->dueTick == entry.dueTick) {
        ++pos;
    }
    bool earliest = (pos == delayed_.begin());
    delayed_.insert(pos, std::move(entry));
    if (earliest) {
        ArmTimerLocked();
    }
    if (taskId != nullptr) {
        *taskId = lastTaskId_;
    }
    return TELEPHONY_SUCCESS;
}

bool LosEventHandler::RemoveAsyncTask(TelMiniTaskId taskId)
{
    LosEventHandlerCall call(activeCalls_, state_);
    if (taskId == TEL_MINI_INVALID_TASK_ID || !call.Running()) {
        return false;
    }
    TelMiniTask removed;
    {
        MutexGuard guard(lock_);
        for (auto it = delayed_.begin(); it != delayed_.end(); ++it) {
            if (it->id == taskId) {
                bool wasEarliest = (it == delayed_.begin());
                removed = std::move(it->task);
                delayed_.erase(it);
                if (wasEarliest) {
                    ArmTimerLocked();
                }
                return true;
            }
        }
    }
    return false;
}

int32_t LosEventHandler::PostSyncTask(TelMiniTask task, uint32_t timeoutMs)
{
    LosEventHandlerCall call(activeCalls_, state_);
    if (!call.Running()) {
        return TELEPHONY_ERR_UNINIT;
    }
    if (IsOnLoop()) {
        if (task) {
            task();
        }
        return TELEPHONY_SUCCESS;
    }
    int32_t slot = AcquireWaiter();
    if (slot < 0) {
        TELEPHONY_LOGE("no free synchronous waiter slot");
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    sptr<SyncContext> context = new (std::nothrow) SyncContext;
    LosEventMessage *message = new (std::nothrow) LosEventMessage;
    if (context == nullptr || message == nullptr) {
        delete message;
        MutexGuard guard(lock_);
        waiterInUse_[slot] = false;
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    context->task = std::move(task);
    context->slot = static_cast<uint32_t>(slot);
    message->kind = MessageKind::SYNC_CALL;
    message->sync = context;
    int32_t ret = Enqueue(message);
    if (ret != TELEPHONY_SUCCESS) {
        delete message;
        MutexGuard guard(lock_);
        waiterInUse_[slot] = false;
        return ret;
    }
    TelOsResult waited = TelOsSemWait(waiterSems_[slot], timeoutMs);
    MutexGuard guard(lock_);
    if (waited == TelOsResult::OK || context->done) {
        if (waited != TelOsResult::OK) {
            // Completed between the timeout and taking the lock: consume the signal it left.
            TelOsSemWait(waiterSems_[slot], 0);
        }
        waiterInUse_[slot] = false;
        return context->status;
    }
    context->abandoned = true;
    TELEPHONY_LOGE("synchronous call timed out after %{public}u ms", timeoutMs);
    return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
}

void LosEventHandler::LoopEntry(void *arg)
{
    static_cast<LosEventHandler *>(arg)->RunLoop();
}

void LosEventHandler::TimerExpired(void *arg)
{
    static_cast<LosEventHandler *>(arg)->OnTimerExpired();
}

void LosEventHandler::RunLoop()
{
    BindLoopTask();
    while (true) {
        TelOsSemWait(queueSem_, TEL_OS_WAIT_FOREVER);
        LosEventMessage *message = nullptr;
        bool stopping = false;
        {
            MutexGuard guard(lock_);
            message = PopLocked();
            stopping = (state_.load() != STATE_RUNNING);
        }
        if (message != nullptr) {
            Dispatch(message);
        }
        if (stopping) {
            break;
        }
    }
    while (true) {
        LosEventMessage *message = nullptr;
        {
            MutexGuard guard(lock_);
            message = PopLocked();
        }
        if (message == nullptr) {
            break;
        }
        Recycle(message);
    }
    loopTaskId_.store(0);
    loopRunning_.store(false);
}

int32_t LosEventHandler::Enqueue(LosEventMessage *message)
{
    if (state_.load() != STATE_RUNNING) {
        return TELEPHONY_ERR_UNINIT;
    }
    if (config_.poster != nullptr) {
        return config_.poster(message, config_.posterContext) ? TELEPHONY_SUCCESS :
            TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    {
        MutexGuard guard(lock_);
        uint32_t capacity = static_cast<uint32_t>(ring_.size());
        uint32_t limit = (message == timerMessage_) ? capacity : config_.queueSize;
        if (ringCount_ >= limit) {
            return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
        }
        ring_[(ringHead_ + ringCount_) % capacity] = message;
        ringCount_++;
    }
    TelOsSemPost(queueSem_);
    return TELEPHONY_SUCCESS;
}

LosEventMessage *LosEventHandler::PopLocked()
{
    if (ringCount_ == 0) {
        return nullptr;
    }
    LosEventMessage *message = ring_[ringHead_];
    ring_[ringHead_] = nullptr;
    ringHead_ = (ringHead_ + 1) % static_cast<uint32_t>(ring_.size());
    ringCount_--;
    return message;
}

void LosEventHandler::Recycle(LosEventMessage *message)
{
    if (message == timerMessage_) {
        timerMessageQueued_.store(false);
        return;
    }
    if (message->kind == MessageKind::SYNC_CALL && message->sync != nullptr) {
        sptr<SyncContext> context = message->sync;
        delete message;
        Complete(context, TELEPHONY_ERR_UNINIT);
        return;
    }
    delete message;
}

void LosEventHandler::Complete(const sptr<SyncContext> &context, int32_t status)
{
    MutexGuard guard(lock_);
    context->status = status;
    context->done = true;
    if (context->abandoned) {
        waiterInUse_[context->slot] = false;
    } else {
        TelOsSemPost(waiterSems_[context->slot]);
    }
}

int32_t LosEventHandler::AcquireWaiter()
{
    MutexGuard guard(lock_);
    for (size_t i = 0; i < waiterInUse_.size(); i++) {
        if (!waiterInUse_[i]) {
            waiterInUse_[i] = true;
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

void LosEventHandler::ArmTimerLocked()
{
    if (delayed_.empty()) {
        TelOsTimerStop(timer_);
        return;
    }
    uint32_t now = TelOsGetTickCount();
    uint32_t dueTick = delayed_.front().dueTick;
    uint32_t ticks = IsDue(dueTick, now) ? 1 : (dueTick - now);
    TelOsTimerStart(timer_, TicksToMs(ticks));
}

void LosEventHandler::OnTimerExpired()
{
    LosEventHandlerCall call(activeCalls_, state_);
    if (!call.Running() || timerMessageQueued_.exchange(true)) {
        return;
    }
    if (Enqueue(timerMessage_) != TELEPHONY_SUCCESS) {
        timerMessageQueued_.store(false);
        TelOsTimerStart(timer_, TicksToMs(1));
    }
}

void LosEventHandler::RunDueTasks()
{
    std::list<DelayedTask> due;
    {
        MutexGuard guard(lock_);
        uint32_t now = TelOsGetTickCount();
        while (!delayed_.empty() && IsDue(delayed_.front().dueTick, now)) {
            due.splice(due.end(), delayed_, delayed_.begin());
        }
        ArmTimerLocked();
    }
    for (DelayedTask &entry : due) {
        if (entry.task) {
            entry.task();
        }
    }
}

bool LosEventHandler::WaitUntil(const std::function<bool()> &done, uint32_t timeoutMs)
{
    for (uint32_t waited = 0; waited < timeoutMs; waited += STOP_POLL_MS) {
        if (done()) {
            return true;
        }
        TelOsTaskDelayMs(STOP_POLL_MS);
    }
    return done();
}

void LosEventHandler::ReleaseObjects()
{
    if (timer_ != nullptr) {
        TelOsTimerDelete(timer_);
        timer_ = nullptr;
    }
    for (TelOsHandle sem : waiterSems_) {
        TelOsSemDelete(sem);
    }
    waiterSems_.clear();
    waiterInUse_.clear();
    if (queueSem_ != nullptr) {
        TelOsSemDelete(queueSem_);
        queueSem_ = nullptr;
    }
    ring_.clear();
    ringHead_ = 0;
    ringCount_ = 0;
    if (lock_ != nullptr) {
        TelOsMutexDelete(lock_);
        lock_ = nullptr;
    }
    delete timerMessage_;
    timerMessage_ = nullptr;
    timerMessageQueued_.store(false);
}

LosEventHandler &TelMiniMainLoop()
{
    return g_mainLoop;
}

bool TelMiniIsOnMainLoop()
{
    return g_mainLoop.IsOnLoop();
}
} // namespace Telephony
} // namespace OHOS
