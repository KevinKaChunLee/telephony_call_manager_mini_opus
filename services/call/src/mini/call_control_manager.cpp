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

// A second non-emergency dial while one is being set up fails before the pending dial parameters are
// touched. The pending-hangup protect task is a delayed task on the main loop. Satellite, super-privacy,
// VoIP conflicts, distributed audio, missed-call notifications, call records and running locks are cut.

#include "call_control_manager.h"

#include <string_ex.h>

#include "audio_control_manager.h"
#include "call_ability_report_proxy.h"
#include "call_manager_errors.h"
#include "call_number_utils.h"
#include "call_request_event_handler_helper.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
namespace {
constexpr uint32_t PENDINGHANGUP_DELAY_TIME_MS = 30000;
uint32_t g_pendingHangupDelayMs = PENDINGHANGUP_DELAY_TIME_MS;
} // namespace

CallControlManager::CallControlManager() {}

#ifdef TELEPHONY_MINI_HOST_TEST
void CallControlManager::SetPendingHangupDelayForTest(uint32_t delayMs)
{
    g_pendingHangupDelayMs = (delayMs == 0) ? PENDINGHANGUP_DELAY_TIME_MS : delayMs;
}
#endif

CallControlManager::~CallControlManager() {}

bool CallControlManager::Init()
{
    if (callStateListenerPtr_ != nullptr) {
        return true;
    }
    callStateListenerPtr_ = std::make_unique<CallStateListener>();
    CallRequestHandlerPtr_ = std::make_unique<CallRequestHandler>();
    CallRequestHandlerPtr_->Init();
    DelayedSingleton<AudioControlManager>::GetInstance()->Init();
    CallStateObserve();
    return true;
}

void CallControlManager::UnInit()
{
    RemovePendingHangupProtectTask();
    if (callStateListenerPtr_ != nullptr) {
        callStateListenerPtr_->RemoveAllObserver();
    }
    DelayedSingleton<AudioControlManager>::GetInstance()->UnInit();
    callStateListenerPtr_ = nullptr;
    CallRequestHandlerPtr_ = nullptr;
}

int32_t CallControlManager::DialCall(std::u16string &number, AppExecFwk::PacMap &extras)
{
    std::string accountNumber(Str16ToStr8(number));
    int32_t ret = NumberLegalityCheck(accountNumber);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    auto accountId = extras.GetIntValue("accountId");
    bool isEcc = false;
    std::string newPhoneNum =
        DelayedSingleton<CallNumberUtils>::GetInstance()->RemoveSeparatorsPhoneNumber(accountNumber);
    DelayedSingleton<CallNumberUtils>::GetInstance()->CheckNumberIsEmergency(newPhoneNum, accountId, isEcc);
    if (isEcc) {
        extras.PutIntValue("dialScene", (int32_t)DialScene::CALL_EMERGENCY);
    }
    std::u16string newPhoneNumU16 = Str8ToStr16(newPhoneNum);
    ret = CanDial(newPhoneNumU16, extras, isEcc);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("dial policy result:%{public}d", ret);
        return ret;
    }
    if (!isEcc && HasNewCall() != TELEPHONY_SUCCESS) {
        return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
    }
    SetCallTypeExtras(extras);
    // temporarily save dial information
    PackageDialInformation(extras, newPhoneNum, isEcc);
    if (CallRequestHandlerPtr_ == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    ret = CallRequestHandlerPtr_->DialCall();
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("DialCall failed!");
        return ret;
    }
    return TELEPHONY_SUCCESS;
}

void CallControlManager::SetCallTypeExtras(AppExecFwk::PacMap &extras)
{
    int32_t dialType = extras.GetIntValue("dialType");
    if (dialType == (int32_t)DialType::DIAL_CARRIER_TYPE || dialType == (int32_t)DialType::DIAL_VOICE_MAIL_TYPE) {
        if (!IsSupportVideoCall(extras)) {
            extras.PutIntValue("videoState", (int32_t)VideoStateType::TYPE_VOICE);
        }
        VideoStateType videoState = (VideoStateType)extras.GetIntValue("videoState");
        if (videoState == VideoStateType::TYPE_VIDEO) {
            extras.PutIntValue("callType", (int32_t)CallType::TYPE_IMS);
        }
    }
}

int32_t CallControlManager::CanDial(std::u16string &number, AppExecFwk::PacMap &extras, bool isEcc)
{
    int32_t ret = DialPolicy(number, extras, isEcc);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    return CanDialMulityCall(extras, isEcc);
}

void CallControlManager::PackageDialInformation(AppExecFwk::PacMap &extras, std::string accountNumber, bool isEcc)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    dialSrcInfo_.callId = ERR_ID;
    dialSrcInfo_.number = accountNumber;
    dialSrcInfo_.isDialing = true;
    dialSrcInfo_.isEcc = isEcc;
    dialSrcInfo_.isRTT = extras.GetBooleanValue("isRTT");
    dialSrcInfo_.callType = (CallType)extras.GetIntValue("callType");
    dialSrcInfo_.accountId = extras.GetIntValue("accountId");
    dialSrcInfo_.dialType = (DialType)extras.GetIntValue("dialType");
    dialSrcInfo_.videoState = (VideoStateType)extras.GetIntValue("videoState");
    dialSrcInfo_.originalCallType = (int32_t)extras.GetIntValue("videoState");
    dialSrcInfo_.bundleName = extras.GetStringValue("bundleName");
    dialSrcInfo_.isCustomAccessibility = extras.GetBooleanValue("isCustomAccessibility", false);
    dialSrcInfo_.token = extras.GetStringValue("token");
    dialSrcInfo_.phoneIndex = extras.GetIntValue("phoneIndex");
    extras_.Clear();
    extras_ = extras;
}

sptr<CallBase> CallControlManager::GetRingCall(int32_t callId, int32_t videoState)
{
    sptr<CallBase> call = GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_RINGING);
    if (call == nullptr) {
        TELEPHONY_LOGE("call is nullptr");
        return nullptr;
    }
    int32_t ringCallId = callId;
    if (ringCallId == INVALID_CALLID) {
        ringCallId = call->GetCallID();
    }
    return GetOneCallObject(ringCallId);
}

int32_t CallControlManager::AnswerCall(int32_t callId, int32_t videoState, bool isRTT)
{
    sptr<CallBase> call = GetRingCall(callId, videoState);
    if (call == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    auto ringCallId = call->GetCallID();
    call->SetAnsweredCall(true);
    NotifyCallStateUpdated(call, TelCallState::CALL_STATUS_INCOMING, TelCallState::CALL_STATUS_ANSWERED);
    int32_t ret = AnswerCallPolicy(ringCallId, videoState);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("AnswerCallPolicy failed: %{public}d", ret);
        return ret;
    }
    return HandlerAnswerCall(ringCallId, videoState, isRTT);
}

int32_t CallControlManager::HandlerAnswerCall(int32_t callId, int32_t videoState, bool isRTT)
{
    if (CallRequestHandlerPtr_ == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    int ret = CallRequestHandlerPtr_->AnswerCall(callId, videoState, isRTT);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("AnswerCall failed!");
        return ret;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallControlManager::RejectCall(int32_t callId, bool rejectWithMessage, std::u16string textMessage)
{
    if (CallRequestHandlerPtr_ == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    sptr<CallBase> call = GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_RINGING);
    if (call == nullptr) {
        TELEPHONY_LOGE("call is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    if (callId == INVALID_CALLID) {
        callId = call->GetCallID();
    }
    int32_t ret = RejectCallPolicy(callId);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    std::string messageStr(Str16ToStr8(textMessage));
    ret = CallRequestHandlerPtr_->RejectCall(callId, rejectWithMessage, messageStr);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("RejectCall failed!");
        return ret;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallControlManager::HangUpCall(int32_t callId)
{
    if (callId == INVALID_CALLID) {
        std::vector<CallRunningState> callRunningStateVec;
        callRunningStateVec.push_back(CallRunningState::CALL_RUNNING_STATE_ACTIVE);
        callRunningStateVec.push_back(CallRunningState::CALL_RUNNING_STATE_DIALING);
        callRunningStateVec.push_back(CallRunningState::CALL_RUNNING_STATE_CONNECTING);
        callRunningStateVec.push_back(CallRunningState::CALL_RUNNING_STATE_HOLD);
        for (auto &state : callRunningStateVec) {
            sptr<CallBase> call = GetOneCallObject(state);
            if (call != nullptr) {
                callId = call->GetCallID();
                break;
            }
        }
        if (callId == INVALID_CALLID) {
            TELEPHONY_LOGE("callId is INVALID_CALLID!");
            return TELEPHONY_ERR_ARGUMENT_INVALID;
        }
    }
    if (CallRequestHandlerPtr_ == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    int32_t ret = HangUpPolicy(callId);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    auto callRequestEventHandler = DelayedSingleton<CallRequestEventHandlerHelper>::GetInstance();
    if (callRequestEventHandler->HasPendingMo(callId)) {
        callRequestEventHandler->SetPendingMo(false, -1);
        callRequestEventHandler->SetPendingHangup(true, callId);
        TELEPHONY_LOGI("HangUpCall before dialingHandle,hangup after CLCC");
        PostPendingHangupProtectTask(callId);
        return TELEPHONY_SUCCESS;
    }
    ret = CallRequestHandlerPtr_->HangUpCall(callId);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("HangUpCall failed!");
        return ret;
    }
    return TELEPHONY_SUCCESS;
}

void CallControlManager::PostPendingHangupProtectTask(int32_t callId)
{
    if (pendingHangupTaskId_ != TEL_MINI_INVALID_TASK_ID) {
        return;
    }
    int32_t ret = TelMiniMainLoop().PostAsyncTask([callId]() {
        auto strong = DelayedSingleton<CallControlManager>::GetInstance();
        strong->pendingHangupTaskId_ = TEL_MINI_INVALID_TASK_ID;
        auto callRequestEventHandler = DelayedSingleton<CallRequestEventHandlerHelper>::GetInstance();
        if (callRequestEventHandler->HasPendingHangup(callId)) {
            sptr<CallBase> call = GetOneCallObject(callId);
            if (call != nullptr) {
                strong->DealFailDial(call);
            }
            callRequestEventHandler->SetPendingHangup(false, -1);
            TELEPHONY_LOGI("PendingHangup timeout, clear pending state");
        }
    }, g_pendingHangupDelayMs, &pendingHangupTaskId_);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("pending hangup protect task not scheduled: %{public}d", ret);
    }
}

void CallControlManager::RemovePendingHangupProtectTask()
{
    if (pendingHangupTaskId_ == TEL_MINI_INVALID_TASK_ID) {
        return;
    }
    TelMiniMainLoop().RemoveAsyncTask(pendingHangupTaskId_);
    pendingHangupTaskId_ = TEL_MINI_INVALID_TASK_ID;
}

int32_t CallControlManager::GetCallState()
{
    CallStateToApp callState = CallStateToApp::CALL_STATE_UNKNOWN;
    if (!HasCellularCallExist()) {
        callState = CallStateToApp::CALL_STATE_IDLE;
    } else {
        callState = CallStateToApp::CALL_STATE_OFFHOOK;
        bool hasRingingCall = false;
        if (HasRingingCall(hasRingingCall) == TELEPHONY_SUCCESS && hasRingingCall) {
            callState = CallStateToApp::CALL_STATE_RINGING;
        }
    }
    return static_cast<int32_t>(callState);
}

int32_t CallControlManager::HoldCall(int32_t callId)
{
    int32_t ret = HoldCallPolicy(callId);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("HoldCall failed!");
        return ret;
    }
    if (CallRequestHandlerPtr_ == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return CallRequestHandlerPtr_->HoldCall(callId);
}

int32_t CallControlManager::UnHoldCall(const int32_t callId)
{
    int32_t ret = UnHoldCallPolicy(callId);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UnHoldCall failed!");
        return ret;
    }
    if (CallRequestHandlerPtr_ == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return CallRequestHandlerPtr_->UnHoldCall(callId);
}

// swap two calls state, turn active call into holding, and turn holding call into active
int32_t CallControlManager::SwitchCall(int32_t callId)
{
    int32_t ret = SwitchCallPolicy(callId);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    if (CallRequestHandlerPtr_ == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return CallRequestHandlerPtr_->SwitchCall(callId);
}

bool CallControlManager::HasCall()
{
    return HasCellularCallExist();
}

int32_t CallControlManager::IsNewCallAllowed(bool &enabled)
{
    return IsNewCallAllowedCreate(enabled);
}

int32_t CallControlManager::IsRinging(bool &enabled)
{
    return HasRingingCall(enabled);
}

int32_t CallControlManager::HasEmergency(bool &enabled)
{
    return HasEmergencyCall(enabled);
}

int32_t CallControlManager::IsEmergencyPhoneNumber(std::u16string &number, int32_t slotId, bool &enabled)
{
    if (IsValidSlotId(slotId)) {
        return CALL_ERR_INVALID_SLOT_ID;
    }
    std::string newPhoneNum =
        DelayedSingleton<CallNumberUtils>::GetInstance()->RemoveSeparatorsPhoneNumber(Str16ToStr8(number));
    return DelayedSingleton<CallNumberUtils>::GetInstance()->CheckNumberIsEmergency(newPhoneNum, slotId, enabled);
}

bool CallControlManager::EndCall()
{
    std::vector<CallRunningState> callRunningStateVec;
    callRunningStateVec.push_back(CallRunningState::CALL_RUNNING_STATE_ACTIVE);
    callRunningStateVec.push_back(CallRunningState::CALL_RUNNING_STATE_RINGING);
    callRunningStateVec.push_back(CallRunningState::CALL_RUNNING_STATE_DIALING);
    callRunningStateVec.push_back(CallRunningState::CALL_RUNNING_STATE_CONNECTING);
    callRunningStateVec.push_back(CallRunningState::CALL_RUNNING_STATE_HOLD);
    int32_t callId = INVALID_CALLID;
    int32_t ret = TELEPHONY_SUCCESS;
    for (auto &state : callRunningStateVec) {
        sptr<CallBase> call = GetOneCallObject(state);
        if (call != nullptr) {
            callId = call->GetCallID();
            if (callId == INVALID_CALLID) {
                continue;
            }
            if (state == CallRunningState::CALL_RUNNING_STATE_RINGING) {
                ret = RejectCall(callId, false, Str8ToStr16(""));
            } else {
                ret = HangUpCall(callId);
            }
            break;
        }
    }
    return ret == TELEPHONY_SUCCESS;
}

bool CallControlManager::NotifyNewCallCreated(sptr<CallBase> &callObjectPtr)
{
    if (callObjectPtr == nullptr) {
        TELEPHONY_LOGE("callObjectPtr is null!");
        return false;
    }
    if (callStateListenerPtr_ != nullptr) {
        callStateListenerPtr_->NewCallCreated(callObjectPtr);
    }
    return true;
}

bool CallControlManager::NotifyCallDestroyed(const DisconnectedDetails &details)
{
    if (callStateListenerPtr_ != nullptr) {
        callStateListenerPtr_->CallDestroyed(details);
        return true;
    }
    return false;
}

bool CallControlManager::NotifyCallStateUpdated(
    sptr<CallBase> &callObjectPtr, TelCallState priorState, TelCallState nextState)
{
    if (callObjectPtr == nullptr || callStateListenerPtr_ == nullptr) {
        TELEPHONY_LOGE("NotifyCallStateUpdated null ptr!");
        return false;
    }
    callStateListenerPtr_->CallStateUpdated(callObjectPtr, priorState, nextState);
    return true;
}

bool CallControlManager::NotifyIncomingCallAnswered(sptr<CallBase> &callObjectPtr)
{
    if (callObjectPtr == nullptr) {
        return false;
    }
    if (callStateListenerPtr_ != nullptr) {
        callStateListenerPtr_->IncomingCallActivated(callObjectPtr);
        return true;
    }
    return false;
}

bool CallControlManager::NotifyIncomingCallRejected(sptr<CallBase> &callObjectPtr, bool isSendSms, std::string content)
{
    if (callObjectPtr == nullptr) {
        return false;
    }
    if (callStateListenerPtr_ != nullptr) {
        callStateListenerPtr_->IncomingCallHungUp(callObjectPtr, isSendSms, content);
        return true;
    }
    return false;
}

bool CallControlManager::NotifyCallEventUpdated(CallEventInfo &info)
{
    if (callStateListenerPtr_ != nullptr) {
        callStateListenerPtr_->CallEventUpdated(info);
        return true;
    }
    return false;
}

void CallControlManager::GetDialParaInfo(DialParaInfo &info)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    info = dialSrcInfo_;
}

void CallControlManager::GetDialParaInfo(DialParaInfo &info, AppExecFwk::PacMap &extras)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    info = dialSrcInfo_;
    extras = extras_;
}

void CallControlManager::CallStateObserve()
{
    if (callStateListenerPtr_ == nullptr) {
        return;
    }
    callStateListenerPtr_->AddOneObserver(DelayedSingleton<CallAbilityReportProxy>::GetInstance());
    callStateListenerPtr_->AddOneObserver(DelayedSingleton<AudioControlManager>::GetInstance());
}

int32_t CallControlManager::NumberLegalityCheck(std::string &number)
{
    if (number.empty()) {
        TELEPHONY_LOGE("phone number is NULL!");
        return CALL_ERR_PHONE_NUMBER_EMPTY;
    }
    if (number.length() > kMaxNumberLen) {
        TELEPHONY_LOGE(
            "the number length exceeds limit,len:%{public}zu,maxLen:%{public}d", number.length(), kMaxNumberLen);
        return CALL_ERR_NUMBER_OUT_OF_RANGE;
    }
    size_t commaPos = number.find(',');
    if (commaPos != std::string::npos) {
        number = number.substr(0, commaPos);
    }
    return TELEPHONY_SUCCESS;
}
} // namespace Telephony
} // namespace OHOS
