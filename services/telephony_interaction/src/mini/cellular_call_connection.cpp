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

#include "cellular_call_connection.h"

#include "call_manager_errors.h"
#include "call_status_callback_guard.h"
#include "cellular_call_lite_interface.h"
#include "los_event_handler.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
namespace {
sptr<CellularCallLiteInterface> GetStack(const char *method)
{
    sptr<CellularCallLiteInterface> stack = CellularCallLiteRegistry::Get();
    if (stack == nullptr) {
        TELEPHONY_LOGE("%{public}s: cellular call lite is not registered", method);
    }
    return stack;
}

int32_t NotSupported(const char *method)
{
    TELEPHONY_LOGW("%{public}s is not supported on the mini system", method);
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}
} // namespace

CellularCallConnection::CellularCallConnection() {}

CellularCallConnection::~CellularCallConnection() {}

void CellularCallConnection::Init(int32_t systemAbilityId)
{
    if (initialized_) {
        return;
    }
    initialized_ = true;
    if (!CellularCallLiteRegistry::AddReadyListener(OnStackReady)) {
        TELEPHONY_LOGE("cannot watch for the cellular call lite registration");
    }
}

void CellularCallConnection::UnInit()
{
    CellularCallLiteRegistry::RemoveReadyListener(OnStackReady);
    initialized_ = false;
    if (callbackRegistered_) {
        sptr<CellularCallLiteInterface> stack = CellularCallLiteRegistry::Get();
        if (stack != nullptr) {
            stack->UnRegisterCallManagerCallBack();
        }
        callbackRegistered_ = false;
    }
    callback_ = nullptr;
}

bool CellularCallConnection::IsCallbackRegistered() const
{
    return callbackRegistered_;
}

void CellularCallConnection::OnStackReady()
{
    int32_t ret = TelMiniMainLoop().PostAsyncTask([]() {
        DelayedSingleton<CellularCallConnection>::GetInstance()->RegisterCallbackOnLoop();
    });
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("cannot schedule the uplink registration: %{public}d", ret);
    }
}

void CellularCallConnection::RegisterCallbackOnLoop()
{
    if (!initialized_ || callbackRegistered_) {
        return;
    }
    sptr<CellularCallLiteInterface> stack = GetStack(__func__);
    if (stack == nullptr) {
        return;
    }
    if (callback_ == nullptr) {
        callback_ = new (std::nothrow) CallStatusCallbackGuard();
        if (callback_ == nullptr) {
            TELEPHONY_LOGE("no memory for the uplink callback");
            return;
        }
    }
    int32_t ret = stack->RegisterCallManagerCallBack(callback_);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("uplink registration failed: %{public}d", ret);
        return;
    }
    callbackRegistered_ = true;
    TELEPHONY_LOGI("uplink registered with cellular call lite");
}

int32_t CellularCallConnection::Dial(const CellularCallInfo &callInfo)
{
    sptr<CellularCallLiteInterface> stack = GetStack(__func__);
    return (stack == nullptr) ? TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL : stack->Dial(callInfo);
}

int32_t CellularCallConnection::HangUp(const CellularCallInfo &callInfo, CallSupplementType type)
{
    sptr<CellularCallLiteInterface> stack = GetStack(__func__);
    return (stack == nullptr) ? TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL : stack->HangUp(callInfo, type);
}

int32_t CellularCallConnection::Reject(const CellularCallInfo &callInfo)
{
    sptr<CellularCallLiteInterface> stack = GetStack(__func__);
    return (stack == nullptr) ? TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL : stack->Reject(callInfo);
}

int32_t CellularCallConnection::Answer(const CellularCallInfo &callInfo)
{
    sptr<CellularCallLiteInterface> stack = GetStack(__func__);
    return (stack == nullptr) ? TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL : stack->Answer(callInfo);
}

int32_t CellularCallConnection::HoldCall(const CellularCallInfo &callInfo)
{
    sptr<CellularCallLiteInterface> stack = GetStack(__func__);
    return (stack == nullptr) ? TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL : stack->HoldCall(callInfo);
}

int32_t CellularCallConnection::UnHoldCall(const CellularCallInfo &callInfo)
{
    sptr<CellularCallLiteInterface> stack = GetStack(__func__);
    return (stack == nullptr) ? TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL : stack->UnHoldCall(callInfo);
}

int32_t CellularCallConnection::SwitchCall(const CellularCallInfo &callInfo)
{
    sptr<CellularCallLiteInterface> stack = GetStack(__func__);
    return (stack == nullptr) ? TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL : stack->SwitchCall(callInfo);
}

int32_t CellularCallConnection::IsEmergencyPhoneNumber(const std::string &phoneNum, int32_t slotId, bool &enabled)
{
    sptr<CellularCallLiteInterface> stack = GetStack(__func__);
    return (stack == nullptr) ? TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL :
        stack->IsEmergencyPhoneNumber(slotId, phoneNum, enabled);
}

int32_t CellularCallConnection::HangUpAllConnection()
{
    sptr<CellularCallLiteInterface> stack = GetStack(__func__);
    return (stack == nullptr) ? TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL : stack->HangUpAllConnection();
}

int32_t CellularCallConnection::CombineConference(const CellularCallInfo &callInfo)
{
    return NotSupported(__func__);
}

int32_t CellularCallConnection::SeparateConference(const CellularCallInfo &callInfo)
{
    return NotSupported(__func__);
}

int32_t CellularCallConnection::KickOutFromConference(const CellularCallInfo &callInfo)
{
    return NotSupported(__func__);
}

int32_t CellularCallConnection::StartDtmf(char cDTMFCode, const CellularCallInfo &callInfo)
{
    return NotSupported(__func__);
}

int32_t CellularCallConnection::StopDtmf(const CellularCallInfo &callInfo)
{
    return NotSupported(__func__);
}

int32_t CellularCallConnection::PostDialProceed(const CellularCallInfo &callInfo, const bool proceed)
{
    return NotSupported(__func__);
}

int32_t CellularCallConnection::SetMute(int32_t mute, int32_t slotId)
{
    return NotSupported(__func__);
}

int32_t CellularCallConnection::GetVideoCallWaiting(int32_t slotId, bool &enabled)
{
    return NotSupported(__func__);
}

int32_t CellularCallConnection::GetCarrierVtConfig(int32_t slotId, bool &enabled)
{
    return NotSupported(__func__);
}

bool CellularCallConnection::IsMmiCode(int32_t slotId, std::string &number)
{
    return false;
}
} // namespace Telephony
} // namespace OHOS
