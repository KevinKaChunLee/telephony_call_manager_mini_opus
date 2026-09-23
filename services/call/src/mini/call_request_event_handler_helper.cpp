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

#include "call_request_event_handler_helper.h"

#include "telephony_errors.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
namespace {
constexpr uint32_t DELAY_TIME_MS = 3000;
} // namespace

CallRequestEventHandlerHelper::CallRequestEventHandlerHelper() {}

CallRequestEventHandlerHelper::~CallRequestEventHandlerHelper() {}

int32_t CallRequestEventHandlerHelper::SetDialingCallProcessing()
{
    if (!IsDialingCallProcessing()) {
        return TELEPHONY_ERROR;
    }
    RemoveEventHandlerTask();
    int32_t ret = TelMiniMainLoop().PostAsyncTask([]() {
        auto helper = DelayedSingleton<CallRequestEventHandlerHelper>::GetInstance();
        helper->restoreTaskId_ = TEL_MINI_INVALID_TASK_ID;
        helper->RestoreDialingFlag(false);
    }, DELAY_TIME_MS, &restoreTaskId_);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("restore dialing flag task failed: %{public}d", ret);
        return TELEPHONY_ERROR;
    }
    return TELEPHONY_SUCCESS;
}

void CallRequestEventHandlerHelper::RemoveEventHandlerTask()
{
    if (restoreTaskId_ != TEL_MINI_INVALID_TASK_ID) {
        TelMiniMainLoop().RemoveAsyncTask(restoreTaskId_);
        restoreTaskId_ = TEL_MINI_INVALID_TASK_ID;
    }
}

void CallRequestEventHandlerHelper::RestoreDialingFlag(bool isDialingCallProcessing)
{
    isDialingCallProcessing_ = isDialingCallProcessing;
}

bool CallRequestEventHandlerHelper::IsDialingCallProcessing()
{
    return isDialingCallProcessing_;
}

void CallRequestEventHandlerHelper::SetPendingMo(bool pendingMo, int32_t callId)
{
    pendingMo_ = pendingMo;
    pendingMoCallId_ = callId;
}

bool CallRequestEventHandlerHelper::HasPendingMo(int32_t callId)
{
    return pendingMo_ && (pendingMoCallId_ == callId);
}

bool CallRequestEventHandlerHelper::IsPendingHangup()
{
    if (pendingHangupCallId_ != -1) {
        return pendingHangup_;
    }
    return false;
}

int32_t CallRequestEventHandlerHelper::GetPendingHangupCallId()
{
    if (pendingHangup_) {
        return pendingHangupCallId_;
    }
    return -1;
}

void CallRequestEventHandlerHelper::SetPendingHangup(bool pendingHangup, int32_t callId)
{
    pendingHangup_ = pendingHangup;
    pendingHangupCallId_ = callId;
}

bool CallRequestEventHandlerHelper::HasPendingHangup(int32_t callId)
{
    return pendingHangup_ && (pendingHangupCallId_ == callId);
}
} // namespace Telephony
} // namespace OHOS
