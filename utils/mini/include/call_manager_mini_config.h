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

#ifndef TELEPHONY_MINI_CALL_MANAGER_MINI_CONFIG_H
#define TELEPHONY_MINI_CALL_MANAGER_MINI_CONFIG_H

#include <cstdint>

// Product values come from callmanager_mini.gni (declare_args -> defines); these are the defaults.

#ifndef CALL_MANAGER_MINI_TASK_PRIORITY
#define CALL_MANAGER_MINI_TASK_PRIORITY 24
#endif
#ifndef CALL_MANAGER_MINI_TASK_STACK_SIZE
#define CALL_MANAGER_MINI_TASK_STACK_SIZE 16384
#endif
#ifndef CALL_MANAGER_MINI_QUEUE_SIZE
#define CALL_MANAGER_MINI_QUEUE_SIZE 16
#endif
#ifndef CALL_MANAGER_MINI_SYNC_WAIT_MS
#define CALL_MANAGER_MINI_SYNC_WAIT_MS 3000
#endif
#ifndef CALL_MANAGER_MINI_MAX_SYNC_WAITERS
#define CALL_MANAGER_MINI_MAX_SYNC_WAITERS 2
#endif
#ifndef CALL_MANAGER_MINI_MAX_CALL_OBJECTS
#define CALL_MANAGER_MINI_MAX_CALL_OBJECTS 5
#endif
#ifndef CALL_MANAGER_MINI_MAX_DELAYED_TASKS
#define CALL_MANAGER_MINI_MAX_DELAYED_TASKS 8
#endif
#ifndef CALL_MANAGER_MINI_PACMAP_MAX_ENTRIES
#define CALL_MANAGER_MINI_PACMAP_MAX_ENTRIES 32
#endif
// Standard reads const.telephony.slotCount at run time; the Mini image fixes it at build time.
#ifndef CALL_MANAGER_MINI_SLOT_COUNT
#define CALL_MANAGER_MINI_SLOT_COUNT 2
#endif

namespace OHOS {
namespace Telephony {
constexpr uint32_t MINI_TASK_PRIORITY = CALL_MANAGER_MINI_TASK_PRIORITY;
constexpr uint32_t MINI_TASK_STACK_SIZE = CALL_MANAGER_MINI_TASK_STACK_SIZE;
constexpr uint32_t MINI_QUEUE_SIZE = CALL_MANAGER_MINI_QUEUE_SIZE;
constexpr uint32_t MINI_SYNC_WAIT_MS = CALL_MANAGER_MINI_SYNC_WAIT_MS;
constexpr uint32_t MINI_MAX_SYNC_WAITERS = CALL_MANAGER_MINI_MAX_SYNC_WAITERS;
constexpr uint32_t MINI_MAX_CALL_OBJECTS = CALL_MANAGER_MINI_MAX_CALL_OBJECTS;
constexpr uint32_t MINI_ECC_RESERVED_CALL_OBJECTS = 1;
constexpr uint32_t MINI_MAX_DELAYED_TASKS = CALL_MANAGER_MINI_MAX_DELAYED_TASKS;
constexpr uint32_t MINI_PACMAP_MAX_ENTRIES = CALL_MANAGER_MINI_PACMAP_MAX_ENTRIES;
constexpr int32_t MINI_SLOT_COUNT = CALL_MANAGER_MINI_SLOT_COUNT;
// Largest batch a stack report may carry, as the standard CallStatusCallbackStub enforces (MAX_CALLS_NUM).
constexpr uint32_t MINI_MAX_REPORTED_CALLS = 5;

// samgr_lite TaskConfig carries stackSize and queueSize as uint16.
static_assert(MINI_TASK_STACK_SIZE > 0 && MINI_TASK_STACK_SIZE <= UINT16_MAX, "stack size must fit TaskConfig");
static_assert(MINI_QUEUE_SIZE > 0 && MINI_QUEUE_SIZE <= UINT16_MAX, "queue size must fit TaskConfig");
static_assert(MINI_TASK_PRIORITY < 39, "priority must be below samgr_lite PRI_BUTT");
static_assert(MINI_SYNC_WAIT_MS > 0, "synchronous calls must have a finite, non-zero wait");
static_assert(MINI_MAX_SYNC_WAITERS > 0, "at least one synchronous caller must be able to wait");
static_assert(MINI_MAX_CALL_OBJECTS > MINI_ECC_RESERVED_CALL_OBJECTS, "one call object slot is kept for ECC");
static_assert(MINI_MAX_DELAYED_TASKS > 0, "delayed tasks are part of the retained behaviour");
static_assert(MINI_PACMAP_MAX_ENTRIES > 0, "dial extras need at least one entry");
// CallStatusManager keeps per-slot report state in arrays of SLOT_NUM (2) entries.
static_assert(MINI_SLOT_COUNT > 0 && MINI_SLOT_COUNT <= 2, "slot count must fit CallStatusManager's slot tables");
} // namespace Telephony
} // namespace OHOS
#endif // TELEPHONY_MINI_CALL_MANAGER_MINI_CONFIG_H
