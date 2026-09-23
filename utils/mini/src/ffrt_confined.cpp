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

#include "ffrt.h"

#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
namespace {
std::atomic<TelMiniConfinementCheck> g_confinementCheck { nullptr };
std::atomic<uint32_t> g_confinementViolations { 0 };
} // namespace

void TelMiniSetConfinementCheck(TelMiniConfinementCheck check)
{
    g_confinementCheck.store(check, std::memory_order_release);
}

uint32_t TelMiniConfinementViolations()
{
    return g_confinementViolations.load(std::memory_order_relaxed);
}

#ifdef TELEPHONY_MINI_HOST_TEST
void TelMiniNoteConfinedAccess()
{
    TelMiniConfinementCheck check = g_confinementCheck.load(std::memory_order_acquire);
    if (check != nullptr && !check()) {
        g_confinementViolations.fetch_add(1, std::memory_order_relaxed);
        TELEPHONY_LOGE("business state locked outside the main loop");
    }
}
#endif
} // namespace Telephony
} // namespace OHOS
