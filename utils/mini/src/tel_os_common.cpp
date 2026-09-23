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

namespace OHOS {
namespace Telephony {
uint32_t TelOsMsToTicks(uint32_t delayMs)
{
    if (delayMs == TEL_OS_WAIT_FOREVER) {
        return TEL_OS_WAIT_FOREVER;
    }
    constexpr uint64_t msPerSecond = 1000;
    uint64_t tickFreq = TelOsGetTickFreq();
    if (tickFreq == 0) {
        return delayMs;
    }
    uint64_t ticks = (static_cast<uint64_t>(delayMs) * tickFreq + msPerSecond - 1) / msPerSecond;
    if (ticks > TEL_OS_MAX_FINITE_TICKS) {
        return TEL_OS_MAX_FINITE_TICKS;
    }
    return static_cast<uint32_t>(ticks);
}
} // namespace Telephony
} // namespace OHOS
