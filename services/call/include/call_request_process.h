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

#ifndef CALL_REQUEST_PROCESS_H
#define CALL_REQUEST_PROCESS_H

#include "ffrt.h"

#include "call_object_manager.h"

namespace OHOS {
namespace Telephony {
class CallRequestProcess : public CallObjectManager {
public:
    CallRequestProcess();
    ~CallRequestProcess();

    int32_t DialRequest();
    int32_t HandleDialRequest(DialParaInfo &info);
    void AnswerRequest(int32_t callId, int32_t videoState, bool isRTT = false);
    void AnswerRequestForDsda(sptr<CallBase> call, int32_t callId, int32_t videoState, bool isRTT = false);
    void RejectRequest(int32_t callId, bool isSendSms, std::string &content);
    void HangUpRequest(int32_t callId);
    bool HangUpForDsdaRequest(sptr<CallBase> call);
    void HandleHoldAfterHangUp(TelCallState state, int32_t waitingCallNum);
    void HoldRequest(int32_t callId);
    void UnHoldRequest(int32_t callId);
    void SwitchRequest(int32_t callId);
    void CombineConferenceRequest(int32_t mainCallId);
    void SeparateConferenceRequest(int32_t callId);
    void KickOutFromConferenceRequest(int32_t callId);
    void JoinConference(int32_t callId, std::vector<std::string> &numberList);
    bool IsDsdsMode3();
    bool IsDsdsMode5();
    bool HasActivedCall(bool isIncludeVoipCall);
    bool HasConnectingCall(bool isIncludeVoipCall);
    bool NeedAnswerVTAndEndActiveVO(int32_t callId, int32_t videoState);
    bool NeedAnswerVOAndEndActiveVT(int32_t callId, int32_t videoState);
    void DisconnectOtherSubIdCall(int32_t callId, int32_t slotId, int32_t videoState);
    void DisconnectOtherCallForVideoCall(int32_t callId);
    void HandleCallWaitingNumTwo(sptr<CallBase> incomingCall, sptr<CallBase> call, int32_t slotId,
        int32_t activeCallNum, bool &flagForConference);
    void HandleCallWaitingNumOne(sptr<CallBase> incomingCall, sptr<CallBase> call, int32_t slotId,
        int32_t activeCallNum, bool &flagForConference);
    void HandleCallWaitingNumOneNext(sptr<CallBase> incomingCall, sptr<CallBase> call, sptr<CallBase> holdCall,
        int32_t slotId, bool &flagForConference);
    void HandleCallWaitingNumZero(sptr<CallBase> incomingCall, sptr<CallBase> call, int32_t slotId,
        int32_t activeCallNum, bool &flagForConference);
    void HoldOrDisconnectedCall(int32_t callId, int32_t slotId, int32_t videoState);
    bool HandleDsdaIncomingCall(
        sptr<CallBase> call, int32_t activeCallNum, int32_t slotId, int32_t videoState, sptr<CallBase> incomingCall);
    void IsExistCallOtherSlot(std::list<int32_t> &list, int32_t slotId, bool &noOtherCall);
    int32_t HandleDialingInfo(std::string newPhoneNum, DialParaInfo &info);
#ifdef SUPPORT_RTT_CALL
    void UpdateImsRttCallModeRequest(int32_t callId, ImsRTTCallMode mode);
#endif

private:
    int32_t CarrierDialProcess(DialParaInfo &info);
    int32_t IsDialCallForDsda(DialParaInfo &info);
    int32_t HandleEccCallForDsda(std::string newPhoneNum, DialParaInfo &info, bool &isEcc);
    int32_t VoiceMailDialProcess(DialParaInfo &info);
    int32_t OttDialProcess(DialParaInfo &info);
    int32_t PackCellularCallInfo(DialParaInfo &info, CellularCallInfo &callInfo);
    bool IsFdnNumber(std::vector<std::u16string> fdnNumberList, std::string phoneNumber);
    int32_t UpdateCallReportInfo(const DialParaInfo &info, TelCallState state);
    int32_t HandleStartDial(bool isMMiCode, DialParaInfo &info);
    int32_t HandleDialFail();
    int32_t GetOtherRingingCall(int32_t currentCallId);
    int32_t EccDialPolicy();
    int32_t BluetoothDialProcess(DialParaInfo &info);
    void GetDialingCall(sptr<CallBase> &call);
    bool IsCnSimCard(int32_t slotId);
    
private:
    ffrt::mutex mutex_;
};
} // namespace Telephony
} // namespace OHOS

#endif // CALL_REQUEST_PROCESS_H
