/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef CALL_BASE_H
#define CALL_BASE_H

#include <unistd.h>
#include <cstring>
#include <memory>
#include "ffrt.h"

#include "refbase.h"
#include "pac_map.h"

#include "common_type.h"
#include "conference_base.h"
#include "call_manager_info.h"

/**
 * @ClassName: CallBase
 * @Description: an abstraction  of telephone calls, provide basic call ops interfaces
 */
namespace OHOS {
namespace Telephony {
class CallBase : public virtual RefBase {
public:
    explicit CallBase(DialParaInfo &info);
    CallBase(DialParaInfo &info, AppExecFwk::PacMap &extras);
    virtual ~CallBase();

    virtual int32_t DialingProcess() = 0;
    virtual int32_t AnswerCall(int32_t videoState, bool isRTT = false) = 0;
    virtual int32_t RejectCall() = 0;
    virtual int32_t HangUpCall() = 0;
    virtual int32_t HoldCall() = 0;
    virtual int32_t UnHoldCall() = 0;
    virtual int32_t SwitchCall() = 0;
    virtual void GetCallAttributeInfo(CallAttributeInfo &info) = 0;
    virtual bool GetEmergencyState() = 0;
    virtual int32_t StartDtmf(char str) = 0;
    virtual int32_t StopDtmf() = 0;
    virtual int32_t PostDialProceed(bool proceed) = 0;
    virtual int32_t GetSlotId() = 0;
    virtual int32_t CombineConference() = 0;
    virtual void HandleCombineConferenceFailEvent() = 0;
    virtual int32_t SeparateConference() = 0;
    virtual int32_t KickOutFromConference() = 0;
    virtual int32_t CanCombineConference() = 0;
    virtual int32_t CanSeparateConference() = 0;
    virtual int32_t CanKickOutFromConference() = 0;
    virtual int32_t LaunchConference() = 0;
    virtual int32_t ExitConference() = 0;
    virtual int32_t HoldConference() = 0;
    virtual int32_t GetMainCallId(int32_t &mainCallId) = 0;
    virtual int32_t GetSubCallIdList(std::vector<std::u16string> &callIdList) = 0;
    virtual int32_t GetCallIdListForConference(std::vector<std::u16string> &callIdList) = 0;
    virtual int32_t IsSupportConferenceable() = 0;
    virtual int32_t SetMute(int32_t mute, int32_t slotId) = 0;
    int32_t DialCallBase();
    int32_t IncomingCallBase();
    int32_t AnswerCallBase();
    int32_t RejectCallBase();
    void GetCallAttributeBaseInfo(CallAttributeInfo &info);
    int32_t GetCallID();
    CallType GetCallType();
    int32_t GetImsDomain();
    CallRunningState GetCallRunningState();
    int32_t SetTelCallState(TelCallState nextState);
    TelCallState GetTelCallState();
    void SetTelConferenceState(TelConferenceState state);
    TelConferenceState GetTelConferenceState();
    VideoStateType GetVideoStateType();
    void SetVideoStateType(VideoStateType mediaType);
    void SetPolicyFlag(PolicyFlag flag);
    uint64_t GetPolicyFlag();
    ContactInfo GetCallerInfo();
    void SetCallerInfo(const ContactInfo &contactInfo);
    NumberMarkInfo GetNumberMarkInfo();
    void SetNumberMarkInfo(const NumberMarkInfo &numberMarkInfo);
    void SetBlockReason(const int32_t &blockReason);
    void SetDetectDetails(std::string detectDetails);
    std::string GetDetectDetails();
    void SetCallRunningState(CallRunningState callRunningState);
    void SetStartTime(int64_t startTime);
    void SetCallBeginTime(time_t callBeginTime);
    void SetCallCreateTime(time_t callCreateTime);
    void SetCallEndTime(time_t callEndTime);
    void SetRingBeginTime(time_t ringBeginTime);
    void SetRingEndTime(time_t ringEndTime);
    void SetAnswerType(CallAnswerType answerType);
    void SetRejectType(RejectType rejectType);
    time_t GetCallCreateTime();
    CallAnswerType GetAnswerType();
    void SetCallId(int32_t callId);
    bool IsSpeakerphoneEnabled();
    bool IsCurrentRinging();
    std::string GetAccountNumber();
    void SetAccountNumber(const std::string accountNumber);
    int32_t SetSpeakerphoneOn(bool speakerphoneOn);
    bool IsSpeakerphoneOn();
    void SetAutoAnswerState(bool flag);
    bool GetAutoAnswerState();
    void SetAnswerVideoState(int32_t videoState);
    int32_t GetAnswerVideoState();
    void SetCanUnHoldState(bool flag);
    bool GetCanUnHoldState();
    void SetCanSwitchCallState(bool flag);
    bool GetCanSwitchCallState();
    bool CheckVoicemailNumber(std::string phoneNumber);
    bool IsAliveState();
    void SetBundleName(const char *bundleName);
    void SetCallType(CallType callType);
    void SetImsDomain(int32_t imsDomain);
    void SetCrsType(int32_t crsType);
    int32_t GetCrsType();
    void SetOriginalCallType(int32_t originalCallType);
    int32_t GetOriginalCallType();
    void SetNumberLocation(std::string numberLocation);
    std::string GetNumberLocation();
    virtual void SetSlotId(int32_t slotId) {}
    virtual void SetCallIndex(int32_t index) {}
    virtual int32_t GetCallIndex() {return 0;}
    int32_t SetMicPhoneState(bool isMuted);
    bool IsMuted();
    void SetMicDisabled(bool isMicDisabled);
    bool IsMicDisabled();
    void SetIsEccContact(bool isEccContact);
    void SetCeliaCallType(int32_t celiaCallType);
    int32_t GetCeliaCallType();
    void SetExtraParams(AAFwk::WantParams extraParams);
    int GetParamsByKey(const std::string &key, int defaultValue);
    void SetCallDirection(CallDirection direction);
    CallDirection GetCallDirection();
    int32_t GetAccountId();
    void SetAccountId(int32_t accountId);
    bool IsAnsweredCall();
    void SetAnsweredCall(bool isAnswered);
    AAFwk::WantParams GetExtraParams();
    void SetPhoneOrWatchDial(int32_t phoneOrWatch);
    void SetIsCustomAccessibility(bool isCustomAccessibility);
    bool GetIsCustomAccessibility();
    std::string GetToken();
    bool IsAiAutoAnswer();
    void SetAiAutoAnswer(bool isAiAutoAnswer);
    bool IsForcedReportVoiceCall();
    void SetForcedReportVoiceCall(bool isForcedReportVoiceCall);
    int32_t GetPhoneOrWatchDial();
    bool GetAnsweredByPhone();
    void SetIsAnsweredByPhone(bool isAnsweredByPhone);
    void SetNewCallUseBox(int32_t newCallUseBox);
    int32_t GetNewCallUseBox();
    int32_t GetBtCallSlotId();
    void SetBtCallSlotId(int32_t slotId);
    int32_t GetAntiFraudState();
    void SetAntiFraudState(int32_t antifraudState);
    void SetApCauseReported(bool isReported);
    bool IsApCauseReported();
    bool isNonVirtualCall();
    void SetNonVirtualCall(bool isNonVirtualCall);

#ifdef CALL_MANAGER_CALL_TRANSFER
    virtual bool IsTransferCall() {return false;}
    virtual bool IsTransferCallAndForbidden() {return false;}
#endif

protected:
    int32_t callId_;
    CallType callType_;
    VideoStateType videoState_;
    std::string accountNumber_;
    std::string bundleName_;

private:
    void StateChangesToDialing();
    void StateChangesToIncoming();
    void StateChangesToWaiting();
    void StateChangesToActive();
    void StateChangesToHolding();
    void StateChangesToDisconnected();
    void StateChangesToDisconnecting();
    void StateChangesToAlerting();
    void HangUpVoipCall();

    CallRunningState callRunningState_;
    TelConferenceState conferenceState_;
    int64_t startTime_; // Call start time
    CallDirection direction_;
    uint64_t policyFlag_;
    TelCallState callState_;
    bool autoAnswerState_;
    bool canUnHoldState_;
    bool canSwitchCallState_;
    int32_t answerVideoState_;
    bool isSpeakerphoneOn_;
    ContactInfo contactInfo_;
    time_t callBeginTime_;
    time_t callCreateTime_;
    time_t callEndTime_;
    time_t ringBeginTime_;
    time_t ringEndTime_;
    CallAnswerType answerType_;
    int32_t accountId_;
    int32_t crsType_;
    int32_t originalCallType_;
    bool isMuted_;
    ffrt::mutex mutex_;
    std::string numberLocation_;
    NumberMarkInfo numberMarkInfo_;
    int32_t blockReason_;
    bool isEccContact_;
    int32_t celiaCallType_;
    AAFwk::WantParams extraParams_;
    bool isAnswered_;
    std::string detectDetails_;
    int32_t phoneOrWatch_ = 0;
    bool isCustomAccessibility_ = false;
    std::string token_;
    bool isAiAutoAnswer_;
    bool isForcedReportVoiceCall_{false};
    bool isAnsweredByPhone_{false};
    int32_t newCallUseBox_ = 0;
    int32_t btCallSlotId_{BT_CALL_INVALID_SLOT};
    int32_t antiFraudState_ = 0;
    bool isNonVirtualCall_{false};
    int32_t imsDomain_ = 0;
    bool isMicDisabled_;
    bool isApCauseReported_{false};
    RejectType rejectType_;
};
} // namespace Telephony
} // namespace OHOS

#endif // CALL_BASE_H
