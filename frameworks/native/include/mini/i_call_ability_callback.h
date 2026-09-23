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

#ifndef I_CALL_ABILITY_CALLBACK_H
#define I_CALL_ABILITY_CALLBACK_H

// Mini variant of frameworks/native/include/i_call_ability_callback.h: an in-process interface with the
// reports the Mini call manager produces. Calls arrive on the main loop, one posted task per report.

#include "tel_mini_std_includes.h"

#include "refbase.h"
#include "call_manager_disconnected_details.h"
#include "call_manager_info.h"

namespace OHOS {
namespace Telephony {
class ICallAbilityCallback : public virtual RefBase {
public:
    virtual ~ICallAbilityCallback() = default;
    virtual int32_t OnCallDetailsChange(const CallAttributeInfo &info) = 0;
    virtual int32_t OnCallEventChange(const CallEventInfo &info) = 0;
    virtual int32_t OnCallDisconnectedCause(const DisconnectedDetails &details) = 0;

    void SetBundleInfo(const std::string &info)
    {
        bundleInfo_ = info;
    }
    std::string GetBundleInfo() const
    {
        return bundleInfo_;
    }

private:
    std::string bundleInfo_;
};
} // namespace Telephony
} // namespace OHOS
#endif // I_CALL_ABILITY_CALLBACK_H
