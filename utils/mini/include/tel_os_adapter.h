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

#ifndef TELEPHONY_MINI_TEL_OS_ADAPTER_H
#define TELEPHONY_MINI_TEL_OS_ADAPTER_H

#include <cstdint>

// The only way Mini code reaches kernel objects. Two implementations define these symbols and
// must never be linked together: tel_os_adapter_cmsis.cpp ships on the target,
// tel_os_adapter_posix.cpp exists for host self-tests only.
//
// Constraints shared by both implementations:
//  - nothing here may be called from an interrupt handler;
//  - a mutex must not be locked again by its owner (LiteOS-M mutexes are recursive, the host
//    ones report the relock as a failure, so correct code must not depend on either);
//  - a timer must not be deleted from its own callback, and callbacks must not block;
//  - a task ends by returning from its entry function.

namespace OHOS {
namespace Telephony {
enum class TelOsResult : int32_t {
    OK = 0,
    INVALID_PARAM,
    NO_RESOURCE,
    TIMEOUT,
    FAIL,
};

using TelOsHandle = void *;
using TelOsTaskId = uintptr_t;
using TelOsTimerCallback = void (*)(void *arg);
using TelOsTaskEntry = void (*)(void *arg);

constexpr uint32_t TEL_OS_WAIT_FOREVER = 0xFFFFFFFFU;
constexpr uint32_t TEL_OS_MAX_FINITE_TICKS = TEL_OS_WAIT_FOREVER - 1;

struct TelOsTaskParam {
    const char *name = nullptr;
    TelOsTaskEntry entry = nullptr;
    void *arg = nullptr;
    uint32_t stackSize = 0;
    uint16_t priority = 0;
};

struct TelOsObjectCount {
    uint32_t mutexCount = 0;
    uint32_t semaphoreCount = 0;
    uint32_t timerCount = 0;
    uint32_t taskCount = 0;
};

TelOsResult TelOsMutexCreate(TelOsHandle *mutex);
TelOsResult TelOsMutexLock(TelOsHandle mutex);
TelOsResult TelOsMutexUnlock(TelOsHandle mutex);
TelOsResult TelOsMutexDelete(TelOsHandle mutex);

TelOsResult TelOsSemCreate(uint32_t initialCount, uint32_t maxCount, TelOsHandle *sem);
// timeoutMs == 0 polls, TEL_OS_WAIT_FOREVER blocks without limit.
TelOsResult TelOsSemWait(TelOsHandle sem, uint32_t timeoutMs);
// Fails once the count has reached maxCount.
TelOsResult TelOsSemPost(TelOsHandle sem);
TelOsResult TelOsSemDelete(TelOsHandle sem);

// One-shot timers. Start re-arms a pending timer; a delay shorter than one tick waits one tick.
TelOsResult TelOsTimerCreate(TelOsTimerCallback callback, void *arg, TelOsHandle *timer);
TelOsResult TelOsTimerStart(TelOsHandle timer, uint32_t delayMs);
// Returns FAIL when the timer was not pending (never started, already fired or stopped).
TelOsResult TelOsTimerStop(TelOsHandle timer);
TelOsResult TelOsTimerDelete(TelOsHandle timer);

TelOsResult TelOsTaskCreate(const TelOsTaskParam &param);
TelOsTaskId TelOsGetCurrentTaskId();
void TelOsTaskYield();
void TelOsTaskDelayMs(uint32_t delayMs);

uint32_t TelOsGetTickFreq();
uint32_t TelOsGetTickCount();
// Rounds up, maps TEL_OS_WAIT_FOREVER to itself and clamps every finite delay to
// TEL_OS_MAX_FINITE_TICKS so that no finite delay turns into "forever".
uint32_t TelOsMsToTicks(uint32_t delayMs);

// Kernel objects created through this adapter and not yet deleted; tasks count until their
// entry function returns.
TelOsObjectCount TelOsGetObjectCount();
} // namespace Telephony
} // namespace OHOS
#endif // TELEPHONY_MINI_TEL_OS_ADAPTER_H
