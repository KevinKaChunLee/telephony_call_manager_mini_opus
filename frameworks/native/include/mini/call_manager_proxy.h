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

#ifndef CALL_MANAGER_PROXY_H
#define CALL_MANAGER_PROXY_H

// Mini variant of frameworks/native/include/call_manager_proxy.h. It keeps every public method the
// unchanged call_manager_client.cpp calls, with the standard signatures, and reaches the call manager
// in-process instead of over IPC; the IPC-only members and the RTT/transfer methods, whose feature
// macros the Mini build never defines, are left out.
// In-scope calls copy their arguments into a synchronous main-loop task (design D7): they return
// TELEPHONY_ERR_UNINIT before Init or while the service is not ready, and
// TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL when the loop does not take the call within
// CALL_MANAGER_MINI_SYNC_WAIT_MS. The callback registration state lives on the main loop.

#include <atomic>
#include <functional>

#include "singleton.h"
#include "pac_map.h"
#include "telephony_log_wrapper.h"

#include "call_ability_callback.h"
#include "call_manager_callback.h"
#include "call_manager_inner_type.h"
#include "i_call_status_callback.h"

namespace OHOS {
namespace Telephony {
class CallManagerProxy : public std::enable_shared_from_this<CallManagerProxy> {
    DECLARE_DELAYED_SINGLETON(CallManagerProxy)

public:
    void Init(int32_t systemAbilityId);
    void UnInit();
    int32_t RegisterCallBack(std::unique_ptr<CallManagerCallback> callback);
    int32_t UnRegisterCallBack();
    int32_t ObserverOnCallDetailsChange();
    int32_t DialCall(std::u16string number, AppExecFwk::PacMap &extras);
    int32_t MakeCall(std::string number);
    int32_t MakeCallWithToken(std::string number, AppExecFwk::PacMap &options, std::string &token);
    int32_t AnswerCall(int32_t callId, int32_t videoState, bool isRTT = false);
    int32_t RejectCall(int32_t callId, bool isSendSms, std::u16string content);
    int32_t HangUpCall(int32_t callId);
    int32_t GetCallState();
    int32_t HoldCall(int32_t callId);
    int32_t UnHoldCall(int32_t callId);
    int32_t SwitchCall(int32_t callId);
    int32_t CombineConference(int32_t callId);
    int32_t SeparateConference(int32_t callId);
    int32_t KickOutFromConference(int32_t callId);
    int32_t GetMainCallId(int32_t &callId, int32_t &mainCallId);
    int32_t GetSubCallIdList(int32_t callId, std::vector<std::u16string> &callIdList);
    int32_t GetCallIdListForConference(int32_t callId, std::vector<std::u16string> &callIdList);
    int32_t GetCallWaiting(int32_t slotId);
    int32_t SetCallWaiting(int32_t slotId, bool activate);
    int32_t GetCallRestriction(int32_t slotId, CallRestrictionType type);
    int32_t SetCallRestriction(int32_t slotId, CallRestrictionInfo &info);
    int32_t SetCallRestrictionPassword(
        int32_t slotId, CallRestrictionType fac, const char *oldPassword, const char *newPassword);
    int32_t GetCallTransferInfo(int32_t slotId, CallTransferType type);
    int32_t SetCallTransferInfo(int32_t slotId, CallTransferInfo &info);
    int32_t CanSetCallTransferTime(int32_t slotId, bool &result);
    int32_t SetCallPreferenceMode(int32_t slotId, int32_t mode);
    int32_t StartDtmf(int32_t callId, char str);
    int32_t StopDtmf(int32_t callId);
    int32_t PostDialProceed(int32_t callId, bool proceed);
    int32_t IsRinging(bool &enabled);
    bool HasCall(const bool isInCludeVoipCall = true);
    int32_t IsNewCallAllowed(bool &enabled);
    int32_t RegisterVoipCallManagerCallback();
    int32_t UnRegisterVoipCallManagerCallback();
    int32_t IsInEmergencyCall(bool &enabled);
    int32_t IsEmergencyPhoneNumber(std::u16string &number, int32_t slotId, bool &enabled);
    int32_t FormatPhoneNumber(std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber);
    int32_t FormatPhoneNumberToE164(
        std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber);
    int32_t SetMuted(bool isMute);
    int32_t MuteRinger();
    int32_t SetAudioDevice(const AudioDevice &audioDevice);
    int32_t ControlCamera(int32_t callId, std::u16string &cameraId);
    int32_t SetPreviewWindow(int32_t callId, std::string &surfaceId);
    int32_t SetDisplayWindow(int32_t callId, std::string &surfaceId);
    int32_t SetCameraZoom(float zoomRatio);
    int32_t SetPausePicture(int32_t callId, std::u16string &path);
    int32_t SetDeviceDirection(int32_t callId, int32_t rotation);
    int32_t GetImsConfig(int32_t slotId, ImsConfigItem item);
    int32_t SetImsConfig(int32_t slotId, ImsConfigItem item, std::u16string &value);
    int32_t GetImsFeatureValue(int32_t slotId, FeatureType type);
    int32_t SetImsFeatureValue(int32_t slotId, FeatureType type, int32_t value);
    int32_t UpdateImsCallMode(int32_t callId, ImsCallMode mode);
    int32_t EnableImsSwitch(int32_t slotId);
    int32_t DisableImsSwitch(int32_t slotId);
    int32_t IsImsSwitchEnabled(int32_t slotId, bool &enabled);
    int32_t SetVoNRState(int32_t slotId, int32_t state);
    int32_t GetVoNRState(int32_t slotId, int32_t &state);
    int32_t JoinConference(int32_t callId, std::vector<std::u16string> &numberList);
    int32_t ReportOttCallDetailsInfo(std::vector<OttCallDetailsInfo> &ottVec);
    int32_t ReportOttCallEventInfo(OttCallEventInfo &eventInfo);
    int32_t CloseUnFinishedUssd(int32_t slotId);
    int32_t InputDialerSpecialCode(const std::string &specialCode);
    int32_t RemoveMissedIncomingCallNotification();
    int32_t SetVoIPCallState(int32_t state);
    int32_t GetVoIPCallState(int32_t &state);
    int32_t SetVoIPCallInfo(int32_t callId, int32_t state, std::string phoneNumber);
    int32_t GetVoIPCallInfo(int32_t &callId, int32_t &state, std::string &phoneNumber);
    int32_t SetRegMmiCodeCallbackState(bool isReg);
    int32_t ReportAudioDeviceInfo();
    int32_t CancelCallUpgrade(int32_t callId);
    int32_t RequestCameraCapabilities(int32_t callId);
    int32_t SendCallUiEvent(int32_t callId, std::string &eventName);
    int32_t PreloadCallUi(bool enable);
    sptr<ICallStatusCallback> RegisterBluetoothCallManagerCallbackPtr(int32_t phoneIndex, std::string &macAddress);
    int32_t SendUssdResponse(int32_t slotId, std::string &content);
    int32_t SetCallPolicyInfo(bool isDialingTrustlist, const std::vector<std::string> &dialingList,
        bool isIncomingTrustlist, const std::vector<std::string> &incomingList);
    bool EndCall();
    bool HasDistributedCommunicationCapability();
    int32_t NotifyVoIPAudioStreamStart(int32_t uid);
    bool CheckCallRecordingPermission(const std::string& cellularRecordPhoneNum,
        const std::string& cellularRecordToken);
    int32_t SetCallAudioMode(int32_t mode, int32_t scenarios);
    int32_t AnswerCall();
    int32_t RejectCall();
    int32_t RejectCall(RejectType rejectType);
    int32_t HangUpCall();
    int32_t GetCallTransferInfo(const std::string number, CallTransferType type);

private:
    template <typename R>
    int32_t RunOnService(const char *method, std::function<R()> call, R &result, R notReady);

    std::atomic<bool> initStatus_ { false };
    bool registerStatus_ = false;
    sptr<CallAbilityCallback> callAbilityCallbackPtr_;
};
} // namespace Telephony
} // namespace OHOS

#endif
