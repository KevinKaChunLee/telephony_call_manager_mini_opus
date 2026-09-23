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

#ifndef TELEPHONY_MINI_SPIN_LOCK_H
#define TELEPHONY_MINI_SPIN_LOCK_H

// Leaf lock for the adapter registries: it owns no kernel object and needs no creation step, so
// products can register their adapters before the call manager starts. Critical sections only copy
// pointers. A waiter first yields, then sleeps a tick, so that a lower-priority holder gets to run
// under LiteOS-M's strict priority scheduling.

#include "tel_mini_std_includes.h"

#include <mutex>

#include "tel_os_adapter.h"

namespace OHOS {
namespace Telephony {
class TelMiniSpinLock {
public:
    void lock()
    {
        constexpr uint32_t yieldsBeforeSleep = 8;
        uint32_t attempts = 0;
        while (flag_.test_and_set(std::memory_order_acquire)) {
            if (++attempts < yieldsBeforeSleep) {
                TelOsTaskYield();
            } else {
                TelOsTaskDelayMs(1);
            }
        }
    }
    void unlock()
    {
        flag_.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};
} // namespace Telephony
} // namespace OHOS
#endif // TELEPHONY_MINI_SPIN_LOCK_H
