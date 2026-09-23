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

#ifndef CALL_CONTROL_MANAGER_H
#define CALL_CONTROL_MANAGER_H

// Mini variant of services/call/include/call_control_manager.h: the downlink for the in-scope actions
// and the observer fan-out. Only the retained observers are registered (the call ability report and
// audio); everything runs on the main loop.

#include <memory>

#include "singleton.h"
#include "call_policy.h"
#include "call_request_handler.h"
#include "call_state_listener.h"
#include "los_event_handler.h"

namespace OHOS {
namespace Telephony {
class CallControlManager : public CallPolicy, public std::enable_shared_from_this<CallControlManager> {
    DECLARE_DELAYED_SINGLETON(CallControlManager)

public:
    bool Init();
    void UnInit();
    int32_t DialCall(std::u16string &number, AppExecFwk::PacMap &extras);
    int32_t AnswerCall(int32_t callId, int32_t videoState, bool isRTT = false);
    int32_t RejectCall(int32_t callId, bool rejectWithMessage, std::u16string textMessage);
    int32_t HangUpCall(int32_t callId);
    int32_t GetCallState();
    int32_t HoldCall(int32_t callId);
    int32_t UnHoldCall(const int32_t callId);
    int32_t SwitchCall(int32_t callId);
    bool HasCall();
    int32_t IsNewCallAllowed(bool &enabled);
    int32_t IsRinging(bool &enabled);
    int32_t HasEmergency(bool &enabled);
    int32_t IsEmergencyPhoneNumber(std::u16string &number, int32_t slotId, bool &enabled);
    bool EndCall();
    bool NotifyNewCallCreated(sptr<CallBase> &callObjectPtr);
    bool NotifyCallDestroyed(const DisconnectedDetails &details);
    bool NotifyCallStateUpdated(sptr<CallBase> &callObjectPtr, TelCallState priorState, TelCallState nextState);
    bool NotifyIncomingCallAnswered(sptr<CallBase> &callObjectPtr);
    bool NotifyIncomingCallRejected(sptr<CallBase> &callObjectPtr, bool isSendSms, std::string content);
    bool NotifyCallEventUpdated(CallEventInfo &info);
    void GetDialParaInfo(DialParaInfo &info);
    void GetDialParaInfo(DialParaInfo &info, AppExecFwk::PacMap &extras);
    void RemovePendingHangupProtectTask();
#ifdef TELEPHONY_MINI_HOST_TEST
    // 0 restores the standard 30 s.
    static void SetPendingHangupDelayForTest(uint32_t delayMs);
#endif

private:
    void CallStateObserve();
    int32_t NumberLegalityCheck(std::string &number);
    void SetCallTypeExtras(AppExecFwk::PacMap &extras);
    int32_t CanDial(std::u16string &number, AppExecFwk::PacMap &extras, bool isEcc);
    void PackageDialInformation(AppExecFwk::PacMap &extras, std::string accountNumber, bool isEcc);
    sptr<CallBase> GetRingCall(int32_t callId, int32_t videoState);
    int32_t HandlerAnswerCall(int32_t callId, int32_t videoState, bool isRTT);
    void PostPendingHangupProtectTask(int32_t callId);

private:
    std::unique_ptr<CallStateListener> callStateListenerPtr_;
    std::unique_ptr<CallRequestHandler> CallRequestHandlerPtr_;
    DialParaInfo dialSrcInfo_;
    AppExecFwk::PacMap extras_;
    TelMiniTaskId pendingHangupTaskId_ = TEL_MINI_INVALID_TASK_ID;
    ffrt::mutex mutex_;
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_CONTROL_MANAGER_H
