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
#include <new>

#include "cmsis_os2.h"
#include "los_task.h"

namespace OHOS {
namespace Telephony {
namespace {
// The LiteOS-M KAL truncates osTimerNew's argument and LOS_TaskCreate's uwArg to 32 bits, so
// callbacks receive a slot index into these tables rather than a pointer.
constexpr uint32_t CMSIS_MAX_TIMERS = 4;
constexpr uint32_t CMSIS_MAX_TASKS = 8;
constexpr uint32_t SLOT_FREE = 0;
constexpr uint32_t SLOT_USED = 1;

std::atomic<uint32_t> g_mutexCount { 0 };
std::atomic<uint32_t> g_semCount { 0 };
std::atomic<uint32_t> g_timerCount { 0 };
std::atomic<uint32_t> g_taskCount { 0 };

struct CmsisSem {
    osSemaphoreId_t id = nullptr;
    uint32_t maxCount = 0;
};

struct TimerSlot {
    std::atomic<uint32_t> state { SLOT_FREE };
    std::atomic<uint32_t> inCallback { 0 };
    TelOsTimerCallback callback = nullptr;
    void *arg = nullptr;
    osTimerId_t id = nullptr;
};

struct TaskSlot {
    std::atomic<uint32_t> state { SLOT_FREE };
    TelOsTaskEntry entry = nullptr;
    void *arg = nullptr;
};

TimerSlot g_timerSlots[CMSIS_MAX_TIMERS];
TaskSlot g_taskSlots[CMSIS_MAX_TASKS];

template <typename Slot, uint32_t N>
int32_t ClaimSlot(Slot (&slots)[N])
{
    for (uint32_t i = 0; i < N; i++) {
        uint32_t expected = SLOT_FREE;
        if (slots[i].state.compare_exchange_strong(expected, SLOT_USED, std::memory_order_acq_rel)) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

void *SlotArgument(uint32_t index)
{
    return reinterpret_cast<void *>(static_cast<uintptr_t>(index));
}

void TimerTrampoline(void *argument)
{
    uint32_t index = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(argument));
    if (index >= CMSIS_MAX_TIMERS) {
        return;
    }
    TimerSlot &slot = g_timerSlots[index];
    slot.inCallback.fetch_add(1, std::memory_order_acq_rel);
    if (slot.state.load(std::memory_order_acquire) == SLOT_USED && slot.callback != nullptr) {
        slot.callback(slot.arg);
    }
    slot.inCallback.fetch_sub(1, std::memory_order_acq_rel);
}

VOID *TaskTrampoline(UINT32 index)
{
    if (index >= CMSIS_MAX_TASKS) {
        return nullptr;
    }
    TaskSlot &slot = g_taskSlots[index];
    TelOsTaskEntry entry = slot.entry;
    void *arg = slot.arg;
    slot.state.store(SLOT_FREE, std::memory_order_release);
    entry(arg);
    g_taskCount.fetch_sub(1);
    return nullptr;
}

uint32_t ToCmsisTimeout(uint32_t timeoutMs)
{
    return (timeoutMs == TEL_OS_WAIT_FOREVER) ? osWaitForever : TelOsMsToTicks(timeoutMs);
}
} // namespace

TelOsResult TelOsMutexCreate(TelOsHandle *mutex)
{
    if (mutex == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    osMutexId_t id = osMutexNew(nullptr);
    if (id == nullptr) {
        return TelOsResult::NO_RESOURCE;
    }
    g_mutexCount.fetch_add(1);
    *mutex = id;
    return TelOsResult::OK;
}

TelOsResult TelOsMutexLock(TelOsHandle mutex)
{
    if (mutex == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    return (osMutexAcquire(mutex, osWaitForever) == osOK) ? TelOsResult::OK : TelOsResult::FAIL;
}

TelOsResult TelOsMutexUnlock(TelOsHandle mutex)
{
    if (mutex == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    return (osMutexRelease(mutex) == osOK) ? TelOsResult::OK : TelOsResult::FAIL;
}

TelOsResult TelOsMutexDelete(TelOsHandle mutex)
{
    if (mutex == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    if (osMutexDelete(mutex) != osOK) {
        return TelOsResult::FAIL;
    }
    g_mutexCount.fetch_sub(1);
    return TelOsResult::OK;
}

TelOsResult TelOsSemCreate(uint32_t initialCount, uint32_t maxCount, TelOsHandle *sem)
{
    if (sem == nullptr || maxCount == 0 || initialCount > maxCount) {
        return TelOsResult::INVALID_PARAM;
    }
    CmsisSem *obj = new (std::nothrow) CmsisSem;
    if (obj == nullptr) {
        return TelOsResult::NO_RESOURCE;
    }
    obj->id = osSemaphoreNew(maxCount, initialCount, nullptr);
    if (obj->id == nullptr) {
        delete obj;
        return TelOsResult::NO_RESOURCE;
    }
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
    osStatus_t ret = osSemaphoreAcquire(static_cast<CmsisSem *>(sem)->id, ToCmsisTimeout(timeoutMs));
    if (ret == osOK) {
        return TelOsResult::OK;
    }
    // A zero timeout on an empty semaphore reports osErrorResource on LiteOS-M.
    return (ret == osErrorTimeout || ret == osErrorResource) ? TelOsResult::TIMEOUT : TelOsResult::FAIL;
}

TelOsResult TelOsSemPost(TelOsHandle sem)
{
    if (sem == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    CmsisSem *obj = static_cast<CmsisSem *>(sem);
    // LiteOS-M only bounds binary semaphores; counting ones go up to OS_SEM_COUNTING_MAX_COUNT.
    if (osSemaphoreGetCount(obj->id) >= obj->maxCount) {
        return TelOsResult::FAIL;
    }
    return (osSemaphoreRelease(obj->id) == osOK) ? TelOsResult::OK : TelOsResult::FAIL;
}

TelOsResult TelOsSemDelete(TelOsHandle sem)
{
    if (sem == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    CmsisSem *obj = static_cast<CmsisSem *>(sem);
    if (osSemaphoreDelete(obj->id) != osOK) {
        return TelOsResult::FAIL;
    }
    delete obj;
    g_semCount.fetch_sub(1);
    return TelOsResult::OK;
}

TelOsResult TelOsTimerCreate(TelOsTimerCallback callback, void *arg, TelOsHandle *timer)
{
    if (callback == nullptr || timer == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    int32_t index = ClaimSlot(g_timerSlots);
    if (index < 0) {
        return TelOsResult::NO_RESOURCE;
    }
    TimerSlot &slot = g_timerSlots[index];
    slot.callback = callback;
    slot.arg = arg;
    slot.id = osTimerNew(TimerTrampoline, osTimerOnce, SlotArgument(static_cast<uint32_t>(index)), nullptr);
    if (slot.id == nullptr) {
        slot.callback = nullptr;
        slot.state.store(SLOT_FREE, std::memory_order_release);
        return TelOsResult::NO_RESOURCE;
    }
    g_timerCount.fetch_add(1);
    *timer = &slot;
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
    return (osTimerStart(static_cast<TimerSlot *>(timer)->id, ticks) == osOK) ? TelOsResult::OK : TelOsResult::FAIL;
}

TelOsResult TelOsTimerStop(TelOsHandle timer)
{
    if (timer == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    return (osTimerStop(static_cast<TimerSlot *>(timer)->id) == osOK) ? TelOsResult::OK : TelOsResult::FAIL;
}

TelOsResult TelOsTimerDelete(TelOsHandle timer)
{
    if (timer == nullptr) {
        return TelOsResult::INVALID_PARAM;
    }
    TimerSlot *slot = static_cast<TimerSlot *>(timer);
    if (osTimerDelete(slot->id) != osOK) {
        return TelOsResult::FAIL;
    }
    while (slot->inCallback.load(std::memory_order_acquire) != 0) {
        TelOsTaskYield();
    }
    slot->id = nullptr;
    slot->callback = nullptr;
    slot->arg = nullptr;
    slot->state.store(SLOT_FREE, std::memory_order_release);
    g_timerCount.fetch_sub(1);
    return TelOsResult::OK;
}

TelOsResult TelOsTaskCreate(const TelOsTaskParam &param)
{
    if (param.entry == nullptr || param.name == nullptr || param.priority > OS_TASK_PRIORITY_LOWEST) {
        return TelOsResult::INVALID_PARAM;
    }
    int32_t index = ClaimSlot(g_taskSlots);
    if (index < 0) {
        return TelOsResult::NO_RESOURCE;
    }
    TaskSlot &slot = g_taskSlots[index];
    slot.entry = param.entry;
    slot.arg = param.arg;
    TSK_INIT_PARAM_S init {};
    init.pfnTaskEntry = TaskTrampoline;
    init.usTaskPrio = param.priority;
    init.uwArg = static_cast<UINT32>(index);
    init.uwStackSize = param.stackSize;
    // LiteOS-M keeps the name pointer, so callers pass string literals.
    init.pcName = const_cast<CHAR *>(param.name);
    g_taskCount.fetch_add(1);
    UINT32 taskId = 0;
    if (LOS_TaskCreate(&taskId, &init) != LOS_OK) {
        g_taskCount.fetch_sub(1);
        slot.state.store(SLOT_FREE, std::memory_order_release);
        return TelOsResult::NO_RESOURCE;
    }
    return TelOsResult::OK;
}

TelOsTaskId TelOsGetCurrentTaskId()
{
    return reinterpret_cast<TelOsTaskId>(osThreadGetId());
}

void TelOsTaskYield()
{
    (void)LOS_TaskYield();
}

void TelOsTaskDelayMs(uint32_t delayMs)
{
    if (delayMs == 0 || delayMs == TEL_OS_WAIT_FOREVER) {
        return;
    }
    (void)osDelay(TelOsMsToTicks(delayMs));
}

uint32_t TelOsGetTickFreq()
{
    return osKernelGetTickFreq();
}

uint32_t TelOsGetTickCount()
{
    return osKernelGetTickCount();
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
