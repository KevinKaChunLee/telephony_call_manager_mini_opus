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

#ifndef MINI_CORE_SERVICE_CLIENT_H
#define MINI_CORE_SERVICE_CLIENT_H

// Mini variant of core_service's CoreServiceClient, limited to the queries the ported call manager
// makes. It forwards to CoreServiceLiteRegistry, so ported code keeps the standard
// DelayedRefSingleton<CoreServiceClient>::GetInstance().HasSimCard(...) form. Without a registered
// implementation every query fails with TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL and leaves its output
// untouched, which the callers already treat as "no SIM" / "no service".

#include "singleton.h"

namespace OHOS {
namespace Telephony {
class CoreServiceClient {
    DECLARE_DELAYED_REF_SINGLETON(CoreServiceClient)

public:
    int32_t HasSimCard(int32_t slotId, bool &hasSimCard);
    int32_t IsNetworkInService(int32_t slotId, bool &inService);
};
} // namespace Telephony
} // namespace OHOS
#endif // MINI_CORE_SERVICE_CLIENT_H
