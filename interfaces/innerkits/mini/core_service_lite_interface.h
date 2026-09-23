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

#ifndef CORE_SERVICE_LITE_INTERFACE_H
#define CORE_SERVICE_LITE_INTERFACE_H

// SIM and network queries the Mini call manager makes before dialing (CallPolicy::HasNormalCall).
// The product's SIM/network component implements it and registers the instance with
// CoreServiceLiteRegistry. Calls happen on the call manager's main loop and must not block on it.

#include "tel_mini_std_includes.h"

#include "refbase.h"

namespace OHOS {
namespace Telephony {
class CoreServiceLiteInterface : public virtual RefBase {
public:
    virtual ~CoreServiceLiteInterface() = default;

    virtual int32_t HasSimCard(int32_t slotId, bool &hasSimCard) = 0;
    // inService: registered for voice on the given slot (RegServiceState::REG_STATE_IN_SERVICE).
    virtual int32_t IsNetworkInService(int32_t slotId, bool &inService) = 0;
};

class CoreServiceLiteRegistry {
public:
    // Replaces any earlier instance; nullptr unregisters.
    static void Register(const sptr<CoreServiceLiteInterface> &instance);
    static sptr<CoreServiceLiteInterface> Get();
};
} // namespace Telephony
} // namespace OHOS
#endif // CORE_SERVICE_LITE_INTERFACE_H
