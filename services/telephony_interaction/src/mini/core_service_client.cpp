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

#include "core_service_client.h"

#include "core_service_lite_interface.h"
#include "telephony_errors.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
CoreServiceClient::CoreServiceClient() {}

CoreServiceClient::~CoreServiceClient() {}

int32_t CoreServiceClient::HasSimCard(int32_t slotId, bool &hasSimCard)
{
    sptr<CoreServiceLiteInterface> coreService = CoreServiceLiteRegistry::Get();
    if (coreService == nullptr) {
        TELEPHONY_LOGE("core service lite is not registered");
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    return coreService->HasSimCard(slotId, hasSimCard);
}

int32_t CoreServiceClient::IsNetworkInService(int32_t slotId, bool &inService)
{
    sptr<CoreServiceLiteInterface> coreService = CoreServiceLiteRegistry::Get();
    if (coreService == nullptr) {
        TELEPHONY_LOGE("core service lite is not registered");
        return TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL;
    }
    return coreService->IsNetworkInService(slotId, inService);
}
} // namespace Telephony
} // namespace OHOS
