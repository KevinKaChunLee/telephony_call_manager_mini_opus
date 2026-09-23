/*
 * Copyright (C) 2021-2026 Huawei Device Co., Ltd.
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

#include "call_status_policy.h"

#include "call_control_manager.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
CallStatusPolicy::CallStatusPolicy() {}

CallStatusPolicy::~CallStatusPolicy() {}

int32_t CallStatusPolicy::DialingHandlePolicy(const CallDetailInfo &info)
{
    std::string number(info.phoneNum);
    if (IsCallExist(number)) {
        TELEPHONY_LOGW("this call has created yet!");
        return CALL_ERR_CALL_ALREADY_EXISTS;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallStatusPolicy::FilterResultsDispose(sptr<CallBase> call)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    if (call == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return CALL_ERR_CALL_OBJECT_IS_NULL;
    }
    if (HasRingingMaximum() || HasDialingMaximum()) {
        call->SetApCauseReported(true);
        ret = call->HangUpCall();
        if (ret != TELEPHONY_SUCCESS) {
            TELEPHONY_LOGE("HangUpCall failed!");
            return ret;
        }
        return TELEPHONY_SUCCESS;
    }
    ret = call->IncomingCallBase();
    DelayedSingleton<CallControlManager>::GetInstance()->NotifyNewCallCreated(call);
    return TELEPHONY_SUCCESS;
}
} // namespace Telephony
} // namespace OHOS
