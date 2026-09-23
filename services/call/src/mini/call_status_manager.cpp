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

// CS/IMS voice path only: anti-fraud, VoIP, Bluetooth, distributed and watch calls, motion recognition,
// incoming-call filtering and blocking, ring-once detection, number location, ECC location and DSDA are
// cut (the Mini build runs DSDS_MODE_V2). A call the table cannot hold is rejected through the stack
// (incoming) or fails the dial. RefreshCall removes the old object before adding its replacement, which
// keeps the old call id and would otherwise not fit a full table.

#include "call_status_manager.h"

#include <algorithm>
#include <ctime>
#include <securec.h>

#include "call_control_manager.h"
#include "call_manager_errors.h"
#include "call_number_utils.h"
#include "call_request_event_handler_helper.h"
#include "cellular_call_connection.h"
#include "cs_call.h"
#include "ims_call.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
namespace {
constexpr int32_t INIT_INDEX = 0;
} // namespace

CallStatusManager::CallStatusManager() {}

CallStatusManager::~CallStatusManager() {}

int32_t CallStatusManager::Init()
{
    for (int32_t i = 0; i < SLOT_NUM; i++) {
        callDetailsInfo_[i].callVec.clear();
        tmpCallDetailsInfo_[i].callVec.clear();
        priorVideoState_[i] = VideoStateType::TYPE_VOICE;
    }
    mEventIdTransferMap_.clear();
    InitCallBaseEvent();
    return TELEPHONY_SUCCESS;
}

void CallStatusManager::InitCallBaseEvent()
{
    mEventIdTransferMap_[RequestResultEventId::RESULT_DIAL_NO_CARRIER] = CallAbilityEventId::EVENT_DIAL_NO_CARRIER;
    mEventIdTransferMap_[RequestResultEventId::RESULT_HOLD_SEND_FAILED] = CallAbilityEventId::EVENT_HOLD_CALL_FAILED;
    mEventIdTransferMap_[RequestResultEventId::RESULT_SWAP_SEND_FAILED] = CallAbilityEventId::EVENT_SWAP_CALL_FAILED;
    mEventIdTransferMap_[RequestResultEventId::RESULT_COMBINE_SEND_FAILED] =
        CallAbilityEventId::EVENT_COMBINE_CALL_FAILED;
    mEventIdTransferMap_[RequestResultEventId::RESULT_SPLIT_SEND_FAILED] =
        CallAbilityEventId::EVENT_SPLIT_CALL_FAILED;
}

int32_t CallStatusManager::HandleCallReportInfo(const CallDetailInfo &info)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    callReportInfo_ = info;
    switch (info.state) {
        case TelCallState::CALL_STATUS_ACTIVE:
            ret = ActiveHandle(info);
            break;
        case TelCallState::CALL_STATUS_HOLDING:
            ret = HoldingHandle(info);
            break;
        case TelCallState::CALL_STATUS_DIALING:
            ret = DialingHandle(info);
            break;
        case TelCallState::CALL_STATUS_ALERTING:
            ret = AlertHandle(info);
            break;
        case TelCallState::CALL_STATUS_INCOMING:
            ret = IncomingHandle(info);
            break;
        case TelCallState::CALL_STATUS_WAITING:
        case TelCallState::CALL_STATUS_DISCONNECTED:
        case TelCallState::CALL_STATUS_DISCONNECTING:
            ret = HandleCallReportInfoEx(info);
            break;
        default:
            TELEPHONY_LOGE("Invalid call state!");
            break;
    }
    return ret;
}

// handle call state changes, incoming call, outgoing call.
int32_t CallStatusManager::HandleCallsReportInfo(const CallDetailsInfo &info)
{
    bool flag = false;
    int32_t curSlotId = info.slotId;
    if (!DelayedSingleton<CallNumberUtils>::GetInstance()->IsValidSlotId(curSlotId)) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    tmpCallDetailsInfo_[curSlotId].callVec.clear();
    tmpCallDetailsInfo_[curSlotId] = info;
    for (auto &it : info.callVec) {
        for (const auto &it1 : callDetailsInfo_[curSlotId].callVec) {
            if (it.index == it1.index) {
                // call state changes
                if (it.state != it1.state || it.mpty != it1.mpty || it.state == TelCallState::CALL_STATUS_ALERTING ||
                    it.callMode != it1.callMode || it.callType != it1.callType) {
                    HandleCallReportInfo(it);
                }
                flag = true;
                break;
            }
        }
        // incoming/outgoing call handle
        if (!flag || callDetailsInfo_[curSlotId].callVec.empty()) {
            HandleConnectingCallReportInfo(it);
        }
        flag = false;
    }
    // disconnected calls handle
    for (auto &it2 : callDetailsInfo_[curSlotId].callVec) {
        for (const auto &it3 : info.callVec) {
            if (it2.index == it3.index) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            it2.state = TelCallState::CALL_STATUS_DISCONNECTED;
            HandleCallReportInfo(it2);
        }
        flag = false;
    }
    UpdateCallDetailsInfo(info);
    return TELEPHONY_SUCCESS;
}

void CallStatusManager::HandleConnectingCallReportInfo(const CallDetailInfo &info)
{
    HandleCallReportInfo(info);
}

void CallStatusManager::UpdateCallDetailsInfo(const CallDetailsInfo &info)
{
    int32_t curSlotId = info.slotId;
    callDetailsInfo_[curSlotId].callVec.clear();
    callDetailsInfo_[curSlotId] = info;
    auto condition = [](CallDetailInfo i) { return i.state == TelCallState::CALL_STATUS_DISCONNECTED; };
    auto itEnd = std::remove_if(callDetailsInfo_[curSlotId].callVec.begin(),
        callDetailsInfo_[curSlotId].callVec.end(), condition);
    callDetailsInfo_[curSlotId].callVec.erase(itEnd, callDetailsInfo_[curSlotId].callVec.end());
    if (callDetailsInfo_[curSlotId].callVec.empty()) {
        tmpCallDetailsInfo_[curSlotId].callVec.clear();
    }
}

int32_t CallStatusManager::HandleDisconnectedCause(const DisconnectedDetails &details)
{
    bool ret = DelayedSingleton<CallControlManager>::GetInstance()->NotifyCallDestroyed(details);
    if (!ret) {
        TELEPHONY_LOGI("NotifyCallDestroyed failed!");
        return CALL_ERR_PHONE_CALLSTATE_NOTIFY_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallStatusManager::HandleEventResultReportInfo(const CellularCallEventInfo &info)
{
    if (info.eventType != CellularCallEventType::EVENT_REQUEST_RESULT_TYPE) {
        TELEPHONY_LOGE("unexpected type event occurs, eventId:%{public}d", info.eventId);
        return CALL_ERR_PHONE_TYPE_UNEXPECTED;
    }
    sptr<CallBase> call = GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_DIALING);
    if (call != nullptr) {
        int32_t ret = DealFailDial(call);
        TELEPHONY_LOGI("DealFailDial ret:%{public}d", ret);
    }
    CallEventInfo eventInfo;
    if (mEventIdTransferMap_.find(info.eventId) != mEventIdTransferMap_.end()) {
        eventInfo.eventId = mEventIdTransferMap_[info.eventId];
        DialParaInfo dialInfo;
        if (eventInfo.eventId == CallAbilityEventId::EVENT_DIAL_NO_CARRIER) {
            DelayedSingleton<CallControlManager>::GetInstance()->GetDialParaInfo(dialInfo);
            if (dialInfo.number.length() > static_cast<size_t>(kMaxNumberLen)) {
                TELEPHONY_LOGE("Number out of limit!");
                return CALL_ERR_NUMBER_OUT_OF_RANGE;
            }
            if (memcpy_s(eventInfo.phoneNum, kMaxNumberLen, dialInfo.number.c_str(), dialInfo.number.length()) != EOK) {
                TELEPHONY_LOGE("memcpy_s failed!");
                return TELEPHONY_ERR_MEMCPY_FAIL;
            }
        } else if (eventInfo.eventId == CallAbilityEventId::EVENT_COMBINE_CALL_FAILED) {
            sptr<CallBase> activeCall = GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_ACTIVE);
            if (activeCall != nullptr) {
                activeCall->HandleCombineConferenceFailEvent();
            }
        } else if (eventInfo.eventId == CallAbilityEventId::EVENT_HOLD_CALL_FAILED) {
            needWaitHold_ = false;
        }
        DelayedSingleton<CallControlManager>::GetInstance()->NotifyCallEventUpdated(eventInfo);
    } else {
        TELEPHONY_LOGW("unknown type Event, eventid %{public}d", info.eventId);
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallStatusManager::IncomingHandle(const CallDetailInfo &info)
{
    int32_t ret = TELEPHONY_SUCCESS;
    bool isExisted = false;
    ret = PrepareIncomingCall(info, isExisted);
    if (isExisted || ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    sptr<CallBase> call = CreateNewCall(info, CallDirection::CALL_DIRECTION_IN);
    if (call == nullptr) {
        TELEPHONY_LOGE("CreateNewCall failed!");
        return CALL_ERR_CALL_OBJECT_IS_NULL;
    }
    ret = AddOneCallObject(call);
    if (ret != TELEPHONY_SUCCESS) {
        // Nobody else knows about this call yet; let the network end it.
        TELEPHONY_LOGE("incoming call not admitted: %{public}d", ret);
        HandleRejectCall(call, false);
        return ret;
    }
    DelayedSingleton<CallControlManager>::GetInstance()->NotifyNewCallCreated(call);
    return FinalizeIncomingState(call, info.state);
}

int32_t CallStatusManager::FinalizeIncomingState(sptr<CallBase> &call, const TelCallState nextState)
{
    int32_t ret = UpdateCallState(call, nextState);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed!");
        return ret;
    }
    ret = FilterResultsDispose(call);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("FilterResultsDispose failed!");
    }
    return ret;
}

int32_t CallStatusManager::PrepareIncomingCall(const CallDetailInfo &info, bool &isExisted)
{
    return RefreshOldCall(info, isExisted);
}

int32_t CallStatusManager::HandleRejectCall(sptr<CallBase> &call, bool isBlock)
{
    if (call == nullptr) {
        TELEPHONY_LOGE("call is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    int32_t ret = call->SetTelCallState(TelCallState::CALL_STATUS_INCOMING);
    if (ret != TELEPHONY_SUCCESS && ret != CALL_ERR_NOT_NEW_STATE) {
        TELEPHONY_LOGE("Set CallState failed!");
        return ret;
    }
    ret = call->RejectCall();
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("RejectCall failed!");
    }
    return ret;
}

int32_t CallStatusManager::UpdateDialingCallInfo(const CallDetailInfo &info)
{
    sptr<CallBase> call = GetOneCallObjectByIndexSlotIdAndCallType(info.index, info.accountId, info.callType,
        info.phoneIndex);
    if (call != nullptr) {
        call = RefreshCallIfNecessary(call, info);
        return TELEPHONY_SUCCESS;
    }
    call = GetOneCallObjectByIndex(INIT_INDEX);
    if (call == nullptr) {
        TELEPHONY_LOGE("call is nullptr");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::string oriNum = call->GetAccountNumber();
    call = RefreshCallIfNecessary(call, info);
    call->SetCallIndex(info.index);
    call->SetBundleName(info.bundleName);
    call->SetSlotId(info.accountId);
    call->SetTelCallState(info.state);
    call->SetVideoStateType(info.callMode);
    call->SetCallType(info.callType);
    call->SetAccountNumber(oriNum);
    call->SetImsDomain(info.imsDomain);
    ClearPendingState(call);
    return TELEPHONY_SUCCESS;
}

int32_t CallStatusManager::DialingHandle(const CallDetailInfo &info)
{
    if (info.index > 0) {
        if (UpdateDialingHandle(info)) {
            return UpdateDialingCallInfo(info);
        }
    }
    sptr<CallBase> call = CreateNewCall(info, CallDirection::CALL_DIRECTION_OUT);
    if (call == nullptr) {
        TELEPHONY_LOGE("CreateNewCall failed!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    int32_t ret = AddOneCallObject(call);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("dialing call not admitted: %{public}d", ret);
        return ret;
    }
    auto callRequestEventHandler = DelayedSingleton<CallRequestEventHandlerHelper>::GetInstance();
    if (info.index == INIT_INDEX) {
        callRequestEventHandler->SetPendingMo(true, call->GetCallID());
        call->SetPhoneOrWatchDial(static_cast<int32_t>(PhoneOrWatchDial::WATCH_DIAL));
    }
    callRequestEventHandler->RestoreDialingFlag(false);
    callRequestEventHandler->RemoveEventHandlerTask();
    ret = call->DialingProcess();
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    DelayedSingleton<CallControlManager>::GetInstance()->NotifyNewCallCreated(call);
    ret = UpdateCallState(call, TelCallState::CALL_STATUS_DIALING);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed, errCode:%{public}d", ret);
    }
    return ret;
}

bool CallStatusManager::UpdateDialingHandle(const CallDetailInfo &info)
{
    sptr<CallBase> call = GetOneCallObjectByIndexSlotIdAndCallType(INIT_INDEX, info.accountId, info.callType,
        info.phoneIndex);
    if (call == nullptr) {
        call = GetOneCallObjectByIndexSlotIdAndCallType(info.index, info.accountId, info.callType, info.phoneIndex);
    }
    return call != nullptr;
}

int32_t CallStatusManager::ActiveHandle(const CallDetailInfo &info)
{
    sptr<CallBase> call = GetOneCallObjectByIndexSlotIdAndCallType(info.index, info.accountId, info.callType,
        info.phoneIndex);
    if (call == nullptr && !RefreshDialingStateByOtherState(call, info)) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    ClearPendingState(call);
    call = RefreshCallIfNecessary(call, info);
    SetOriginalCallTypeForActiveState(call);
    // call state change active, need to judge if launching a conference
    std::vector<sptr<CallBase>> conferenceCallList = GetConferenceCallList(call->GetSlotId());
    if (info.mpty == 1 && conferenceCallList.size() > 1) {
        SetConferenceCall(conferenceCallList);
    } else {
        call->ExitConference();
    }
    int32_t ret = UpdateCallState(call, TelCallState::CALL_STATUS_ACTIVE);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed, errCode:%{public}d", ret);
        return ret;
    }
    sptr<CallBase> holdCall = GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_HOLD);
    if (holdCall != nullptr) {
        holdCall->SetCanSwitchCallState(true);
    }
    return ret;
}

void CallStatusManager::SetConferenceCall(std::vector<sptr<CallBase>> conferenceCallList)
{
    for (auto conferenceCall : conferenceCallList) {
        conferenceCall->SetTelConferenceState(TelConferenceState::TEL_CONFERENCE_IDLE);
        if (conferenceCall->GetTelConferenceState() != TelConferenceState::TEL_CONFERENCE_ACTIVE) {
            conferenceCall->LaunchConference();
            UpdateCallState(conferenceCall, conferenceCall->GetTelCallState());
        }
    }
}

int32_t CallStatusManager::HoldingHandle(const CallDetailInfo &info)
{
    sptr<CallBase> call = GetOneCallObjectByIndexSlotIdAndCallType(info.index, info.accountId, info.callType,
        info.phoneIndex);
    if (call == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    // if the call is in a conference, it will exit, otherwise just set it holding
    call = RefreshCallIfNecessary(call, info);
    if (info.mpty == 1) {
        call->HoldConference();
    }
    return UpdateCallStateAndHandleDsdsMode(info, call);
}

int32_t CallStatusManager::WaitingHandle(const CallDetailInfo &info)
{
    return IncomingHandle(info);
}

int32_t CallStatusManager::AlertHandle(const CallDetailInfo &info)
{
    sptr<CallBase> call = GetOneCallObjectByIndexSlotIdAndCallType(info.index, info.accountId, info.callType,
        info.phoneIndex);
    if (call == nullptr && !RefreshDialingStateByOtherState(call, info)) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    ClearPendingState(call);
    if (call->GetTelCallState() == TelCallState::CALL_STATUS_DISCONNECTING) {
        TELEPHONY_LOGE("call is disconnecting.");
        return CALL_ERR_CALL_STATE_MISMATCH_OPERATION;
    }
    call = RefreshCallIfNecessary(call, info);
    int32_t ret = UpdateCallState(call, TelCallState::CALL_STATUS_ALERTING);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed, errCode:%{public}d", ret);
    }
    return ret;
}

int32_t CallStatusManager::DisconnectingHandle(const CallDetailInfo &info)
{
    sptr<CallBase> call = GetOneCallObjectByIndexSlotIdAndCallType(info.index, info.accountId, info.callType,
        info.phoneIndex);
    if (call == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    call = RefreshCallIfNecessary(call, info);
    SetOriginalCallTypeForDisconnectState(call);
    int32_t ret = UpdateCallState(call, TelCallState::CALL_STATUS_DISCONNECTING);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed, errCode:%{public}d", ret);
    }
    return ret;
}

int32_t CallStatusManager::DisconnectedHandle(const CallDetailInfo &info)
{
    sptr<CallBase> call = GetOneCallObjectByIndexSlotIdAndCallType(info.index, info.accountId, info.callType,
        info.phoneIndex);
    if (call == nullptr && !RefreshDialingStateByOtherState(call, info)) {
        TELEPHONY_LOGE("Call is Null");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    call = RefreshCallIfNecessary(call, info);
    RefreshCallDisconnectReason(call, static_cast<int32_t>(info.reason), info.message);
    ClearPendingState(call);
    SetOriginalCallTypeForDisconnectState(call);
    std::vector<std::u16string> callIdList;
    call->GetSubCallIdList(callIdList);
    CallRunningState previousState = call->GetCallRunningState();
    call->ExitConference();
    TelCallState priorState = call->GetTelCallState();
    UpdateCallState(call, TelCallState::CALL_STATUS_DISCONNECTED);
    int32_t callId = call->GetCallID();
    if (needWaitHold_ && GetCallNum(TelCallState::CALL_STATUS_ACTIVE) == 0) {
        needWaitHold_ = false;
        HandleDialWhenHolding(callId, call);
    }
    HandleHoldCallOrAutoAnswerCall(call, callIdList, previousState, priorState);
    std::vector<sptr<CallBase>> conferenceCallList = GetConferenceCallList(call->GetSlotId());
    if (conferenceCallList.size() == 1) {
        sptr<CallBase> leftOneConferenceCall = conferenceCallList[0];
        if (leftOneConferenceCall != nullptr &&
            leftOneConferenceCall->GetTelConferenceState() != TelConferenceState::TEL_CONFERENCE_IDLE) {
            leftOneConferenceCall->SetTelConferenceState(TelConferenceState::TEL_CONFERENCE_IDLE);
            UpdateCallState(leftOneConferenceCall, leftOneConferenceCall->GetTelCallState());
        }
    }
    return TELEPHONY_SUCCESS;
}

std::vector<sptr<CallBase>> CallStatusManager::GetConferenceCallList(int32_t slotId)
{
    if (slotId >= SLOT_NUM || slotId < 0) {
        return std::vector<sptr<CallBase>>();
    }
    std::vector<sptr<CallBase>> conferenceCallList;
    for (const auto &it : tmpCallDetailsInfo_[slotId].callVec) {
        if (it.mpty == 1) {
            sptr<CallBase> conferenceCall = GetOneCallObjectByIndexAndSlotId(it.index, it.accountId);
            if (conferenceCall != nullptr) {
                conferenceCallList.emplace_back(conferenceCall);
            }
        }
    }
    return conferenceCallList;
}

void CallStatusManager::HandleHoldCallOrAutoAnswerCall(const sptr<CallBase> call,
    std::vector<std::u16string> callIdList, CallRunningState previousState, TelCallState priorState)
{
    if (call == nullptr) {
        TELEPHONY_LOGE("call is null");
        return;
    }
    bool canUnHold = false;
    size_t size = callIdList.size();
    int32_t activeCallNum = GetCallNum(TelCallState::CALL_STATUS_ACTIVE);
    int32_t waitingCallNum = GetCallNum(TelCallState::CALL_STATUS_WAITING);
    IsCanUnHold(activeCallNum, waitingCallNum, static_cast<int32_t>(size), canUnHold);
    sptr<CallBase> holdCall = CallObjectManager::GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_HOLD);
    if (previousState != CallRunningState::CALL_RUNNING_STATE_HOLD &&
        previousState != CallRunningState::CALL_RUNNING_STATE_ACTIVE &&
        priorState == TelCallState::CALL_STATUS_DISCONNECTING) {
        if (holdCall != nullptr && canUnHold && holdCall->GetCanUnHoldState()) {
            if (holdCall->GetSlotId() == call->GetSlotId()) {
                holdCall->UnHoldCall();
            }
        }
    }
    DeleteOneCallObject(call->GetCallID());
}

void CallStatusManager::IsCanUnHold(int32_t activeCallNum, int32_t waitingCallNum, int32_t size, bool &canUnHold)
{
    int32_t incomingCallNum = GetCallNum(TelCallState::CALL_STATUS_INCOMING);
    int32_t answeredCallNum = GetCallNum(TelCallState::CALL_STATUS_ANSWERED);
    int32_t dialingCallNum = GetCallNum(TelCallState::CALL_STATUS_ALERTING);
    if (answeredCallNum == 0 && incomingCallNum == 0 && (size == 0 || size == 1) && activeCallNum == 0 &&
        waitingCallNum == 0 && dialingCallNum == 0) {
        canUnHold = true;
    }
}

int32_t CallStatusManager::UpdateCallState(sptr<CallBase> &call, TelCallState nextState)
{
    if (call == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    TelCallState priorState = call->GetTelCallState();
    TELEPHONY_LOGI("callIndex:%{public}d, callId:%{public}d, priorState:%{public}d, nextState:%{public}d",
        call->GetCallIndex(), call->GetCallID(), priorState, nextState);
    int32_t ret = call->SetTelCallState(nextState);
    UpdateOneCallObjectByCallId(call->GetCallID(), nextState);
    if (ret != TELEPHONY_SUCCESS && ret != CALL_ERR_NOT_NEW_STATE) {
        TELEPHONY_LOGE("SetTelCallState failed");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    if (!DelayedSingleton<CallControlManager>::GetInstance()->NotifyCallStateUpdated(call, priorState, nextState)) {
        TELEPHONY_LOGE(
            "NotifyCallStateUpdated failed! priorState:%{public}d,nextState:%{public}d", priorState, nextState);
        return CALL_ERR_PHONE_CALLSTATE_NOTIFY_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

sptr<CallBase> CallStatusManager::RefreshCallIfNecessary(const sptr<CallBase> &call, const CallDetailInfo &info)
{
    call->SetNewCallUseBox(info.newCallUseBox);
    if (call->GetCallType() == CallType::TYPE_IMS && call->GetVideoStateType() != info.callMode) {
        call->SetVideoStateType(info.callMode);
    }
    call->SetImsDomain(info.imsDomain);
    if (call->GetCallType() == CallType::TYPE_IMS) {
        call->SetCrsType(info.crsType);
    }
    if (call->GetCallType() == info.callType) {
        return call;
    }
    return RefreshCall(call, info);
}

sptr<CallBase> CallStatusManager::RefreshCall(const sptr<CallBase> &call, const CallDetailInfo &info)
{
    TelCallState priorState = call->GetTelCallState();
    CallAttributeInfo attrInfo;
    call->GetCallAttributeBaseInfo(attrInfo);
    sptr<CallBase> newCall = CreateNewCall(info, attrInfo.callDirection);
    if (newCall == nullptr) {
        TELEPHONY_LOGE("RefreshCall createCallFail");
        return call;
    }
    // The replacement keeps the old call id, so remove the old object first to free its table slot.
    sptr<CallBase> oldCall = call;
    DeleteOneCallObject(oldCall->GetCallID());
    newCall->SetCallId(oldCall->GetCallID());
    if (AddOneCallObject(newCall) != TELEPHONY_SUCCESS) {
        AddOneCallObject(oldCall);
        return oldCall;
    }
    newCall->SetCallRunningState(oldCall->GetCallRunningState());
    newCall->SetTelConferenceState(oldCall->GetTelConferenceState());
    newCall->SetStartTime(attrInfo.startTime);
    newCall->SetPolicyFlag(PolicyFlag(oldCall->GetPolicyFlag()));
    newCall->SetSpeakerphoneOn(oldCall->IsSpeakerphoneOn());
    newCall->SetCallBeginTime(attrInfo.callBeginTime);
    newCall->SetCallCreateTime(attrInfo.callCreateTime);
    newCall->SetCallEndTime(attrInfo.callEndTime);
    newCall->SetRingBeginTime(attrInfo.ringBeginTime);
    newCall->SetRingEndTime(attrInfo.ringEndTime);
    newCall->SetAnswerType(attrInfo.answerType);
    newCall->SetMicPhoneState(oldCall->IsMuted());
    if (oldCall->GetCallType() == CallType::TYPE_IMS && newCall->GetCallType() == CallType::TYPE_CS &&
        info.mpty == 1) {
        sptr<CSCall> csCall = static_cast<CSCall *>(newCall.GetRefPtr());
        csCall->UpdateConferenceId(oldCall->GetCallID());
    }
    newCall->SetTelCallState(priorState);
    newCall->SetImsDomain(attrInfo.imsDomain);
    if (oldCall->GetNumberLocation() != "default") {
        newCall->SetNumberLocation(oldCall->GetNumberLocation());
    }
    return newCall;
}

void CallStatusManager::SetOriginalCallTypeForActiveState(sptr<CallBase> &call)
{
    if (call == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return;
    }
    TelCallState priorState = call->GetTelCallState();
    VideoStateType videoState = call->GetVideoStateType();
    int32_t videoStateHistory = call->GetOriginalCallType();
    if (priorState == TelCallState::CALL_STATUS_ALERTING || priorState == TelCallState::CALL_STATUS_INCOMING ||
        priorState == TelCallState::CALL_STATUS_WAITING) {
        // outgoing/incoming video call, but accepted/answered with voice call
        if (videoStateHistory != static_cast<int32_t>(videoState)) {
            call->SetOriginalCallType(static_cast<int32_t>(videoState));
        }
    } else if (priorState == TelCallState::CALL_STATUS_ACTIVE || priorState == TelCallState::CALL_STATUS_HOLDING) {
        int32_t videoStateCurrent =
            static_cast<int32_t>(static_cast<uint32_t>(videoStateHistory) | static_cast<uint32_t>(videoState));
        call->SetOriginalCallType(videoStateCurrent);
    }
}

void CallStatusManager::SetOriginalCallTypeForDisconnectState(sptr<CallBase> &call)
{
    if (call == nullptr) {
        TELEPHONY_LOGE("Call is NULL");
        return;
    }
    TelCallState priorState = call->GetTelCallState();
    CallAttributeInfo attrInfo;
    call->GetCallAttributeBaseInfo(attrInfo);
    if (priorState == TelCallState::CALL_STATUS_DIALING || priorState == TelCallState::CALL_STATUS_ALERTING ||
        ((priorState == TelCallState::CALL_STATUS_INCOMING || priorState == TelCallState::CALL_STATUS_WAITING) &&
        attrInfo.answerType != CallAnswerType::CALL_ANSWER_REJECT)) {
        // outgoing/incoming video call, but canceled or missed
        call->SetOriginalCallType(static_cast<int32_t>(VideoStateType::TYPE_VOICE));
    }
}

sptr<CallBase> CallStatusManager::CreateNewCall(const CallDetailInfo &info, CallDirection dir)
{
    DialParaInfo paraInfo;
    AppExecFwk::PacMap extras;
    extras.Clear();
    PackParaInfo(paraInfo, info, dir, extras);
    sptr<CallBase> callPtr = CreateNewCallByCallType(paraInfo, info, dir, extras);
    if (callPtr == nullptr) {
        TELEPHONY_LOGE("CreateNewCall failed!");
        return nullptr;
    }
    DialScene dialScene = (DialScene)extras.GetIntValue("dialScene");
    if (dialScene == DialScene::CALL_EMERGENCY) {
        callPtr->SetIsEccContact(true);
    }
    callPtr->SetOriginalCallType(info.originalCallType);
    SetCallParams(callPtr, info);
    return callPtr;
}

void CallStatusManager::SetCallParams(const sptr<CallBase> &callPtr, const CallDetailInfo &info)
{
    time_t createTime = std::max(time(nullptr), static_cast<time_t>(0));
    callPtr->SetCallCreateTime(createTime);
}

sptr<CallBase> CallStatusManager::CreateNewCallByCallType(
    DialParaInfo &paraInfo, const CallDetailInfo &info, CallDirection dir, AppExecFwk::PacMap &extras)
{
    sptr<CallBase> callPtr = nullptr;
    switch (info.callType) {
        case CallType::TYPE_CS: {
            if (dir == CallDirection::CALL_DIRECTION_OUT) {
                callPtr = new (std::nothrow) CSCall(paraInfo, extras);
            } else {
                callPtr = new (std::nothrow) CSCall(paraInfo);
            }
            break;
        }
        case CallType::TYPE_IMS: {
            if (dir == CallDirection::CALL_DIRECTION_OUT) {
                callPtr = new (std::nothrow) IMSCall(paraInfo, extras);
            } else {
                callPtr = new (std::nothrow) IMSCall(paraInfo);
            }
            break;
        }
        default:
            TELEPHONY_LOGE("call type %{public}d is not supported", static_cast<int32_t>(info.callType));
            return nullptr;
    }
    return callPtr;
}

void CallStatusManager::PackParaInfo(
    DialParaInfo &paraInfo, const CallDetailInfo &info, CallDirection dir, AppExecFwk::PacMap &extras)
{
    paraInfo.isEcc = false;
    paraInfo.dialType = DialType::DIAL_CARRIER_TYPE;
    if (dir == CallDirection::CALL_DIRECTION_OUT) {
        DelayedSingleton<CallControlManager>::GetInstance()->GetDialParaInfo(paraInfo, extras);
    }
    paraInfo.number = info.phoneNum;
    paraInfo.callId = GetNewCallId();
    paraInfo.index = info.index;
    paraInfo.videoState = info.callMode;
    paraInfo.accountId = info.accountId;
    paraInfo.callType = info.callType;
    paraInfo.callState = info.state;
    paraInfo.bundleName = info.bundleName;
    paraInfo.crsType = info.crsType;
    paraInfo.originalCallType = info.originalCallType;
    paraInfo.extraParams =
        AAFwk::WantParamWrapper::ParseWantParamsWithBrackets(extras.GetStringValue("extraParams"));
    paraInfo.phoneOrWatch = info.phoneOrWatch;
    paraInfo.newCallUseBox = info.newCallUseBox;
    paraInfo.phoneIndex = info.phoneIndex;
}

int32_t CallStatusManager::UpdateCallStateAndHandleDsdsMode(const CallDetailInfo &info, sptr<CallBase> &call)
{
    if (call == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    int32_t ret = UpdateCallState(call, TelCallState::CALL_STATUS_HOLDING);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallState failed, errCode:%{public}d", ret);
    }
    return ret;
}

void CallStatusManager::HandleDialWhenHolding(int32_t callId, sptr<CallBase> &call)
{
    auto callRequestEventHandler = DelayedSingleton<CallRequestEventHandlerHelper>::GetInstance();
    if (callRequestEventHandler->IsPendingHangup()) {
        sptr<CallBase> holdCall = CallObjectManager::GetOneCallObject(callId);
        if (holdCall != nullptr) {
            holdCall->UnHoldCall();
        }
        int32_t pendingHangupCallId = callRequestEventHandler->GetPendingHangupCallId();
        sptr<CallBase> pendingHangupCall = CallObjectManager::GetOneCallObject(pendingHangupCallId);
        if (pendingHangupCall != nullptr) {
            UpdateCallState(pendingHangupCall, TelCallState::CALL_STATUS_DISCONNECTED);
            DeleteOneCallObject(pendingHangupCallId);
        }
        callRequestEventHandler->SetPendingHangup(false, -1);
        DelayedSingleton<CallControlManager>::GetInstance()->RemovePendingHangupProtectTask();
    } else {
        int32_t result = DelayedSingleton<CellularCallConnection>::GetInstance()->Dial(GetDialCallInfo());
        sptr<CallBase> dialCall = GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_DIALING);
        if (result != TELEPHONY_SUCCESS && dialCall != nullptr) {
            DealFailDial(call);
        }
    }
}

int32_t CallStatusManager::HandleCallReportInfoEx(const CallDetailInfo &info)
{
    int32_t ret = TELEPHONY_ERR_FAIL;
    switch (info.state) {
        case TelCallState::CALL_STATUS_WAITING:
            ret = WaitingHandle(info);
            break;
        case TelCallState::CALL_STATUS_DISCONNECTED:
            ret = DisconnectedHandle(info);
            break;
        case TelCallState::CALL_STATUS_DISCONNECTING:
            ret = DisconnectingHandle(info);
            break;
        default:
            TELEPHONY_LOGE("Invalid call state!");
            break;
    }
    return ret;
}

void CallStatusManager::ClearPendingState(sptr<CallBase> &call)
{
    auto callRequestEventHandler = DelayedSingleton<CallRequestEventHandlerHelper>::GetInstance();
    int32_t callId = call->GetCallID();
    if (callRequestEventHandler->HasPendingMo(callId)) {
        callRequestEventHandler->SetPendingMo(false, -1);
    }
    if (callRequestEventHandler->HasPendingHangup(callId)) {
        if (call->GetCallRunningState() != CallRunningState::CALL_RUNNING_STATE_ENDED) {
            call->HangUpCall();
        }
        callRequestEventHandler->SetPendingHangup(false, -1);
        DelayedSingleton<CallControlManager>::GetInstance()->RemovePendingHangupProtectTask();
    }
}

void CallStatusManager::RefreshCallDisconnectReason(const sptr<CallBase> &call, int32_t reason,
    const std::string &message)
{
    switch (reason) {
        case static_cast<int32_t>(RilDisconnectedReason::DISCONNECTED_REASON_CS_CALL_ANSWERED_ELSEWHER):
            if (call->GetCallType() == CallType::TYPE_CS) {
                call->SetAnswerType(CallAnswerType::CALL_ANSWERED_ELSEWHER);
            }
            break;
        case static_cast<int32_t>(RilDisconnectedReason::DISCONNECTED_REASON_ANSWERED_ELSEWHER): {
            std::string lowerStr = message;
            std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
            if (lowerStr.find("elsewhere") != std::string::npos) {
                call->SetAnswerType(CallAnswerType::CALL_ANSWERED_ELSEWHER);
            }
            break;
        }
        default:
            break;
    }
}

int32_t CallStatusManager::RefreshOldCall(const CallDetailInfo &info, bool &isExistedOldCall)
{
    sptr<CallBase> initCall = GetOneCallObjectByIndex(INIT_INDEX);
    if (initCall != nullptr) {
        DealFailDial(initCall);
    }
    sptr<CallBase> call = GetOneCallObjectByIndexSlotIdAndCallType(info.index, info.accountId, info.callType,
        info.phoneIndex);
    if (call == nullptr) {
        isExistedOldCall = false;
        return TELEPHONY_SUCCESS;
    }
    isExistedOldCall = true;
    auto oldCallType = call->GetCallType();
    auto videoState = call->GetVideoStateType();
    if (oldCallType != info.callType || call->GetTelCallState() != info.state || videoState != info.callMode) {
        call = RefreshCallIfNecessary(call, info);
        if (oldCallType != info.callType || videoState != info.callMode) {
            return UpdateCallState(call, info.state);
        }
    }
    return TELEPHONY_SUCCESS;
}

bool CallStatusManager::RefreshDialingStateByOtherState(sptr<CallBase> &call, const CallDetailInfo &info)
{
    sptr<CallBase> initCall = GetOneCallObjectByIndex(INIT_INDEX);
    if (initCall == nullptr) {
        TELEPHONY_LOGE("initCall is nullptr!");
        return false;
    }
    CallDetailInfo tempInfo = info;
    tempInfo.state = TelCallState::CALL_STATUS_DIALING;
    DialingHandle(tempInfo);
    call = GetOneCallObjectByIndexSlotIdAndCallType(info.index, info.accountId, info.callType, info.phoneIndex);
    if (call == nullptr) {
        TELEPHONY_LOGE("after recalling dialingHandle, call still is nullptr!");
        return false;
    }
    return true;
}
} // namespace Telephony
} // namespace OHOS
