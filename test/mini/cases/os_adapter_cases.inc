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

// Kernel adapter cases: they run unchanged over tel_os_adapter_posix.cpp and
// tel_os_adapter_cmsis.cpp, so shared state goes through globals rather than callback arguments.

#include <atomic>

#include "tel_os_adapter.h"

namespace OsAdapterCases {
using namespace OHOS::Telephony;

std::atomic<int> g_timerFired { 0 };

void CountTimerFire(void *)
{
    g_timerFired.fetch_add(1);
}

bool WaitUntil(const std::atomic<int> &value, int expected, uint32_t timeoutMs)
{
    constexpr uint32_t pollMs = 10;
    for (uint32_t waited = 0; waited < timeoutMs; waited += pollMs) {
        if (value.load() == expected) {
            return true;
        }
        TelOsTaskDelayMs(pollMs);
    }
    return value.load() == expected;
}

struct TaskProbe {
    TelOsHandle done = nullptr;
    std::atomic<TelOsTaskId> taskId { 0 };
};

TaskProbe g_taskProbe;

void RecordTaskAndSignal(void *)
{
    g_taskProbe.taskId.store(TelOsGetCurrentTaskId());
    TelOsSemPost(g_taskProbe.done);
}
} // namespace OsAdapterCases

MINI_TEST(OsAdapter, SemaphoreWaitTimesOut)
{
    using namespace OHOS::Telephony;
    TelOsHandle sem = nullptr;
    ASSERT_TRUE(TelOsSemCreate(0, 1, &sem) == TelOsResult::OK);
    constexpr uint32_t waitMs = 30;
    uint32_t begin = TelOsGetTickCount();
    EXPECT_TRUE(TelOsSemWait(sem, waitMs) == TelOsResult::TIMEOUT);
    uint32_t elapsed = TelOsGetTickCount() - begin;
    EXPECT_TRUE(elapsed + 1 >= TelOsMsToTicks(waitMs));
    EXPECT_TRUE(TelOsSemWait(sem, 0) == TelOsResult::TIMEOUT);
    EXPECT_TRUE(TelOsSemPost(sem) == TelOsResult::OK);
    EXPECT_TRUE(TelOsSemPost(sem) == TelOsResult::FAIL);
    EXPECT_TRUE(TelOsSemWait(sem, 0) == TelOsResult::OK);
    EXPECT_TRUE(TelOsSemDelete(sem) == TelOsResult::OK);
}

MINI_TEST(OsAdapter, OneShotTimerFiresExactlyOnce)
{
    using namespace OHOS::Telephony;
    OsAdapterCases::g_timerFired.store(0);
    TelOsHandle timer = nullptr;
    ASSERT_TRUE(TelOsTimerCreate(OsAdapterCases::CountTimerFire, nullptr, &timer) == TelOsResult::OK);
    EXPECT_TRUE(TelOsTimerStop(timer) == TelOsResult::FAIL);
    EXPECT_TRUE(TelOsTimerStart(timer, 20) == TelOsResult::OK);
    EXPECT_TRUE(OsAdapterCases::WaitUntil(OsAdapterCases::g_timerFired, 1, 1000));
    TelOsTaskDelayMs(80);
    EXPECT_EQ(OsAdapterCases::g_timerFired.load(), 1);
    EXPECT_TRUE(TelOsTimerStop(timer) == TelOsResult::FAIL);

    EXPECT_TRUE(TelOsTimerStart(timer, 50) == TelOsResult::OK);
    EXPECT_TRUE(TelOsTimerStop(timer) == TelOsResult::OK);
    TelOsTaskDelayMs(100);
    EXPECT_EQ(OsAdapterCases::g_timerFired.load(), 1);
    EXPECT_TRUE(TelOsTimerDelete(timer) == TelOsResult::OK);
}

MINI_TEST(OsAdapter, MsToTicksRoundsUpAndNeverWraps)
{
    using namespace OHOS::Telephony;
    const uint64_t freq = TelOsGetTickFreq();
    ASSERT_TRUE(freq > 0 && freq <= 1000);
    EXPECT_EQ(TelOsMsToTicks(0), 0U);
    EXPECT_EQ(TelOsMsToTicks(1), 1U);
    EXPECT_EQ(static_cast<uint64_t>(TelOsMsToTicks(10)), (10 * freq + 999) / 1000);
    EXPECT_EQ(TelOsMsToTicks(TEL_OS_WAIT_FOREVER), TEL_OS_WAIT_FOREVER);
    uint32_t huge = TelOsMsToTicks(TEL_OS_WAIT_FOREVER - 1);
    EXPECT_TRUE(huge <= TEL_OS_MAX_FINITE_TICKS);
    EXPECT_TRUE(huge >= TelOsMsToTicks(0x7FFFFFFFU));
    EXPECT_TRUE(huge >= TelOsMsToTicks(1000));
}

MINI_TEST(OsAdapter, TaskRunsOnItsOwnTask)
{
    using namespace OHOS::Telephony;
    OsAdapterCases::TaskProbe &probe = OsAdapterCases::g_taskProbe;
    probe.taskId.store(0);
    ASSERT_TRUE(TelOsSemCreate(0, 1, &probe.done) == TelOsResult::OK);
    TelOsTaskParam param;
    param.name = "mini_test_task";
    param.entry = OsAdapterCases::RecordTaskAndSignal;
    param.stackSize = 16384;
    param.priority = 20;
    EXPECT_TRUE(TelOsTaskCreate(param) == TelOsResult::OK);
    EXPECT_TRUE(TelOsSemWait(probe.done, 1000) == TelOsResult::OK);
    EXPECT_TRUE(probe.taskId.load() != 0);
    EXPECT_TRUE(probe.taskId.load() != TelOsGetCurrentTaskId());
    EXPECT_EQ(TelOsGetCurrentTaskId(), TelOsGetCurrentTaskId());
    EXPECT_TRUE(TelOsSemDelete(probe.done) == TelOsResult::OK);
    probe.done = nullptr;
}

MINI_TEST(OsAdapter, ObjectCountTracksCreateAndDelete)
{
    using namespace OHOS::Telephony;
    TelOsObjectCount before = TelOsGetObjectCount();
    TelOsHandle mutex = nullptr;
    TelOsHandle sem = nullptr;
    TelOsHandle timer = nullptr;
    ASSERT_TRUE(TelOsMutexCreate(&mutex) == TelOsResult::OK);
    ASSERT_TRUE(TelOsSemCreate(1, 1, &sem) == TelOsResult::OK);
    ASSERT_TRUE(TelOsTimerCreate(OsAdapterCases::CountTimerFire, nullptr, &timer) == TelOsResult::OK);
    TelOsObjectCount during = TelOsGetObjectCount();
    EXPECT_EQ(during.mutexCount, before.mutexCount + 1);
    EXPECT_EQ(during.semaphoreCount, before.semaphoreCount + 1);
    EXPECT_EQ(during.timerCount, before.timerCount + 1);
    EXPECT_TRUE(TelOsMutexLock(mutex) == TelOsResult::OK);
    EXPECT_TRUE(TelOsMutexUnlock(mutex) == TelOsResult::OK);
    EXPECT_TRUE(TelOsTimerDelete(timer) == TelOsResult::OK);
    EXPECT_TRUE(TelOsSemDelete(sem) == TelOsResult::OK);
    EXPECT_TRUE(TelOsMutexDelete(mutex) == TelOsResult::OK);
    TelOsObjectCount after = TelOsGetObjectCount();
    EXPECT_EQ(after.mutexCount, before.mutexCount);
    EXPECT_EQ(after.semaphoreCount, before.semaphoreCount);
    EXPECT_EQ(after.timerCount, before.timerCount);
}
