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

#include "call_request_handler.h"

#include "call_manager_errors.h"
#include "los_event_handler.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
CallRequestHandler::CallRequestHandler() {}

CallRequestHandler::~CallRequestHandler() {}

void CallRequestHandler::Init()
{
    callRequestProcessPtr_ = std::make_shared<CallRequestProcess>();
}

int32_t CallRequestHandler::DialCall()
{
    if (callRequestProcessPtr_ == nullptr) {
        TELEPHONY_LOGE("callRequestProcessPtr_ is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return callRequestProcessPtr_->DialRequest();
}

int32_t CallRequestHandler::AnswerCall(int32_t callId, int32_t videoState, bool isRTT)
{
    if (callRequestProcessPtr_ == nullptr) {
        TELEPHONY_LOGE("callRequestProcessPtr_ is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::shared_ptr<CallRequestProcess> process = callRequestProcessPtr_;
    return TelMiniMainLoop().PostAsyncTask([process, callId, videoState, isRTT]() {
        process->AnswerRequest(callId, videoState, isRTT);
    });
}

int32_t CallRequestHandler::RejectCall(int32_t callId, bool isSendSms, std::string &content)
{
    if (callRequestProcessPtr_ == nullptr) {
        TELEPHONY_LOGE("callRequestProcessPtr_ is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::shared_ptr<CallRequestProcess> process = callRequestProcessPtr_;
    std::string mContent = content;
    return TelMiniMainLoop().PostAsyncTask([process, callId, isSendSms, mContent]() {
        std::string text = mContent;
        process->RejectRequest(callId, isSendSms, text);
    });
}

int32_t CallRequestHandler::HangUpCall(int32_t callId)
{
    if (callRequestProcessPtr_ == nullptr) {
        TELEPHONY_LOGE("callRequestProcessPtr_ is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::shared_ptr<CallRequestProcess> process = callRequestProcessPtr_;
    return TelMiniMainLoop().PostAsyncTask([process, callId]() { process->HangUpRequest(callId); });
}

int32_t CallRequestHandler::HoldCall(int32_t callId)
{
    if (callRequestProcessPtr_ == nullptr) {
        TELEPHONY_LOGE("callRequestProcessPtr_ is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::shared_ptr<CallRequestProcess> process = callRequestProcessPtr_;
    return TelMiniMainLoop().PostAsyncTask([process, callId]() { process->HoldRequest(callId); });
}

int32_t CallRequestHandler::UnHoldCall(int32_t callId)
{
    if (callRequestProcessPtr_ == nullptr) {
        TELEPHONY_LOGE("callRequestProcessPtr_ is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::shared_ptr<CallRequestProcess> process = callRequestProcessPtr_;
    return TelMiniMainLoop().PostAsyncTask([process, callId]() { process->UnHoldRequest(callId); });
}

int32_t CallRequestHandler::SwitchCall(int32_t callId)
{
    if (callRequestProcessPtr_ == nullptr) {
        TELEPHONY_LOGE("callRequestProcessPtr_ is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::shared_ptr<CallRequestProcess> process = callRequestProcessPtr_;
    return TelMiniMainLoop().PostAsyncTask([process, callId]() { process->SwitchRequest(callId); });
}
} // namespace Telephony
} // namespace OHOS
