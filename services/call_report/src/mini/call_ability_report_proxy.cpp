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

// No death recipients (callers are in-process) and no Bluetooth mirror, MMI, OTT, audio-device, video or
// RTT reports.

#include "call_ability_report_proxy.h"

#include <cstring>

#include "call_manager_errors.h"
#include "los_event_handler.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
CallAbilityReportProxy::CallAbilityReportProxy() {}

CallAbilityReportProxy::~CallAbilityReportProxy() {}

int32_t CallAbilityReportProxy::RegisterCallBack(sptr<ICallAbilityCallback> callback, const std::string &bundleInfo)
{
    if (callback == nullptr) {
        TELEPHONY_LOGE("callAbilityCallbackPtr is null");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    if (callbackPtr_ != nullptr) {
        TELEPHONY_LOGE("a call ability callback is already registered");
        return TELEPHONY_ERR_REGISTER_CALLBACK_FAIL;
    }
    callback->SetBundleInfo(bundleInfo);
    callbackPtr_ = callback;
    return TELEPHONY_SUCCESS;
}

int32_t CallAbilityReportProxy::UnRegisterCallBack(const std::string &bundleInfo)
{
    if (callbackPtr_ == nullptr) {
        TELEPHONY_LOGE("no callback registered, UnRegisterCallBack failed");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    if (callbackPtr_->GetBundleInfo() == bundleInfo) {
        callbackPtr_ = nullptr;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallAbilityReportProxy::UnRegisterCallBack(const sptr<ICallAbilityCallback> &callback)
{
    if (callback != nullptr && callbackPtr_ == callback) {
        callbackPtr_ = nullptr;
    }
    return TELEPHONY_SUCCESS;
}

void CallAbilityReportProxy::CallStateUpdated(
    sptr<CallBase> &callObjectPtr, TelCallState priorState, TelCallState nextState)
{
    if (callObjectPtr == nullptr) {
        TELEPHONY_LOGE("callObjectPtr is nullptr!");
        return;
    }
    CallAttributeInfo info;
    callObjectPtr->GetCallAttributeInfo(info);
    size_t accountLen = strlen(info.accountNumber);
    if (accountLen > static_cast<size_t>(kMaxNumberLen)) {
        accountLen = kMaxNumberLen;
    }
    for (size_t i = 0; i < accountLen; i++) {
        if (info.accountNumber[i] == ',' || info.accountNumber[i] == ';') {
            info.accountNumber[i] = '\0';
            break;
        }
    }
    if (nextState == TelCallState::CALL_STATUS_ANSWERED) {
        info.callState = TelCallState::CALL_STATUS_ANSWERED;
    }
    ReportCallStateInfo(info);
}

void CallAbilityReportProxy::CallEventUpdated(CallEventInfo &info)
{
    ReportCallEvent(info);
}

void CallAbilityReportProxy::CallDestroyed(const DisconnectedDetails &details)
{
    sptr<ICallAbilityCallback> callback = callbackPtr_;
    if (callback == nullptr) {
        return;
    }
    int32_t ret = TelMiniMainLoop().PostAsyncTask([callback, details]() {
        int32_t result = callback->OnCallDisconnectedCause(details);
        if (result != TELEPHONY_SUCCESS) {
            TELEPHONY_LOGW("OnCallDisconnectedCause failed, errcode:%{public}d", result);
        }
    });
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("disconnect cause report dropped: %{public}d", ret);
    }
}

int32_t CallAbilityReportProxy::ReportCallStateInfo(const CallAttributeInfo &info)
{
    sptr<ICallAbilityCallback> callback = callbackPtr_;
    if (callback == nullptr) {
        return TELEPHONY_ERR_FAIL;
    }
    int32_t ret = TelMiniMainLoop().PostAsyncTask([callback, info]() {
        int32_t result = callback->OnCallDetailsChange(info);
        if (result != TELEPHONY_SUCCESS) {
            TELEPHONY_LOGD("OnCallDetailsChange failed, errcode:%{public}d", result);
        }
    });
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("call state report dropped: callId[%{public}d] state[%{public}d] %{public}d",
            info.callId, info.callState, ret);
        return ret;
    }
    TELEPHONY_LOGI("report call state info, callId[%{public}d] state[%{public}d]", info.callId, info.callState);
    return TELEPHONY_SUCCESS;
}

int32_t CallAbilityReportProxy::ReportCallStateInfo(const CallAttributeInfo &info, std::string bundleInfo)
{
    if (callbackPtr_ == nullptr || callbackPtr_->GetBundleInfo() != bundleInfo) {
        return TELEPHONY_ERROR;
    }
    return ReportCallStateInfo(info);
}

int32_t CallAbilityReportProxy::ReportCallEvent(const CallEventInfo &info)
{
    sptr<ICallAbilityCallback> callback = callbackPtr_;
    if (callback == nullptr) {
        return TELEPHONY_ERR_FAIL;
    }
    int32_t ret = TelMiniMainLoop().PostAsyncTask([callback, info]() {
        int32_t result = callback->OnCallEventChange(info);
        if (result != TELEPHONY_SUCCESS) {
            TELEPHONY_LOGW("OnCallEventChange failed, errcode:%{public}d", result);
        }
    });
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("call event report dropped: %{public}d", ret);
    }
    return ret;
}
} // namespace Telephony
} // namespace OHOS
