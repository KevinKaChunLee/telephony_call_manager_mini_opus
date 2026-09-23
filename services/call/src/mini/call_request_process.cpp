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

// HandleDialFail does not wait: the local DIALING report is handled on the main loop before the dial
// command is sent, so the dialing call is already in the table (design D4). A failed local DIALING report
// also clears the dialing flag, so that a call the table could not hold does not block the next dial.
// FDN, MMI codes, OTT/Bluetooth dialing, video-call answer rules and DSDA are cut; the Mini build runs
// DSDS_MODE_V2.

#include "call_request_process.h"

#include <securec.h>
#include <string_ex.h>

#include "call_control_manager.h"
#include "call_manager_errors.h"
#include "call_number_utils.h"
#include "call_request_event_handler_helper.h"
#include "cellular_call_connection.h"
#include "report_call_info_handler.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
CallRequestProcess::CallRequestProcess() {}

CallRequestProcess::~CallRequestProcess() {}

int32_t CallRequestProcess::DialRequest()
{
    DialParaInfo info;
    DelayedSingleton<CallControlManager>::GetInstance()->GetDialParaInfo(info);
    if (!info.isDialing) {
        TELEPHONY_LOGE("the device is not dialing!");
        return CALL_ERR_ILLEGAL_CALL_OPERATION;
    }
    if (info.number.length() > static_cast<size_t>(kMaxNumberLen)) {
        TELEPHONY_LOGE("Number out of limit!");
        return CALL_ERR_NUMBER_OUT_OF_RANGE;
    }
    return HandleDialRequest(info);
}

bool CallRequestProcess::IsCnSimCard(int32_t slotId)
{
    return false;
}

int32_t CallRequestProcess::HandleDialRequest(DialParaInfo &info)
{
    int32_t ret = CALL_ERR_UNKNOW_DIAL_TYPE;
    switch (info.dialType) {
        case DialType::DIAL_CARRIER_TYPE:
        case DialType::DIAL_VOICE_MAIL_TYPE:
            ret = CarrierDialProcess(info);
            break;
        default:
            TELEPHONY_LOGE("dial type %{public}d is not supported", static_cast<int32_t>(info.dialType));
            ret = CALL_ERR_FUNCTION_NOT_SUPPORTED;
            break;
    }
    return ret;
}

void CallRequestProcess::AnswerRequest(int32_t callId, int32_t videoState, bool isRTT)
{
    sptr<CallBase> call = GetOneCallObject(callId);
    if (call == nullptr) {
        TELEPHONY_LOGE("the call object is nullptr, callId:%{public}d", callId);
        return;
    }
    call->SetAnswerVideoState(videoState);
    AnswerRequestForDsda(call, callId, videoState, isRTT);
}

void CallRequestProcess::AnswerRequestForDsda(sptr<CallBase> call, int32_t callId, int32_t videoState, bool isRTT)
{
    int32_t ret = call->AnswerCall(videoState, isRTT);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("AnswerCall failed!");
        return;
    }
    DelayedSingleton<CallControlManager>::GetInstance()->NotifyIncomingCallAnswered(call);
}

bool CallRequestProcess::IsDsdsMode3()
{
    return false;
}

bool CallRequestProcess::IsDsdsMode5()
{
    return false;
}

void CallRequestProcess::IsExistCallOtherSlot(std::list<int32_t> &list, int32_t slotId, bool &noOtherCall)
{
    if (list.size() > 1) {
        for (int32_t otherCallId : list) {
            sptr<CallBase> call = GetOneCallObject(otherCallId);
            if (call != nullptr && call->GetSlotId() != slotId) {
                noOtherCall = false;
                break;
            }
        }
    }
}

void CallRequestProcess::RejectRequest(int32_t callId, bool isSendSms, std::string &content)
{
    sptr<CallBase> call = GetOneCallObject(callId);
    if (call == nullptr) {
        TELEPHONY_LOGE("the call object is nullptr, callId:%{public}d", callId);
        return;
    }
    int32_t ret = call->RejectCall();
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("RejectCall failed!");
        return;
    }
    sptr<CallBase> holdCall = CallObjectManager::GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_HOLD);
    if (holdCall) {
        holdCall->SetCanUnHoldState(false);
    }
    DelayedSingleton<CallControlManager>::GetInstance()->NotifyIncomingCallRejected(call, isSendSms, content);
}

void CallRequestProcess::HangUpRequest(int32_t callId)
{
    sptr<CallBase> call = GetOneCallObject(callId);
    if (call == nullptr) {
        TELEPHONY_LOGE("the call object is nullptr, callId:%{public}d", callId);
        return;
    }
    int32_t waitingCallNum = GetCallNum(TelCallState::CALL_STATUS_WAITING);
    TelCallState state = call->GetTelCallState();
    TelConferenceState confState = call->GetTelConferenceState();
    if ((((state == TelCallState::CALL_STATUS_ACTIVE) &&
        (CallObjectManager::IsCallExist(call->GetCallType(), TelCallState::CALL_STATUS_HOLDING))) ||
        (confState == TelConferenceState::TEL_CONFERENCE_ACTIVE)) && waitingCallNum == 0) {
        if (!HangUpForDsdaRequest(call)) {
            TELEPHONY_LOGI("release the active call and recover the held call");
            call->SetPolicyFlag(PolicyFlag::POLICY_FLAG_HANG_UP_ACTIVE);
        }
    } else if (confState == TelConferenceState::TEL_CONFERENCE_HOLDING && waitingCallNum == 0) {
        TELEPHONY_LOGI("release the held call and the wait call");
        call->SetPolicyFlag(PolicyFlag::POLICY_FLAG_HANG_UP_HOLD_WAIT);
    }
    call->HangUpCall();
}

bool CallRequestProcess::HangUpForDsdaRequest(sptr<CallBase> call)
{
    return false;
}

void CallRequestProcess::HoldRequest(int32_t callId)
{
    sptr<CallBase> call = GetOneCallObject(callId);
    if (call == nullptr) {
        TELEPHONY_LOGE("the call object is nullptr, callId:%{public}d", callId);
        return;
    }
    call->HoldCall();
}

void CallRequestProcess::UnHoldRequest(int32_t callId)
{
    sptr<CallBase> call = GetOneCallObject(callId);
    if (call == nullptr) {
        TELEPHONY_LOGE("the call object is nullptr, callId:%{public}d", callId);
        return;
    }
    call->SetCanUnHoldState(true);
    bool noOtherCall = true;
    std::list<int32_t> callIdList;
    GetCarrierCallList(callIdList);
    IsExistCallOtherSlot(callIdList, call->GetSlotId(), noOtherCall);
    if (noOtherCall) {
        call->UnHoldCall();
    }
}

void CallRequestProcess::SwitchRequest(int32_t callId)
{
    sptr<CallBase> call = GetOneCallObject(callId);
    if (call == nullptr) {
        TELEPHONY_LOGE("the call object is nullptr, callId:%{public}d", callId);
        return;
    }
    call->SwitchCall();
}

int32_t CallRequestProcess::UpdateCallReportInfo(const DialParaInfo &info, TelCallState state)
{
    CallDetailInfo callDetatilInfo;
    callDetatilInfo.callType = info.callType;
    callDetatilInfo.accountId = info.accountId;
    callDetatilInfo.index = info.index;
    callDetatilInfo.state = state;
    callDetatilInfo.callMode = info.videoState;
    callDetatilInfo.originalCallType = info.originalCallType;
    callDetatilInfo.voiceDomain = static_cast<int32_t>(info.callType);
    callDetatilInfo.phoneOrWatch = info.phoneOrWatch;
    callDetatilInfo.phoneIndex = info.phoneIndex;
    if (info.number.length() > kMaxNumberLen) {
        TELEPHONY_LOGE("numbser length out of range");
        return CALL_ERR_NUMBER_OUT_OF_RANGE;
    }
    if (memcpy_s(&callDetatilInfo.phoneNum, kMaxNumberLen, info.number.c_str(), info.number.length()) != EOK) {
        TELEPHONY_LOGE("memcpy_s number failed!");
        return TELEPHONY_ERR_MEMCPY_FAIL;
    }
    return DelayedSingleton<ReportCallInfoHandler>::GetInstance()->UpdateCallReportInfo(callDetatilInfo);
}

int32_t CallRequestProcess::HandleDialFail()
{
    sptr<CallBase> call = nullptr;
    GetDialingCall(call);
    if (call != nullptr) {
        return DealFailDial(call);
    }
    TELEPHONY_LOGE("can not find connect call or dialing call");
    return CALL_ERR_CALL_STATE;
}

void CallRequestProcess::GetDialingCall(sptr<CallBase> &call)
{
    call = GetOneCarrierCallObject(CallRunningState::CALL_RUNNING_STATE_CREATE);
    if (call != nullptr) {
        return;
    }
    call = GetOneCarrierCallObject(CallRunningState::CALL_RUNNING_STATE_CONNECTING);
    if (call != nullptr) {
        return;
    }
    call = GetOneCarrierCallObject(CallRunningState::CALL_RUNNING_STATE_DIALING);
}

int32_t CallRequestProcess::CarrierDialProcess(DialParaInfo &info)
{
    auto callRequestEventHandler = DelayedSingleton<CallRequestEventHandlerHelper>::GetInstance();
    if (callRequestEventHandler->IsDialingCallProcessing()) {
        return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
    }
    callRequestEventHandler->RestoreDialingFlag(true);
    callRequestEventHandler->SetDialingCallProcessing();
    std::string newPhoneNum =
        DelayedSingleton<CallNumberUtils>::GetInstance()->RemoveSeparatorsPhoneNumber(info.number);
    int32_t ret = HandleDialingInfo(newPhoneNum, info);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("HandleDialingInfo failed!");
        callRequestEventHandler->RestoreDialingFlag(false);
        callRequestEventHandler->RemoveEventHandlerTask();
        needWaitHold_ = false;
        return ret;
    }
    std::string tempNumber = info.number;
    isFirstDialCallAdded_ = false;
    info.number = newPhoneNum;
    ret = UpdateCallReportInfo(info, TelCallState::CALL_STATUS_DIALING);
    info.number = tempNumber;
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallReportInfo failed!");
        callRequestEventHandler->RestoreDialingFlag(false);
        callRequestEventHandler->RemoveEventHandlerTask();
        needWaitHold_ = false;
        return ret;
    }
    return HandleStartDial(false, info);
}

int32_t CallRequestProcess::HandleDialingInfo(std::string newPhoneNum, DialParaInfo &info)
{
    bool isEcc = false;
    int32_t ret = HandleEccCallForDsda(newPhoneNum, info, isEcc);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    if (!isEcc) {
        bool canDial = true;
        IsNewCallAllowedCreate(canDial);
        if (!canDial) {
            return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
        }
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallRequestProcess::HandleStartDial(bool isMMiCode, DialParaInfo &info)
{
    CellularCallInfo callInfo;
    int32_t ret = PackCellularCallInfo(info, callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
        needWaitHold_ = false;
        return ret;
    }
    if (needWaitHold_ && !isMMiCode) {
        dialCallInfo_ = callInfo;
        return ret;
    }
    ret = DelayedSingleton<CellularCallConnection>::GetInstance()->Dial(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("Dial failed!");
        int32_t handleRet = HandleDialFail();
        if (handleRet != TELEPHONY_SUCCESS) {
            TELEPHONY_LOGE("HandleDialFail failed!");
            return handleRet;
        }
        return ret;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallRequestProcess::HandleEccCallForDsda(std::string newPhoneNum, DialParaInfo &info, bool &isEcc)
{
    int32_t ret =
        DelayedSingleton<CallNumberUtils>::GetInstance()->CheckNumberIsEmergency(newPhoneNum, info.accountId, isEcc);
    TELEPHONY_LOGI("CheckNumberIsEmergency ret is %{public}d, isEcc: %{public}d", ret, isEcc);
    if (isEcc) {
        return EccDialPolicy();
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallRequestProcess::PackCellularCallInfo(DialParaInfo &info, CellularCallInfo &callInfo)
{
    callInfo.callId = info.callId;
    callInfo.accountId = info.accountId;
    callInfo.callType = info.callType;
    callInfo.videoState = static_cast<int32_t>(info.videoState);
    callInfo.index = info.index;
    callInfo.slotId = info.accountId;
    callInfo.isRTT = info.isRTT;
    if (memset_s(callInfo.phoneNum, kMaxNumberLen, 0, kMaxNumberLen) != EOK) {
        TELEPHONY_LOGW("memset_s failed!");
        return TELEPHONY_ERR_MEMSET_FAIL;
    }
    if (info.number.length() > static_cast<size_t>(kMaxNumberLen)) {
        TELEPHONY_LOGE("Number out of limit!");
        return CALL_ERR_NUMBER_OUT_OF_RANGE;
    }
    if (memcpy_s(callInfo.phoneNum, kMaxNumberLen, info.number.c_str(), info.number.length()) != EOK) {
        TELEPHONY_LOGE("memcpy_s failed!");
        return TELEPHONY_ERR_MEMCPY_FAIL;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallRequestProcess::EccDialPolicy()
{
    std::list<int32_t> callIdList;
    std::list<sptr<CallBase>> hangupList;
    std::list<sptr<CallBase>> rejectList;
    GetCarrierCallList(callIdList);
    for (int32_t callId : callIdList) {
        sptr<CallBase> call = GetOneCallObject(callId);
        if (call == nullptr) {
            continue;
        }
        CallRunningState crState = call->GetCallRunningState();
        if (call->GetCallType() == CallType::TYPE_SATELLITE) {
            hangupList.emplace_back(call);
        } else if (crState == CallRunningState::CALL_RUNNING_STATE_CREATE ||
            crState == CallRunningState::CALL_RUNNING_STATE_CONNECTING ||
            crState == CallRunningState::CALL_RUNNING_STATE_DIALING) {
            if (call->GetEmergencyState()) {
                TELEPHONY_LOGE("already has ecc call dailing!");
                return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
            }
            hangupList.emplace_back(call);
        } else if (crState == CallRunningState::CALL_RUNNING_STATE_RINGING) {
            rejectList.emplace_back(call);
        } else if (crState == CallRunningState::CALL_RUNNING_STATE_ACTIVE ||
            crState == CallRunningState::CALL_RUNNING_STATE_HOLD) {
            hangupList.emplace_back(call);
        }
    }
    for (sptr<CallBase> call : hangupList) {
        TELEPHONY_LOGI("HangUpCall call[id:%{public}d]", call->GetCallID());
        call->HangUpCall();
    }
    for (sptr<CallBase> call : rejectList) {
        TELEPHONY_LOGI("RejectCall call[id:%{public}d]", call->GetCallID());
        call->RejectCall();
    }
    return TELEPHONY_SUCCESS;
}
} // namespace Telephony
} // namespace OHOS
