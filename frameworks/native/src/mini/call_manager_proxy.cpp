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

#include "call_manager_proxy.h"

#include "call_ability_report_proxy.h"
#include "call_manager_errors.h"
#include "call_manager_mini_config.h"
#include "call_manager_service.h"
#include "los_event_handler.h"

namespace OHOS {
namespace Telephony {
namespace {
int32_t NotSupported(const char *method)
{
    TELEPHONY_LOGW("%{public}s is not supported on the mini system", method);
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t ServiceNotReady(const char *method)
{
    TELEPHONY_LOGE("%{public}s: call manager service is not ready", method);
    return TELEPHONY_ERR_UNINIT;
}
} // namespace

CallManagerProxy::CallManagerProxy() {}

CallManagerProxy::~CallManagerProxy() {}

template <typename R>
int32_t CallManagerProxy::RunOnService(const char *method, std::function<R()> call, R &result, R notReady)
{
    if (!initStatus_.load()) {
        result = notReady;
        return ServiceNotReady(method);
    }
    int32_t ret = TelMiniMainLoop().PostSyncCall<R>([call, notReady]() {
        std::shared_ptr<CallManagerService> service = DelayedSingleton<CallManagerService>::GetInstance();
        if (service == nullptr || !service->IsReady()) {
            return notReady;
        }
        return call();
    }, result, MINI_SYNC_WAIT_MS);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("%{public}s did not reach the call manager: %{public}d", method, ret);
        result = notReady;
    }
    return ret;
}

void CallManagerProxy::Init(int32_t systemAbilityId)
{
    initStatus_.store(true);
}

void CallManagerProxy::UnInit()
{
    if (!initStatus_.exchange(false)) {
        return;
    }
    TelMiniMainLoop().PostSyncTask([this]() {
        if (callAbilityCallbackPtr_ != nullptr) {
            callAbilityCallbackPtr_->Close();
            sptr<ICallAbilityCallback> callback = callAbilityCallbackPtr_;
            DelayedSingleton<CallAbilityReportProxy>::GetInstance()->UnRegisterCallBack(callback);
        }
        callAbilityCallbackPtr_ = nullptr;
        registerStatus_ = false;
    }, MINI_SYNC_WAIT_MS);
}

int32_t CallManagerProxy::RegisterCallBack(std::unique_ptr<CallManagerCallback> callback)
{
    if (!initStatus_.load()) {
        return ServiceNotReady(__func__);
    }
    if (callback == nullptr) {
        TELEPHONY_LOGE("callback is null");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    sptr<CallAbilityCallback> abilityCallback = new (std::nothrow) CallAbilityCallback();
    if (abilityCallback == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    abilityCallback->SetProcessCallback(std::move(callback));
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, [this, abilityCallback]() {
        if (registerStatus_) {
            TELEPHONY_LOGE("you have already register callback yet!");
            return static_cast<int32_t>(TELEPHONY_ERR_REGISTER_CALLBACK_FAIL);
        }
        sptr<ICallAbilityCallback> callbackPtr = abilityCallback;
        int32_t serviceRet = DelayedSingleton<CallManagerService>::GetInstance()->RegisterCallBack(callbackPtr);
        if (serviceRet == TELEPHONY_ERR_PERMISSION_ERR || serviceRet == TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API) {
            return serviceRet;
        }
        if (serviceRet != TELEPHONY_SUCCESS) {
            return static_cast<int32_t>(TELEPHONY_ERR_REGISTER_CALLBACK_FAIL);
        }
        callAbilityCallbackPtr_ = abilityCallback;
        registerStatus_ = true;
        return static_cast<int32_t>(TELEPHONY_SUCCESS);
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::UnRegisterCallBack()
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, [this]() {
        if (!registerStatus_) {
            TELEPHONY_LOGE("you haven't register callback yet, please RegisterCallBack first!");
            return static_cast<int32_t>(TELEPHONY_ERR_REGISTER_CALLBACK_FAIL);
        }
        int32_t serviceRet = DelayedSingleton<CallManagerService>::GetInstance()->UnRegisterCallBack();
        if (serviceRet == TELEPHONY_ERR_PERMISSION_ERR || serviceRet == TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API) {
            return serviceRet;
        }
        if (serviceRet != TELEPHONY_SUCCESS) {
            return static_cast<int32_t>(TELEPHONY_ERR_UNREGISTER_CALLBACK_FAIL);
        }
        if (callAbilityCallbackPtr_ != nullptr) {
            callAbilityCallbackPtr_->Close();
        }
        callAbilityCallbackPtr_ = nullptr;
        registerStatus_ = false;
        return static_cast<int32_t>(TELEPHONY_SUCCESS);
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::ObserverOnCallDetailsChange()
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, []() {
        return DelayedSingleton<CallManagerService>::GetInstance()->ObserverOnCallDetailsChange();
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::DialCall(std::u16string number, AppExecFwk::PacMap &extras)
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    AppExecFwk::PacMap dialExtras = extras;
    int32_t ret = RunOnService<int32_t>(__func__, [number, dialExtras]() mutable {
        return DelayedSingleton<CallManagerService>::GetInstance()->DialCall(number, dialExtras);
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::MakeCall(std::string number)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::MakeCallWithToken(std::string number, AppExecFwk::PacMap &options, std::string &token)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::AnswerCall(int32_t callId, int32_t videoState, bool isRTT)
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, [callId, videoState, isRTT]() {
        return DelayedSingleton<CallManagerService>::GetInstance()->AnswerCall(callId, videoState, isRTT);
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::RejectCall(int32_t callId, bool isSendSms, std::u16string content)
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, [callId, isSendSms, content]() {
        return DelayedSingleton<CallManagerService>::GetInstance()->RejectCall(callId, isSendSms, content);
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::HangUpCall(int32_t callId)
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, [callId]() {
        return DelayedSingleton<CallManagerService>::GetInstance()->HangUpCall(callId);
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::GetCallState()
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, []() {
        return DelayedSingleton<CallManagerService>::GetInstance()->GetCallState();
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::HoldCall(int32_t callId)
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, [callId]() {
        return DelayedSingleton<CallManagerService>::GetInstance()->HoldCall(callId);
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::UnHoldCall(int32_t callId)
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, [callId]() {
        return DelayedSingleton<CallManagerService>::GetInstance()->UnHoldCall(callId);
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::SwitchCall(int32_t callId)
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, [callId]() {
        return DelayedSingleton<CallManagerService>::GetInstance()->SwitchCall(callId);
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::CombineConference(int32_t callId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SeparateConference(int32_t callId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::KickOutFromConference(int32_t callId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::GetMainCallId(int32_t &callId, int32_t &mainCallId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::GetSubCallIdList(int32_t callId, std::vector<std::u16string> &callIdList)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::GetCallIdListForConference(int32_t callId, std::vector<std::u16string> &callIdList)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::GetCallWaiting(int32_t slotId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetCallWaiting(int32_t slotId, bool activate)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::GetCallRestriction(int32_t slotId, CallRestrictionType type)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetCallRestriction(int32_t slotId, CallRestrictionInfo &info)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetCallRestrictionPassword(
    int32_t slotId, CallRestrictionType fac, const char *oldPassword, const char *newPassword)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::GetCallTransferInfo(int32_t slotId, CallTransferType type)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetCallTransferInfo(int32_t slotId, CallTransferInfo &info)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::CanSetCallTransferTime(int32_t slotId, bool &result)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetCallPreferenceMode(int32_t slotId, int32_t mode)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::StartDtmf(int32_t callId, char str)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::StopDtmf(int32_t callId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::PostDialProceed(int32_t callId, bool proceed)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::IsRinging(bool &enabled)
{
    std::pair<int32_t, bool> result(TELEPHONY_ERR_UNINIT, false);
    int32_t ret = RunOnService<std::pair<int32_t, bool>>(__func__, []() {
        bool value = false;
        int32_t code = DelayedSingleton<CallManagerService>::GetInstance()->IsRinging(value);
        return std::make_pair(code, value);
    }, result, std::make_pair(static_cast<int32_t>(TELEPHONY_ERR_UNINIT), false));
    enabled = result.second;
    return (ret == TELEPHONY_SUCCESS) ? result.first : ret;
}

bool CallManagerProxy::HasCall(const bool isInCludeVoipCall)
{
    bool result = false;
    RunOnService<bool>(__func__, [isInCludeVoipCall]() {
        return DelayedSingleton<CallManagerService>::GetInstance()->HasCall(isInCludeVoipCall);
    }, result, false);
    return result;
}

int32_t CallManagerProxy::IsNewCallAllowed(bool &enabled)
{
    std::pair<int32_t, bool> result(TELEPHONY_ERR_UNINIT, false);
    int32_t ret = RunOnService<std::pair<int32_t, bool>>(__func__, []() {
        bool value = false;
        int32_t code = DelayedSingleton<CallManagerService>::GetInstance()->IsNewCallAllowed(value);
        return std::make_pair(code, value);
    }, result, std::make_pair(static_cast<int32_t>(TELEPHONY_ERR_UNINIT), false));
    enabled = result.second;
    return (ret == TELEPHONY_SUCCESS) ? result.first : ret;
}

int32_t CallManagerProxy::RegisterVoipCallManagerCallback()
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::UnRegisterVoipCallManagerCallback()
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::IsInEmergencyCall(bool &enabled)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::IsEmergencyPhoneNumber(std::u16string &number, int32_t slotId, bool &enabled)
{
    std::pair<int32_t, bool> result(TELEPHONY_ERR_UNINIT, false);
    std::u16string phoneNumber = number;
    int32_t ret = RunOnService<std::pair<int32_t, bool>>(__func__, [phoneNumber, slotId]() {
        bool value = false;
        std::u16string numberCopy = phoneNumber;
        int32_t code =
            DelayedSingleton<CallManagerService>::GetInstance()->IsEmergencyPhoneNumber(numberCopy, slotId, value);
        return std::make_pair(code, value);
    }, result, std::make_pair(static_cast<int32_t>(TELEPHONY_ERR_UNINIT), false));
    enabled = result.second;
    return (ret == TELEPHONY_SUCCESS) ? result.first : ret;
}

int32_t CallManagerProxy::FormatPhoneNumber(
    std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::FormatPhoneNumberToE164(
    std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetMuted(bool isMute)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::MuteRinger()
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetAudioDevice(const AudioDevice &audioDevice)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::ControlCamera(int32_t callId, std::u16string &cameraId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetPreviewWindow(int32_t callId, std::string &surfaceId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetDisplayWindow(int32_t callId, std::string &surfaceId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetCameraZoom(float zoomRatio)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetPausePicture(int32_t callId, std::u16string &path)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetDeviceDirection(int32_t callId, int32_t rotation)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::GetImsConfig(int32_t slotId, ImsConfigItem item)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetImsConfig(int32_t slotId, ImsConfigItem item, std::u16string &value)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::GetImsFeatureValue(int32_t slotId, FeatureType type)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetImsFeatureValue(int32_t slotId, FeatureType type, int32_t value)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::UpdateImsCallMode(int32_t callId, ImsCallMode mode)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::EnableImsSwitch(int32_t slotId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::DisableImsSwitch(int32_t slotId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::IsImsSwitchEnabled(int32_t slotId, bool &enabled)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetVoNRState(int32_t slotId, int32_t state)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::GetVoNRState(int32_t slotId, int32_t &state)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::JoinConference(int32_t callId, std::vector<std::u16string> &numberList)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::ReportOttCallDetailsInfo(std::vector<OttCallDetailsInfo> &ottVec)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::ReportOttCallEventInfo(OttCallEventInfo &eventInfo)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::CloseUnFinishedUssd(int32_t slotId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::InputDialerSpecialCode(const std::string &specialCode)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::RemoveMissedIncomingCallNotification()
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetVoIPCallState(int32_t state)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::GetVoIPCallState(int32_t &state)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetVoIPCallInfo(int32_t callId, int32_t state, std::string phoneNumber)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::GetVoIPCallInfo(int32_t &callId, int32_t &state, std::string &phoneNumber)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetRegMmiCodeCallbackState(bool isReg)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::ReportAudioDeviceInfo()
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::CancelCallUpgrade(int32_t callId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::RequestCameraCapabilities(int32_t callId)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SendCallUiEvent(int32_t callId, std::string &eventName)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::PreloadCallUi(bool enable)
{
    return NotSupported(__func__);
}

sptr<ICallStatusCallback> CallManagerProxy::RegisterBluetoothCallManagerCallbackPtr(
    int32_t phoneIndex, std::string &macAddress)
{
    NotSupported(__func__);
    return nullptr;
}

int32_t CallManagerProxy::SendUssdResponse(int32_t slotId, std::string &content)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::SetCallPolicyInfo(bool isDialingTrustlist, const std::vector<std::string> &dialingList,
    bool isIncomingTrustlist, const std::vector<std::string> &incomingList)
{
    return NotSupported(__func__);
}

bool CallManagerProxy::EndCall()
{
    bool result = false;
    RunOnService<bool>(__func__, []() {
        return DelayedSingleton<CallManagerService>::GetInstance()->EndCall();
    }, result, false);
    return result;
}

bool CallManagerProxy::HasDistributedCommunicationCapability()
{
    NotSupported(__func__);
    return false;
}

int32_t CallManagerProxy::NotifyVoIPAudioStreamStart(int32_t uid)
{
    return NotSupported(__func__);
}

bool CallManagerProxy::CheckCallRecordingPermission(
    const std::string &cellularRecordPhoneNum, const std::string &cellularRecordToken)
{
    NotSupported(__func__);
    return false;
}

int32_t CallManagerProxy::SetCallAudioMode(int32_t mode, int32_t scenarios)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::AnswerCall()
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, []() {
        return DelayedSingleton<CallManagerService>::GetInstance()->AnswerCall();
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::RejectCall()
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, []() {
        return DelayedSingleton<CallManagerService>::GetInstance()->RejectCall();
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::RejectCall(RejectType rejectType)
{
    return NotSupported(__func__);
}

int32_t CallManagerProxy::HangUpCall()
{
    int32_t result = TELEPHONY_ERR_UNINIT;
    int32_t ret = RunOnService<int32_t>(__func__, []() {
        return DelayedSingleton<CallManagerService>::GetInstance()->HangUpCall();
    }, result, TELEPHONY_ERR_UNINIT);
    return (ret == TELEPHONY_SUCCESS) ? result : ret;
}

int32_t CallManagerProxy::GetCallTransferInfo(const std::string number, CallTransferType type)
{
    return NotSupported(__func__);
}
} // namespace Telephony
} // namespace OHOS
