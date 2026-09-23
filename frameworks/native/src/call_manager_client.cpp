/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#include "call_manager_client.h"

#include <memory>

#include "call_manager_proxy.h"
#include "parameter.h"
#include "telephony_errors.h"

namespace OHOS {
namespace Telephony {
std::shared_ptr<CallManagerProxy> g_callManagerProxy = nullptr;

const std::string KEY_VOICECALL_CAP = "const.telephony.voice.capable";
const int32_t VOICECALL_CAP_VAL_LEN = 6;

CallManagerClient::CallManagerClient() {}

CallManagerClient::~CallManagerClient() {}

void CallManagerClient::Init(int32_t systemAbilityId)
{
    if (!isInit_.load()) {
        g_callManagerProxy = DelayedSingleton<CallManagerProxy>::GetInstance();
        if (g_callManagerProxy == nullptr) {
            TELEPHONY_LOGE("g_callManagerProxy is nullptr");
            return;
        }
        g_callManagerProxy->Init(systemAbilityId);
        isInit_.store(true);
    }
    TELEPHONY_LOGI("CallManagerClient init success!");
}

void CallManagerClient::UnInit()
{
    if (isInit_.load()) {
        g_callManagerProxy->UnInit();
        isInit_.store(false);
    } else {
        TELEPHONY_LOGE("init first please!");
    }
}

int32_t CallManagerClient::RegisterCallBack(std::unique_ptr<CallManagerCallback> callback)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->RegisterCallBack(std::move(callback));
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::UnRegisterCallBack()
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->UnRegisterCallBack();
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::ObserverOnCallDetailsChange()
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->ObserverOnCallDetailsChange();
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::DialCall(std::u16string number, AppExecFwk::PacMap &extras)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->DialCall(number, extras);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::MakeCall(std::string number)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->MakeCall(number);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::MakeCallWithToken(std::string number, AppExecFwk::PacMap &options, std::string &token)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->MakeCallWithToken(number, options, token);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::AnswerCall(int32_t callId, int32_t videoState, bool isRTT)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->AnswerCall(callId, videoState, isRTT);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::RejectCall(int32_t callId, bool isSendSms, std::u16string content)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->RejectCall(callId, isSendSms, content);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::HangUpCall(int32_t callId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->HangUpCall(callId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::GetCallState()
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetCallState();
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::RegisterVoipCallManagerCallback()
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->RegisterVoipCallManagerCallback();
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::UnRegisterVoipCallManagerCallback()
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->UnRegisterVoipCallManagerCallback();
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::HoldCall(int32_t callId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->HoldCall(callId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::UnHoldCall(int32_t callId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->UnHoldCall(callId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SwitchCall(int32_t callId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SwitchCall(callId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::CombineConference(int32_t callId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->CombineConference(callId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SeparateConference(int32_t callId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SeparateConference(callId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::KickOutFromConference(int32_t callId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->KickOutFromConference(callId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::GetMainCallId(int32_t &callId, int32_t &mainCallId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetMainCallId(callId, mainCallId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::GetSubCallIdList(int32_t callId, std::vector<std::u16string> &callIdList)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetSubCallIdList(callId, callIdList);
    } else {
        TELEPHONY_LOGE("init first please!");
        callIdList.clear();
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::GetCallIdListForConference(int32_t callId, std::vector<std::u16string> &callIdList)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetCallIdListForConference(callId, callIdList);
    } else {
        TELEPHONY_LOGE("init first please!");
        callIdList.clear();
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::GetCallWaiting(int32_t slotId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetCallWaiting(slotId);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetCallWaiting(int32_t slotId, bool activate)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetCallWaiting(slotId, activate);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::GetCallRestriction(int32_t slotId, CallRestrictionType type)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetCallRestriction(slotId, type);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetCallRestriction(int32_t slotId, CallRestrictionInfo &info)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetCallRestriction(slotId, info);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetCallRestrictionPassword(
    int32_t slotId, CallRestrictionType fac, const char *oldPassword, const char *newPassword)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetCallRestrictionPassword(slotId, fac, oldPassword, newPassword);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::GetCallTransferInfo(int32_t slotId, CallTransferType type)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetCallTransferInfo(slotId, type);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetCallTransferInfo(int32_t slotId, CallTransferInfo &info)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetCallTransferInfo(slotId, info);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::CanSetCallTransferTime(int32_t slotId, bool &result)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->CanSetCallTransferTime(slotId, result);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetCallPreferenceMode(int32_t slotId, int32_t mode)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetCallPreferenceMode(slotId, mode);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::StartDtmf(int32_t callId, char str)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->StartDtmf(callId, str);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::StopDtmf(int32_t callId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->StopDtmf(callId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::PostDialProceed(int32_t callId, bool proceed)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->PostDialProceed(callId, proceed);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::IsRinging(bool &enabled)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->IsRinging(enabled);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

bool CallManagerClient::HasCall(const bool isInCludeVoipCall)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->HasCall(isInCludeVoipCall);
    } else {
        TELEPHONY_LOGE("init first please!");
        return false;
    }
}

int32_t CallManagerClient::IsNewCallAllowed(bool &enabled)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->IsNewCallAllowed(enabled);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::IsInEmergencyCall(bool &enabled)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->IsInEmergencyCall(enabled);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::IsEmergencyPhoneNumber(std::u16string &number, int32_t slotId, bool &enabled)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->IsEmergencyPhoneNumber(number, slotId, enabled);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::FormatPhoneNumber(
    std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->FormatPhoneNumber(number, countryCode, formatNumber);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::FormatPhoneNumberToE164(
    std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->FormatPhoneNumberToE164(number, countryCode, formatNumber);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetMuted(bool isMute)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetMuted(isMute);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::MuteRinger()
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->MuteRinger();
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetAudioDevice(const AudioDevice &audioDevice)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetAudioDevice(audioDevice);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::ControlCamera(int32_t callId, std::u16string cameraId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->ControlCamera(callId, cameraId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetPreviewWindow(int32_t callId, std::string &surfaceId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetPreviewWindow(callId, surfaceId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetDisplayWindow(int32_t callId, std::string &surfaceId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetDisplayWindow(callId, surfaceId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetCameraZoom(float zoomRatio)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetCameraZoom(zoomRatio);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetPausePicture(int32_t callId, std::u16string path)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetPausePicture(callId, path);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetDeviceDirection(int32_t callId, int32_t rotation)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetDeviceDirection(callId, rotation);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::GetImsConfig(int32_t slotId, ImsConfigItem item)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetImsConfig(slotId, item);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetImsConfig(int32_t slotId, ImsConfigItem item, std::u16string &value)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetImsConfig(slotId, item, value);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::GetImsFeatureValue(int32_t slotId, FeatureType type)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetImsFeatureValue(slotId, type);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetImsFeatureValue(int32_t slotId, FeatureType type, int32_t value)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetImsFeatureValue(slotId, type, value);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::UpdateImsCallMode(int32_t callId, ImsCallMode mode)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->UpdateImsCallMode(callId, mode);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::EnableImsSwitch(int32_t slotId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->EnableImsSwitch(slotId);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::DisableImsSwitch(int32_t slotId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->DisableImsSwitch(slotId);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::IsImsSwitchEnabled(int32_t slotId, bool &enabled)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->IsImsSwitchEnabled(slotId, enabled);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetVoNRState(int32_t slotId, int32_t state)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetVoNRState(slotId, state);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::GetVoNRState(int32_t slotId, int32_t &state)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetVoNRState(slotId, state);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::JoinConference(int32_t callId, std::vector<std::u16string> &numberList)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->JoinConference(callId, numberList);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::ReportOttCallDetailsInfo(std::vector<OttCallDetailsInfo> &ottVec)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->ReportOttCallDetailsInfo(ottVec);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::ReportOttCallEventInfo(OttCallEventInfo &eventInfo)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->ReportOttCallEventInfo(eventInfo);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::CloseUnFinishedUssd(int32_t slotId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->CloseUnFinishedUssd(slotId);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::InputDialerSpecialCode(const std::string &specialCode)
{
    if (g_callManagerProxy == nullptr) {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
    return g_callManagerProxy->InputDialerSpecialCode(specialCode);
}

int32_t CallManagerClient::RemoveMissedIncomingCallNotification()
{
    if (g_callManagerProxy == nullptr) {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
    return g_callManagerProxy->RemoveMissedIncomingCallNotification();
}

int32_t CallManagerClient::SetVoIPCallState(int32_t state)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetVoIPCallState(state);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetVoIPCallInfo(int32_t callId, int32_t state, std::string phoneNumber)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetVoIPCallInfo(callId, state, phoneNumber);
    }
    TELEPHONY_LOGE("init first please!");
    return TELEPHONY_ERR_UNINIT;
}

int32_t CallManagerClient::GetVoIPCallInfo(int32_t &callId, int32_t &state, std::string &phoneNumber)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetVoIPCallInfo(callId, state, phoneNumber);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
    return 0;
}

int32_t CallManagerClient::GetVoIPCallState(int32_t &state)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetVoIPCallState(state);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

bool CallManagerClient::HasVoiceCapability()
{
    char retValue[VOICECALL_CAP_VAL_LEN + 1] = {"true"};
    int retLen = GetParameter(KEY_VOICECALL_CAP.c_str(), "true", retValue, VOICECALL_CAP_VAL_LEN);
    TELEPHONY_LOGD("HasVoiceCapability retValue %{public}s, retLen %{public}d", retValue, retLen);
    if (strcmp(retValue, "true") == 0) {
        return true;
    }

    return false;
}

int32_t CallManagerClient::SetRegMmiCodeCallbackState(bool isReg)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetRegMmiCodeCallbackState(isReg);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::ReportAudioDeviceInfo()
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->ReportAudioDeviceInfo();
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::CancelCallUpgrade(int32_t callId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->CancelCallUpgrade(callId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::RequestCameraCapabilities(int32_t callId)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->RequestCameraCapabilities(callId);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::PreloadCallUi(bool enable)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->PreloadCallUi(enable);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SendCallUiEvent(int32_t callId, std::string &eventName)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SendCallUiEvent(callId, eventName);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

sptr<ICallStatusCallback> CallManagerClient::RegisterBluetoothCallManagerCallbackPtr(std::string &macAddress)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->RegisterBluetoothCallManagerCallbackPtr(0, macAddress);
    } else {
        TELEPHONY_LOGE("init first please!");
        return nullptr;
    }
}
 
sptr<ICallStatusCallback> CallManagerClient::RegisterTwoPhoneCallManagerCallbackPtr(int32_t phoneIndex,
    std::string &macAddress)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->RegisterBluetoothCallManagerCallbackPtr(phoneIndex, macAddress);
    } else {
        TELEPHONY_LOGE("init first please!");
        return nullptr;
    }
}

int32_t CallManagerClient::SendUssdResponse(int32_t slotId, std::string &content)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SendUssdResponse(slotId, content);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::SetCallPolicyInfo(bool isDialingTrustlist, const std::vector<std::string> &dialingList,
    bool isIncomingTrustlist, const std::vector<std::string> &incomingList)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetCallPolicyInfo(isDialingTrustlist,
            dialingList, isIncomingTrustlist, incomingList);
    }
    TELEPHONY_LOGE("init first please!");
    return TELEPHONY_ERR_UNINIT;
}

bool CallManagerClient::EndCall()
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->EndCall();
    } else {
        TELEPHONY_LOGE("init first please!");
        return false;
    }
}

int32_t CallManagerClient::NotifyVoIPAudioStreamStart(int32_t uid)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->NotifyVoIPAudioStreamStart(uid);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

#ifdef SUPPORT_RTT_CALL
int32_t CallManagerClient::SendRttMessage(int32_t callId, const std::string &rttMessage)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SendRttMessage(callId, rttMessage);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}
 
int32_t CallManagerClient::SetRttCapability(int32_t slotId, bool isEnable)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetRttCapability(slotId, isEnable);
    } else {
        TELEPHONY_LOGE("[slot%{public}d] init first please!", slotId);
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::UpdateImsRttCallMode(int32_t callId, ImsRTTCallMode mode)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->UpdateImsRttCallMode(callId, mode);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}
#endif

int32_t CallManagerClient::SetCallAudioMode(int32_t mode, int32_t scenarios)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->SetCallAudioMode(mode, scenarios);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::AnswerCall()
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->AnswerCall();
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::RejectCall()
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->RejectCall();
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::RejectCall(RejectType rejectType)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->RejectCall(rejectType);
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::HangUpCall()
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->HangUpCall();
    } else {
        TELEPHONY_LOGE("init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

int32_t CallManagerClient::GetCallTransferInfo(const std::string number, CallTransferType type)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->GetCallTransferInfo(number, type);
    } else {
        TELEPHONY_LOGE("GetCallTransferInfo by number, init first please!");
        return TELEPHONY_ERR_UNINIT;
    }
}

#ifdef CALL_MANAGER_CALL_TRANSFER
int32_t CallManagerClient::RegisterTransferController(std::unique_ptr<TransferControl> transferControl)
{
    TELEPHONY_LOGI("CallManagerClient::RegisterTransferController");
    if (transferControl == nullptr) {
        TELEPHONY_LOGE("CallManagerClient::RegisterTransferController transferControl is nullptr");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }

    if (g_callManagerProxy == nullptr) {
        TELEPHONY_LOGE("CallManagerClient::RegisterTransferController g_callManagerProxy is nullptr");
        return TELEPHONY_ERR_UNINIT;
    }

    return g_callManagerProxy->RegisterTransferController(transferControl);
}

int32_t CallManagerClient::UnRegisterTransferController()
{
    if (g_callManagerProxy == nullptr) {
        TELEPHONY_LOGE("CallManagerClient::UnRegisterTransferController g_callManagerProxy is nullptr");
        return TELEPHONY_ERR_UNINIT;
    }

    return g_callManagerProxy->UnRegisterTransferController();
}

int32_t CallManagerClient::NotifyTransferCallContact(const std::string contactName)
{
    if (g_callManagerProxy == nullptr) {
        TELEPHONY_LOGE("CallManagerClient::NotifyTransferCallContact g_callManagerProxy is nullptr");
        return TELEPHONY_ERR_UNINIT;
    }

    return g_callManagerProxy->NotifyTransferCallContact(contactName);
}
#else
int32_t CallManagerClient::RegisterTransferController(std::unique_ptr<TransferControl> transferControl)
{
    TELEPHONY_LOGE("CallManagerClient::RegisterTransferController CALL_MANAGER_CALL_TRANSFER is false");
    return TELEPHONY_ERR_UNINIT;
}

int32_t CallManagerClient::UnRegisterTransferController()
{
    TELEPHONY_LOGE("CallManagerClient::UnRegisterTransferController CALL_MANAGER_CALL_TRANSFER is false");
    return TELEPHONY_ERR_UNINIT;
}

int32_t CallManagerClient::NotifyTransferCallContact(const std::string contactName)
{
    TELEPHONY_LOGE("CallManagerClient::NotifyTransferCallContact CALL_MANAGER_CALL_TRANSFER is false");
    return TELEPHONY_ERR_UNINIT;
}
#endif

bool CallManagerClient::CheckCallRecordingPermission(const std::string& cellularRecordPhoneNum,
    const std::string& cellularRecordToken)
{
    if (g_callManagerProxy != nullptr) {
        return g_callManagerProxy->CheckCallRecordingPermission(cellularRecordPhoneNum, cellularRecordToken);
    } else {
        TELEPHONY_LOGE("CheckCallRecordingPermission, init first please!");
        return false;
    }
}
} // namespace Telephony
} // namespace OHOS
