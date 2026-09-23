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

#ifndef CALL_ABILITY_CALLBACK_H
#define CALL_ABILITY_CALLBACK_H

// Mini variant of frameworks/native/include/call_ability_callback.h. It owns the application's
// CallManagerCallback. Reports may still be queued on the main loop when the application unregisters,
// so Close() makes later reports no-ops instead of reaching a callback the application gave up.
// Defined inline: the gates build frameworks/native/src/mini/call_manager_proxy.cpp only.

#include <memory>

#include "call_manager_callback.h"
#include "i_call_ability_callback.h"
#include "telephony_errors.h"

namespace OHOS {
namespace Telephony {
class CallAbilityCallback : public ICallAbilityCallback {
public:
    CallAbilityCallback() = default;
    ~CallAbilityCallback() override = default;

    void SetProcessCallback(std::unique_ptr<CallManagerCallback> callback)
    {
        callbackPtr_ = std::move(callback);
    }
    void Close()
    {
        closed_ = true;
        callbackPtr_ = nullptr;
    }
    int32_t OnCallDetailsChange(const CallAttributeInfo &info) override
    {
        return (closed_ || callbackPtr_ == nullptr) ? TELEPHONY_ERR_LOCAL_PTR_NULL :
            callbackPtr_->OnCallDetailsChange(info);
    }
    int32_t OnCallEventChange(const CallEventInfo &info) override
    {
        return (closed_ || callbackPtr_ == nullptr) ? TELEPHONY_ERR_LOCAL_PTR_NULL :
            callbackPtr_->OnCallEventChange(info);
    }
    int32_t OnCallDisconnectedCause(const DisconnectedDetails &details) override
    {
        return (closed_ || callbackPtr_ == nullptr) ? TELEPHONY_ERR_LOCAL_PTR_NULL :
            callbackPtr_->OnCallDisconnectedCause(details);
    }

private:
    std::unique_ptr<CallManagerCallback> callbackPtr_;
    bool closed_ = false;
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_ABILITY_CALLBACK_H
