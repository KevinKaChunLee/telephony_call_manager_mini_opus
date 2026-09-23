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

#ifndef CALL_STATUS_MANAGER_H
#define CALL_STATUS_MANAGER_H

// Mini variant of services/call/include/call_status_manager.h: the same class, base and in-scope
// methods, minus anti-fraud, VoIP, Bluetooth, distributed, watch, motion recognition, incoming-call
// filtering and DSDA. Everything runs on the main loop.

#include <map>

#include "refbase.h"
#include "ffrt.h"
#include "common_type.h"
#include "call_status_policy.h"

namespace OHOS {
namespace Telephony {
const int32_t SLOT_NUM = 2;

class CallStatusManager : public CallStatusPolicy {
public:
    CallStatusManager();
    ~CallStatusManager();
    int32_t Init();
    int32_t HandleCallReportInfo(const CallDetailInfo &info);
    int32_t HandleCallsReportInfo(const CallDetailsInfo &info);
    int32_t HandleDisconnectedCause(const DisconnectedDetails &details);
    int32_t HandleEventResultReportInfo(const CellularCallEventInfo &info);

private:
    void InitCallBaseEvent();
    int32_t IncomingHandle(const CallDetailInfo &info);
    int32_t FinalizeIncomingState(sptr<CallBase> &call, const TelCallState nextState);
    int32_t PrepareIncomingCall(const CallDetailInfo &info, bool &isExisted);
    int32_t HandleRejectCall(sptr<CallBase> &call, bool isBlock);
    int32_t DialingHandle(const CallDetailInfo &info);
    bool UpdateDialingHandle(const CallDetailInfo &info);
    int32_t UpdateDialingCallInfo(const CallDetailInfo &info);
    int32_t ActiveHandle(const CallDetailInfo &info);
    int32_t HoldingHandle(const CallDetailInfo &info);
    int32_t WaitingHandle(const CallDetailInfo &info);
    int32_t AlertHandle(const CallDetailInfo &info);
    int32_t DisconnectingHandle(const CallDetailInfo &info);
    int32_t DisconnectedHandle(const CallDetailInfo &info);
    int32_t HandleCallReportInfoEx(const CallDetailInfo &info);
    void HandleConnectingCallReportInfo(const CallDetailInfo &info);
    void UpdateCallDetailsInfo(const CallDetailsInfo &info);
    void HandleHoldCallOrAutoAnswerCall(const sptr<CallBase> call,
        std::vector<std::u16string> callIdList, CallRunningState previousState, TelCallState priorState);
    void IsCanUnHold(int32_t activeCallNum, int32_t waitingCallNum, int32_t size, bool &canUnHold);
    sptr<CallBase> CreateNewCall(const CallDetailInfo &info, CallDirection dir);
    void SetCallParams(const sptr<CallBase> &callPtr, const CallDetailInfo &info);
    sptr<CallBase> CreateNewCallByCallType(
        DialParaInfo &paraInfo, const CallDetailInfo &info, CallDirection dir, AppExecFwk::PacMap &extras);
    sptr<CallBase> RefreshCallIfNecessary(const sptr<CallBase> &call, const CallDetailInfo &info);
    sptr<CallBase> RefreshCall(const sptr<CallBase> &call, const CallDetailInfo &info);
    void SetOriginalCallTypeForActiveState(sptr<CallBase> &call);
    void SetOriginalCallTypeForDisconnectState(sptr<CallBase> &call);
    void PackParaInfo(
        DialParaInfo &paraInfo, const CallDetailInfo &info, CallDirection dir, AppExecFwk::PacMap &extras);
    int32_t UpdateCallState(sptr<CallBase> &call, TelCallState nextState);
    void SetConferenceCall(std::vector<sptr<CallBase>> conferenceCallList);
    std::vector<sptr<CallBase>> GetConferenceCallList(int32_t slotId);
    int32_t UpdateCallStateAndHandleDsdsMode(const CallDetailInfo &info, sptr<CallBase> &call);
    void HandleDialWhenHolding(int32_t callId, sptr<CallBase> &call);
    void ClearPendingState(sptr<CallBase> &call);
    void RefreshCallDisconnectReason(const sptr<CallBase> &call, int32_t reason, const std::string &message);
    int32_t RefreshOldCall(const CallDetailInfo &info, bool &isExistedOldCall);
    bool RefreshDialingStateByOtherState(sptr<CallBase> &call, const CallDetailInfo &info);

private:
    CallDetailInfo callReportInfo_;
    CallDetailsInfo callDetailsInfo_[SLOT_NUM];
    CallDetailsInfo tmpCallDetailsInfo_[SLOT_NUM];
    std::map<RequestResultEventId, CallAbilityEventId> mEventIdTransferMap_;
    VideoStateType priorVideoState_[SLOT_NUM] = {VideoStateType::TYPE_VOICE};
};
} // namespace Telephony
} // namespace OHOS

#endif // CALL_STATUS_MANAGER_H
