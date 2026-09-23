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

#include "tel_os_adapter.h"

#include <atomic>
#include <cerrno>
#include <ctime>
#include <new>
#include <pthread.h>
#include <sched.h>

namespace OHOS {
namespace Telephony {
namespace {
#ifdef LOSCFG_BASE_CORE_TICK_PER_SECOND
constexpr uint32_t POSIX_TICK_PER_SECOND = LOSCFG_BASE_CORE_TICK_PER_SECOND;
#else
constexpr uint32_t POSIX_TICK_PER_SECOND = 100;
#endif
constexpr int64_t NS_PER_SECOND = 1000000000LL;
// Host threads run glibc's printf and the self-test framework, which the target task never
// does; the product stack size is verified on the board, not here.
constexpr uint32_t HOST_MIN_TASK_STACK = 256 * 1024;

std::atomic<uint32_t> g_mutexCount { 0 };
std::atomic<uint32_t> g_semCount { 0 };
std::atomic<uint32_t> g_timerCount { 0 };
std::atomic<uint32_t> g_taskCount { 0 };
std::atomic<TelOsTaskId> g_nextTaskId { 0 };

int64_t MonotonicNs()
{
    timespec now {};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<int64_t>(now.tv_sec) * NS_PER_SECOND + now.tv_nsec;
}

timespec ToTimespec(int64_t ns)
{
    timespec ts {};
    ts.tv_sec = static_cast<time_t>(ns / NS_PER_SECOND);
    ts.tv_nsec = static_cast<long>(ns % NS_PER_SECOND);
    return ts;
}

int64_t TicksToNs(uint32_t ticks)
{
    return static_cast<int64_t>(ticks) * NS_PER_SECOND / POSIX_TICK_PER_SECOND;
}

bool InitMonotonicCond(pthread_cond_t *cond)
{
    pthread_condattr_t attr;
    if (pthread_condattr_init(&attr) != 0) {
        return false;
    }
    bool ok = (pthread_condattr_setclock(&attr, CLOCK_MONOTONIC) == 0) && (pthread_cond_init(cond, &attr) == 0);
    pthread_condattr_destroy(&attr);
    return ok;
}

struct PosixMutex {
    pthread_mutex_t mutex;
};

struct PosixSem {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    uint32_t count = 0;
    uint32_t maxCount = 0;
};

struct PosixTimer {
    TelOsTimerCallback callback = nullptr;
    void *arg = nullptr;
    bool armed = false;
    int64_t deadlineNs = 0;
    PosixTimer *next = nullptr;
};

// Plays the swtmr task: one thread serves every timer, so callbacks never overlap.
struct TimerService {
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond;
    pthread_t thread;
    bool started = false;
    PosixTimer *timers = nullptr;
    PosixTimer *running = nullptr;
};

TimerService g_timerService;
pthread_once_t g_timerServiceOnce = PTHREAD_ONCE_INIT;
bool g_timerServiceReady = false;

void *TimerServiceLoop(void *)
{
    TimerService &svc = g_timerService;
    pthread_mutex_lock(&svc.lock);
    while (true) {
        PosixTimer *earliest = nullptr;
        for (PosixTimer *t = svc.timers; t != nullptr; t = t->next) {
            if (t->armed && (earliest == nullptr || t->deadlineNs < earliest->deadlineNs)) {
                earliest = t;
            }
        }
        if (earliest == nullptr) {
            pthread_cond_wait(&svc.cond, &svc.lock);
            continue;
        }
        if (MonotonicNs() < earliest->deadlineNs) {
            timespec until = ToTimespec(earliest->deadlineNs);
            pthread_cond_timedwait(&svc.cond, &svc.lock, &until);
            continue;
        }
        earliest->armed = false;
        svc.running = earliest;
        TelOsTimerCallback callback = earliest->callback;
        void *arg = earliest->arg;
        pthread_mutex_unlock(&svc.lock);
        callback(arg);
        pthread_mutex_lock(&svc.lock);
        svc.running = nullptr;
        pthread_cond_broadcast(&svc.cond);
    }
    return nullptr;
}

void StartTimerService()
{
    TimerService &svc = g_timerService;
    if (!InitMonotonicCond(&svc.cond)) {
        return;
    }
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    svc.started = (pthread_create(&svc.thread, &attr, TimerServiceLoop, nullptr) == 0);
    pthread_attr_destroy(&attr);
    g_timerServiceReady = svc.started;
}

struct TaskStart {
    TelOsTaskEntry entry = nullptr;
    void *arg = nullptr;
};

void *TaskTrampoline(void *startArg)
{
    TaskStart *start = static_cast<TaskStart *>(startArg);
    TelOsTaskEntry entry = start->entry;
    void *arg = start->arg;
    delete start;
    entry(arg);
    g_taskCount.fetch_sub(1);
    return nullptr;
}
} // namespace

TelOsResult TelOsMutexCreate(TelOsHandle *mutex)
{
    if (mutex == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    PosixMutex *obj = new (std::nothrow) PosixMutex;
    if (obj == nullptr) {
        return TelOsResult::NO_RESOURCE;
    }
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    int ret = pthread_mutex_init(&obj->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    if (ret != 0) {
        delete obj;
        return TelOsResult::NO_RESOURCE;
    }
    g_mutexCount.fetch_add(1);
    *mutex = obj;
    return TelOsResult::OK;
}

TelOsResult TelOsMutexLock(TelOsHandle mutex)
{
    if (mutex == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    return (pthread_mutex_lock(&static_cast<PosixMutex *>(mutex)->mutex) == 0) ? TelOsResult::OK : TelOsResult::FAIL;
}

TelOsResult TelOsMutexUnlock(TelOsHandle mutex)
{
    if (mutex == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    return (pthread_mutex_unlock(&static_cast<PosixMutex *>(mutex)->mutex) == 0) ? TelOsResult::OK :
        TelOsResult::FAIL;
}

TelOsResult TelOsMutexDelete(TelOsHandle mutex)
{
    if (mutex == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    PosixMutex *obj = static_cast<PosixMutex *>(mutex);
    if (pthread_mutex_destroy(&obj->mutex) != 0) {
        return TelOsResult::FAIL;
    }
    delete obj;
    g_mutexCount.fetch_sub(1);
    return TelOsResult::OK;
}

TelOsResult TelOsSemCreate(uint32_t initialCount, uint32_t maxCount, TelOsHandle *sem)
{
    if (sem == nullptr || maxCount == 0 || initialCount > maxCount) {
        return TelOsResult::INVALID_PARAM;
    }
    PosixSem *obj = new (std::nothrow) PosixSem;
    if (obj == nullptr) {
        return TelOsResult::NO_RESOURCE;
    }
    if (pthread_mutex_init(&obj->lock, nullptr) != 0) {
        delete obj;
        return TelOsResult::NO_RESOURCE;
    }
    if (!InitMonotonicCond(&obj->cond)) {
        pthread_mutex_destroy(&obj->lock);
        delete obj;
        return TelOsResult::NO_RESOURCE;
    }
    obj->count = initialCount;
    obj->maxCount = maxCount;
    g_semCount.fetch_add(1);
    *sem = obj;
    return TelOsResult::OK;
}

TelOsResult TelOsSemWait(TelOsHandle sem, uint32_t timeoutMs)
{
    if (sem == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    PosixSem *obj = static_cast<PosixSem *>(sem);
    int64_t deadline = 0;
    if (timeoutMs != TEL_OS_WAIT_FOREVER) {
        deadline = MonotonicNs() + TicksToNs(TelOsMsToTicks(timeoutMs));
    }
    pthread_mutex_lock(&obj->lock);
    while (obj->count == 0) {
        if (timeoutMs == 0) {
            break;
        }
        if (timeoutMs == TEL_OS_WAIT_FOREVER) {
            pthread_cond_wait(&obj->cond, &obj->lock);
            continue;
        }
        timespec until = ToTimespec(deadline);
        if (pthread_cond_timedwait(&obj->cond, &obj->lock, &until) == ETIMEDOUT) {
            break;
        }
    }
    TelOsResult result = TelOsResult::TIMEOUT;
    if (obj->count > 0) {
        obj->count--;
        result = TelOsResult::OK;
    }
    pthread_mutex_unlock(&obj->lock);
    return result;
}

TelOsResult TelOsSemPost(TelOsHandle sem)
{
    if (sem == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    PosixSem *obj = static_cast<PosixSem *>(sem);
    pthread_mutex_lock(&obj->lock);
    TelOsResult result = TelOsResult::FAIL;
    if (obj->count < obj->maxCount) {
        obj->count++;
        pthread_cond_signal(&obj->cond);
        result = TelOsResult::OK;
    }
    pthread_mutex_unlock(&obj->lock);
    return result;
}

TelOsResult TelOsSemDelete(TelOsHandle sem)
{
    if (sem == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    PosixSem *obj = static_cast<PosixSem *>(sem);
    pthread_cond_destroy(&obj->cond);
    pthread_mutex_destroy(&obj->lock);
    delete obj;
    g_semCount.fetch_sub(1);
    return TelOsResult::OK;
}

TelOsResult TelOsTimerCreate(TelOsTimerCallback callback, void *arg, TelOsHandle *timer)
{
    if (callback == nullptr || timer == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    pthread_once(&g_timerServiceOnce, StartTimerService);
    if (!g_timerServiceReady) {
        return TelOsResult::NO_RESOURCE;
    }
    PosixTimer *obj = new (std::nothrow) PosixTimer;
    if (obj == nullptr) {
        return TelOsResult::NO_RESOURCE;
    }
    obj->callback = callback;
    obj->arg = arg;
    TimerService &svc = g_timerService;
    pthread_mutex_lock(&svc.lock);
    obj->next = svc.timers;
    svc.timers = obj;
    pthread_mutex_unlock(&svc.lock);
    g_timerCount.fetch_add(1);
    *timer = obj;
    return TelOsResult::OK;
}

TelOsResult TelOsTimerStart(TelOsHandle timer, uint32_t delayMs)
{
    if (timer == nullptr || delayMs == TEL_OS_WAIT_FOREVER) {
        return TelOsResult::INVALID_PARAM;
    }
    uint32_t ticks = TelOsMsToTicks(delayMs);
    if (ticks == 0) {
        ticks = 1;
    }
    TimerService &svc = g_timerService;
    PosixTimer *obj = static_cast<PosixTimer *>(timer);
    pthread_mutex_lock(&svc.lock);
    obj->deadlineNs = MonotonicNs() + TicksToNs(ticks);
    obj->armed = true;
    pthread_cond_broadcast(&svc.cond);
    pthread_mutex_unlock(&svc.lock);
    return TelOsResult::OK;
}

TelOsResult TelOsTimerStop(TelOsHandle timer)
{
    if (timer == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    TimerService &svc = g_timerService;
    PosixTimer *obj = static_cast<PosixTimer *>(timer);
    pthread_mutex_lock(&svc.lock);
    bool wasArmed = obj->armed;
    obj->armed = false;
    pthread_mutex_unlock(&svc.lock);
    return wasArmed ? TelOsResult::OK : TelOsResult::FAIL;
}

TelOsResult TelOsTimerDelete(TelOsHandle timer)
{
    if (timer == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    TimerService &svc = g_timerService;
    PosixTimer *obj = static_cast<PosixTimer *>(timer);
    pthread_mutex_lock(&svc.lock);
    if (svc.running == obj && pthread_equal(pthread_self(), svc.thread)) {
        pthread_mutex_unlock(&svc.lock);
        return TelOsResult::INVALID_PARAM;
    }
    PosixTimer **link = &svc.timers;
    while (*link != nullptr && *link != obj) {
        link = &(*link)->next;
    }
    if (*link == nullptr) {
        pthread_mutex_unlock(&svc.lock);
        return TelOsResult::INVALID_PARAM;
    }
    *link = obj->next;
    while (svc.running == obj) {
        pthread_cond_wait(&svc.cond, &svc.lock);
    }
    pthread_mutex_unlock(&svc.lock);
    delete obj;
    g_timerCount.fetch_sub(1);
    return TelOsResult::OK;
}

TelOsResult TelOsTaskCreate(const TelOsTaskParam &param)
{
    if (param.entry == nullptr || param.name == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    TaskStart *start = new (std::nothrow) TaskStart;
    if (start == nullptr) {
        return TelOsResult::NO_RESOURCE;
    }
    start->entry = param.entry;
    start->arg = param.arg;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, (param.stackSize > HOST_MIN_TASK_STACK) ? param.stackSize : HOST_MIN_TASK_STACK);
    g_taskCount.fetch_add(1);
    pthread_t thread;
    int ret = pthread_create(&thread, &attr, TaskTrampoline, start);
    pthread_attr_destroy(&attr);
    if (ret != 0) {
        g_taskCount.fetch_sub(1);
        delete start;
        return TelOsResult::NO_RESOURCE;
    }
    return TelOsResult::OK;
}

TelOsTaskId TelOsGetCurrentTaskId()
{
    thread_local TelOsTaskId taskId = g_nextTaskId.fetch_add(1) + 1;
    return taskId;
}

void TelOsTaskYield()
{
    sched_yield();
}

void TelOsTaskDelayMs(uint32_t delayMs)
{
    if (delayMs == 0 || delayMs == TEL_OS_WAIT_FOREVER) {
        return;
    }
    timespec until = ToTimespec(MonotonicNs() + TicksToNs(TelOsMsToTicks(delayMs)));
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &until, nullptr) == EINTR) {
    }
}

uint32_t TelOsGetTickFreq()
{
    return POSIX_TICK_PER_SECOND;
}

uint32_t TelOsGetTickCount()
{
    return static_cast<uint32_t>(MonotonicNs() / (NS_PER_SECOND / POSIX_TICK_PER_SECOND));
}

TelOsObjectCount TelOsGetObjectCount()
{
    TelOsObjectCount count;
    count.mutexCount = g_mutexCount.load();
    count.semaphoreCount = g_semCount.load();
    count.timerCount = g_timerCount.load();
    count.taskCount = g_taskCount.load();
    return count;
}
} // namespace Telephony
} // namespace OHOS
