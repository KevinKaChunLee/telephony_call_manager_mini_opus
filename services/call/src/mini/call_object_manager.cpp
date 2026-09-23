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

// The table holds at most CALL_MANAGER_MINI_MAX_CALL_OBJECTS calls, the last slot kept for emergency
// calls. VoIP, Bluetooth, RTT, video ring and the call UI ability are cut: their lists are always empty.

#include "call_object_manager.h"

#include <securec.h>

#include "call_manager_errors.h"
#include "call_manager_mini_config.h"
#include "call_number_utils.h"
#include "ims_conference.h"
#include "report_call_info_handler.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
std::list<sptr<CallBase>> CallObjectManager::callObjectPtrList_;
std::map<int32_t, CallAttributeInfo> CallObjectManager::voipCallObjectList_;
ffrt::mutex CallObjectManager::listMutex_;
int32_t CallObjectManager::callId_ = CALL_START_ID;
ffrt::condition_variable CallObjectManager::cv_;
bool CallObjectManager::isFirstDialCallAdded_ = false;
bool CallObjectManager::needWaitHold_ = false;
CellularCallInfo CallObjectManager::dialCallInfo_;
constexpr int32_t CRS_TYPE = 2;
static constexpr const char *VIDEO_RING_PATH_FIX_TAIL = ".mp4";
constexpr int32_t VIDEO_RING_PATH_FIX_TAIL_LENGTH = 4;
static constexpr const char *SYSTEM_VIDEO_RING = "system_video_ring";

CallObjectManager::CallObjectManager() {}

CallObjectManager::~CallObjectManager() {}

int32_t CallObjectManager::AddOneCallObject(sptr<CallBase> &call)
{
    if (call == nullptr) {
        TELEPHONY_LOGE("call is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin();
    for (; it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallID() == call->GetCallID()) {
            TELEPHONY_LOGE("this call has existed yet!");
            return CALL_ERR_PHONE_CALL_ALREADY_EXISTS;
        }
    }
    size_t limit = call->GetEmergencyState() ? MINI_MAX_CALL_OBJECTS :
        (MINI_MAX_CALL_OBJECTS - MINI_ECC_RESERVED_CALL_OBJECTS);
    if (callObjectPtrList_.size() >= limit) {
        TELEPHONY_LOGE("call table full: %{public}zu calls, ecc:%{public}d", callObjectPtrList_.size(),
            call->GetEmergencyState());
        return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
    }
    ConnectAbilityIfNeed(call);
    callObjectPtrList_.emplace_back(call);
    if (callObjectPtrList_.size() == ONE_CALL_EXIST) {
        if (callObjectPtrList_.front()->GetTelCallState() == TelCallState::CALL_STATUS_DIALING) {
            isFirstDialCallAdded_ = true;
            cv_.notify_all();
        }
    }
    TELEPHONY_LOGI("AddOneCallObject success! callId:%{public}d,call list size:%{public}zu", call->GetCallID(),
        callObjectPtrList_.size());
    return TELEPHONY_SUCCESS;
}

void CallObjectManager::ConnectAbilityIfNeed(sptr<CallBase> &call) {}

int32_t CallObjectManager::AddOneVoipCallObject(CallAttributeInfo info)
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t CallObjectManager::DeleteOneVoipCallObject(int32_t callId)
{
    return TELEPHONY_SUCCESS;
}

bool CallObjectManager::IsVoipCallExist()
{
    return false;
}

bool CallObjectManager::IsVoipCallExist(TelCallState callState, int32_t &callId)
{
    return false;
}

void CallObjectManager::ClearVoipList() {}

int32_t CallObjectManager::UpdateOneVoipCallObjectByCallId(int32_t callId, TelCallState nextCallState)
{
    return TELEPHONY_ERROR;
}

CallAttributeInfo CallObjectManager::GetVoipCallInfo()
{
    return CallAttributeInfo();
}

CallAttributeInfo CallObjectManager::GetActiveVoipCallInfo()
{
    return CallAttributeInfo();
}

void CallObjectManager::DelayedDisconnectCallConnectAbility(uint64_t time) {}

int32_t CallObjectManager::DeleteOneCallObject(int32_t callId)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallID() == callId) {
            callObjectPtrList_.erase(it);
            TELEPHONY_LOGI("DeleteOneCallObject success! call list size:%{public}zu", callObjectPtrList_.size());
            break;
        }
    }
    return TELEPHONY_SUCCESS;
}

void CallObjectManager::DeleteOneCallObject(sptr<CallBase> &call)
{
    if (call == nullptr) {
        TELEPHONY_LOGE("call is null!");
        return;
    }
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    callObjectPtrList_.remove(call);
}

sptr<CallBase> CallObjectManager::GetOneCallObject(int32_t callId)
{
    sptr<CallBase> retPtr = nullptr;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it = CallObjectManager::callObjectPtrList_.begin();
    for (; it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallID() == callId) {
            retPtr = *it;
            break;
        }
    }
    return retPtr;
}

sptr<CallBase> CallObjectManager::GetOneCallObject(std::string &phoneNumber)
{
    if (phoneNumber.empty()) {
        TELEPHONY_LOGE("call is null!");
        return nullptr;
    }
    sptr<CallBase> retPtr = nullptr;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin();
    for (; it != callObjectPtrList_.end(); ++it) {
        std::string networkAddress =
            DelayedSingleton<CallNumberUtils>::GetInstance()->RemovePostDialPhoneNumber((*it)->GetAccountNumber());
        if (networkAddress == phoneNumber) {
            retPtr = *it;
            break;
        }
    }
    return retPtr;
}

int32_t CallObjectManager::HasNewCall()
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallType() != CallType::TYPE_VOIP &&
            ((*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_CREATE ||
            (*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_CONNECTING ||
            (*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_DIALING ||
            (*it)->GetCallType() == CallType::TYPE_SATELLITE)) {
            TELEPHONY_LOGE("there is already a new call[callId:%{public}d,state:%{public}d], please redial later",
                (*it)->GetCallID(), (*it)->GetCallRunningState());
            return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
        }
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallObjectManager::IsNewCallAllowedCreate(bool &enabled)
{
    enabled = true;
    {
        std::lock_guard<ffrt::mutex> lock(listMutex_);
        std::list<sptr<CallBase>>::iterator it;
        for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
            if ((*it)->GetCallType() != CallType::TYPE_VOIP &&
                ((*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_CREATE ||
                (*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_CONNECTING ||
                (*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_DIALING ||
                (*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_RINGING)) {
                TELEPHONY_LOGE("there is already a new call, please redial later");
                enabled = false;
                return TELEPHONY_ERR_SUCCESS;
            }
        }
    }
    int32_t count = 0;
    int32_t callNum = 2;
    std::list<int32_t> callIdList;
    GetCarrierCallList(callIdList);
    for (int32_t otherCallId : callIdList) {
        sptr<CallBase> call = GetOneCallObject(otherCallId);
        if (call != nullptr) {
            TelConferenceState confState = call->GetTelConferenceState();
            int32_t conferenceId = DelayedSingleton<ImsConference>::GetInstance()->GetMainCall();
            if (confState != TelConferenceState::TEL_CONFERENCE_IDLE && conferenceId == otherCallId) {
                count++;
            } else if (confState == TelConferenceState::TEL_CONFERENCE_IDLE) {
                count++;
            }
        }
    }
    if (count >= callNum) {
        enabled = false;
    }
    return TELEPHONY_ERR_SUCCESS;
}

int32_t CallObjectManager::GetCurrentCallNum()
{
    int32_t count = 0;
    std::list<int32_t> callIdList;
    GetCarrierCallList(callIdList);
    for (int32_t otherCallId : callIdList) {
        sptr<CallBase> call = GetOneCallObject(otherCallId);
        if (call != nullptr) {
            TelConferenceState confState = call->GetTelConferenceState();
            int32_t conferenceId = DelayedSingleton<ImsConference>::GetInstance()->GetMainCall();
            if (confState != TelConferenceState::TEL_CONFERENCE_IDLE && conferenceId == otherCallId) {
                count++;
            } else if (confState == TelConferenceState::TEL_CONFERENCE_IDLE) {
                count++;
            }
        }
    }
    return count;
}

int32_t CallObjectManager::GetCarrierCallList(std::list<int32_t> &list)
{
    list.clear();
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallType() == CallType::TYPE_CS || (*it)->GetCallType() == CallType::TYPE_IMS ||
            (*it)->GetCallType() == CallType::TYPE_SATELLITE ||
            (*it)->GetCallType() == CallType::TYPE_BLUETOOTH) {
            list.emplace_back((*it)->GetCallID());
        }
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallObjectManager::GetVoipCallNum(bool isOnlyIncludeNonVirtualVoIP)
{
    return 0;
}

int32_t CallObjectManager::GetVoipCallList(std::list<int32_t> &list)
{
    list.clear();
    return TELEPHONY_SUCCESS;
}

bool CallObjectManager::HasRingingMaximum()
{
    int32_t ringingCount = 0;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_RINGING) {
            ringingCount++;
        }
    }
    return ringingCount >= RINGING_CALL_NUMBER_LEN;
}

bool CallObjectManager::HasDialingMaximum()
{
    int32_t dialingCount = 0;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_ACTIVE) {
            dialingCount++;
        }
    }
    return dialingCount >= DIALING_CALL_NUMBER_LEN;
}

int32_t CallObjectManager::HasEmergencyCall(bool &enabled)
{
    enabled = false;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetEmergencyState()) {
            enabled = true;
        }
    }
    return TELEPHONY_ERR_SUCCESS;
}

int32_t CallObjectManager::GetNewCallId()
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    return ++callId_;
}

bool CallObjectManager::IsCallExist(int32_t callId)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin();
    for (; it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallID() == callId) {
            return true;
        }
    }
    return false;
}

bool CallObjectManager::IsCallExist(std::string &phoneNumber)
{
    if (phoneNumber.empty()) {
        return false;
    }
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin();
    for (; it != callObjectPtrList_.end(); ++it) {
        std::string networkAddress =
            DelayedSingleton<CallNumberUtils>::GetInstance()->RemovePostDialPhoneNumber((*it)->GetAccountNumber());
        if (networkAddress == phoneNumber) {
            return true;
        }
    }
    return false;
}

bool CallObjectManager::HasCallExist()
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    return !callObjectPtrList_.empty();
}

std::list<sptr<CallBase>> CallObjectManager::GetAllCallList()
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    return callObjectPtrList_;
}

bool CallObjectManager::HasCellularCallExist()
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallType() == CallType::TYPE_CS || (*it)->GetCallType() == CallType::TYPE_IMS ||
            (*it)->GetCallType() == CallType::TYPE_SATELLITE ||
            (*it)->GetCallType() == CallType::TYPE_BLUETOOTH) {
            if ((*it)->GetTelCallState() != TelCallState::CALL_STATUS_DISCONNECTED &&
                (*it)->GetTelCallState() != TelCallState::CALL_STATUS_DISCONNECTING) {
                return true;
            }
        }
    }
    return false;
}

bool CallObjectManager::HasVoipCallExist()
{
    return false;
}

bool CallObjectManager::HasOtherBtCallExist(int32_t callId)
{
    return false;
}

bool CallObjectManager::HasIncomingCallCrsType()
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_RINGING &&
            (*it)->GetCrsType() == CRS_TYPE) {
            return true;
        }
    }
    return false;
}

bool CallObjectManager::HasIncomingCallVideoRingType()
{
    return false;
}

bool CallObjectManager::IsVideoRing(const std::string &personalNotificationRingtone, const std::string &ringtonePath)
{
    return (personalNotificationRingtone.length() > VIDEO_RING_PATH_FIX_TAIL_LENGTH &&
        personalNotificationRingtone.substr(personalNotificationRingtone.length() - VIDEO_RING_PATH_FIX_TAIL_LENGTH,
        VIDEO_RING_PATH_FIX_TAIL_LENGTH) == VIDEO_RING_PATH_FIX_TAIL) || ringtonePath == SYSTEM_VIDEO_RING;
}

bool CallObjectManager::HasVideoCall()
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetVideoStateType() == VideoStateType::TYPE_VIDEO && (*it)->GetCallType() != CallType::TYPE_VOIP) {
            return true;
        }
    }
    return false;
}

bool CallObjectManager::HasSatelliteCallExist()
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallType() == CallType::TYPE_SATELLITE) {
            return true;
        }
    }
    return false;
}

int32_t CallObjectManager::GetSatelliteCallList(std::list<int32_t> &list)
{
    list.clear();
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallType() == CallType::TYPE_SATELLITE) {
            list.emplace_back((*it)->GetCallID());
        }
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallObjectManager::HasRingingCall(bool &hasRingingCall)
{
    hasRingingCall = false;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_RINGING) {
            hasRingingCall = true;
            break;
        }
    }
    return TELEPHONY_ERR_SUCCESS;
}

int32_t CallObjectManager::HasHoldCall(bool &hasHoldCall)
{
    hasHoldCall = false;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallRunningState() == CallRunningState::CALL_RUNNING_STATE_HOLD) {
            hasHoldCall = true;
            break;
        }
    }
    return TELEPHONY_ERR_SUCCESS;
}

TelCallState CallObjectManager::GetCallState(int32_t callId)
{
    TelCallState retState = TelCallState::CALL_STATUS_IDLE;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it = CallObjectManager::callObjectPtrList_.begin();
    for (; it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallID() == callId) {
            retState = (*it)->GetTelCallState();
            break;
        }
    }
    return retState;
}

sptr<CallBase> CallObjectManager::GetOneCallObject(CallRunningState callState)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::reverse_iterator it;
    for (it = callObjectPtrList_.rbegin(); it != callObjectPtrList_.rend(); ++it) {
        if ((*it)->GetCallRunningState() == callState) {
            return (*it);
        }
    }
    return nullptr;
}

sptr<CallBase> CallObjectManager::GetOneCarrierCallObject(CallRunningState callState)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::reverse_iterator it;
    for (it = callObjectPtrList_.rbegin(); it != callObjectPtrList_.rend(); ++it) {
        if ((*it)->GetCallRunningState() == callState && (*it)->GetCallType() != CallType::TYPE_VOIP) {
            return (*it);
        }
    }
    return nullptr;
}

sptr<CallBase> CallObjectManager::GetOneCallObjectByIndex(int32_t index)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin();
    for (; it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallIndex() == index && (*it)->GetCallType() != CallType::TYPE_VOIP) {
            return (*it);
        }
    }
    return nullptr;
}

sptr<CallBase> CallObjectManager::GetDialingCall()
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin();
    for (; it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetTelCallState() == TelCallState::CALL_STATUS_DIALING) {
            return (*it);
        }
    }
    return nullptr;
}

sptr<CallBase> CallObjectManager::GetOneCallObjectByIndexAndSlotId(int32_t index, int32_t slotId)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin();
    for (; it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallIndex() == index) {
            if ((*it)->GetSlotId() == slotId && (*it)->GetCallType() != CallType::TYPE_VOIP) {
                return (*it);
            }
        }
    }
    return nullptr;
}

sptr<CallBase> CallObjectManager::GetOneCallObjectByIndexSlotIdAndCallType(int32_t index, int32_t slotId,
    CallType callType, int32_t phoneIndex)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin();
    for (; it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallType() != CallType::TYPE_BLUETOOTH && (*it)->GetCallIndex() == index &&
            (*it)->GetSlotId() == slotId && (*it)->GetCallType() != CallType::TYPE_VOIP) {
            return (*it);
        }
    }
    return nullptr;
}

sptr<CallBase> CallObjectManager::GetOneCallObjectByVoipCallId(
    std::string voipCallId, std::string bundleName, int32_t uid)
{
    return nullptr;
}

bool CallObjectManager::IsCallExist(CallType callType, TelCallState callState)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallType() == callType && (*it)->GetTelCallState() == callState) {
            return true;
        }
    }
    return false;
}

bool CallObjectManager::IsCallExist(TelCallState callState)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetTelCallState() == callState) {
            return true;
        }
    }
    return false;
}

bool CallObjectManager::IsCallExist(TelCallState callState, int32_t &callId)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetTelCallState() == callState) {
            callId = (*it)->GetCallID();
            return true;
        }
    }
    return false;
}

bool CallObjectManager::IsConferenceCallExist(TelConferenceState state, int32_t &callId)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetTelConferenceState() == state) {
            callId = (*it)->GetCallID();
            return true;
        }
    }
    return false;
}

bool CallObjectManager::HasActivedCallExist(int32_t &callId, bool isIncludeVoipCall)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetTelCallState() == TelCallState::CALL_STATUS_ACTIVE &&
            (*it)->GetCallType() != CallType::TYPE_VOIP) {
            callId = (*it)->GetCallID();
            return true;
        }
    }
    return false;
}

int32_t CallObjectManager::GetCallNum(TelCallState callState, bool isIncludeVoipCall)
{
    int32_t num = 0;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetTelCallState() == callState && (*it)->GetCallType() != CallType::TYPE_VOIP) {
            ++num;
        }
    }
    return num;
}

std::string CallObjectManager::GetCallNumber(TelCallState callState, bool isIncludeVoipCall)
{
    std::string number = "";
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetTelCallState() == callState && (*it)->GetCallType() != CallType::TYPE_VOIP) {
            number = (*it)->GetAccountNumber();
            break;
        }
    }
    return number;
}

std::vector<CallAttributeInfo> CallObjectManager::GetCallInfoList(int32_t slotId, bool isIncludeVoipCall)
{
    std::vector<CallAttributeInfo> callVec;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallType() == CallType::TYPE_VOIP) {
            continue;
        }
        CallAttributeInfo info;
        (*it)->GetCallAttributeInfo(info);
        if (info.accountId == slotId && info.callType != CallType::TYPE_OTT) {
            callVec.emplace_back(info);
        }
    }
    return callVec;
}

std::vector<CallAttributeInfo> CallObjectManager::GetVoipCallInfoList()
{
    return std::vector<CallAttributeInfo>();
}

void CallObjectManager::UpdateOneCallObjectByCallId(int32_t callId, TelCallState nextCallState)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin();
    for (; it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallID() == callId) {
            (*it)->SetTelCallState(nextCallState);
        }
    }
}

sptr<CallBase> CallObjectManager::GetForegroundCall(bool isIncludeVoipCall)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    sptr<CallBase> liveCall = nullptr;
    for (std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if (!isIncludeVoipCall && (*it)->GetCallType() == CallType::TYPE_VOIP) {
            continue;
        }
        TelCallState telCallState = (*it)->GetTelCallState();
        if (telCallState == TelCallState::CALL_STATUS_WAITING ||
            telCallState == TelCallState::CALL_STATUS_INCOMING) {
            liveCall = (*it);
            break;
        }
        if (telCallState == TelCallState::CALL_STATUS_ALERTING ||
            telCallState == TelCallState::CALL_STATUS_DIALING ||
            telCallState == TelCallState::CALL_STATUS_ACTIVE ||
            telCallState == TelCallState::CALL_STATUS_HOLDING) {
            liveCall = (*it);
        }
    }
    return liveCall;
}

sptr<CallBase> CallObjectManager::GetForegroundLiveCall(bool isIncludeVoipCall)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    sptr<CallBase> liveCall = nullptr;
    for (std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if (!isIncludeVoipCall && (*it)->GetCallType() == CallType::TYPE_VOIP) {
            continue;
        }
        TelCallState telCallState = (*it)->GetTelCallState();
        if (telCallState == TelCallState::CALL_STATUS_ACTIVE ||
            telCallState == TelCallState::CALL_STATUS_ALERTING ||
            telCallState == TelCallState::CALL_STATUS_DIALING) {
            liveCall = (*it);
            break;
        }
    }
    return liveCall;
}

sptr<CallBase> CallObjectManager::GetIncomingCall(bool isIncludeVoipCall)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    sptr<CallBase> call = nullptr;
    for (std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if (!isIncludeVoipCall && (*it)->GetCallType() == CallType::TYPE_VOIP) {
            continue;
        }
        TelCallState callState = (*it)->GetTelCallState();
        if (callState == TelCallState::CALL_STATUS_INCOMING || callState == TelCallState::CALL_STATUS_WAITING) {
            call = (*it);
            break;
        }
    }
    return call;
}

sptr<CallBase> CallObjectManager::GetAudioLiveCall()
{
    sptr<CallBase> call = GetForegroundLiveCall(false);
    if (call == nullptr) {
        call = GetIncomingCall(false);
    }
    if (call == nullptr) {
        call = GetOneCarrierCallObject(CallRunningState::CALL_RUNNING_STATE_HOLD);
    }
    return call;
}

CellularCallInfo CallObjectManager::GetDialCallInfo()
{
    return dialCallInfo_;
}

int32_t CallObjectManager::DealFailDial(sptr<CallBase> call)
{
    return ReportCallDisconnected(call);
}

int32_t CallObjectManager::ReportCallDisconnected(sptr<CallBase> call)
{
    if (call == nullptr) {
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallDetailInfo callDetatilInfo;
    std::string number = call->GetAccountNumber();
    callDetatilInfo.callType = call->GetCallType();
    callDetatilInfo.accountId = call->GetSlotId();
    callDetatilInfo.index = call->GetCallIndex();
    callDetatilInfo.state = TelCallState::CALL_STATUS_DISCONNECTED;
    callDetatilInfo.callMode = call->GetVideoStateType();
    callDetatilInfo.voiceDomain = static_cast<int32_t>(call->GetCallType());
    if (number.length() > kMaxNumberLen) {
        TELEPHONY_LOGE("numbser length out of range");
        return CALL_ERR_NUMBER_OUT_OF_RANGE;
    }
    if (memcpy_s(&callDetatilInfo.phoneNum, kMaxNumberLen, number.c_str(), number.length()) != EOK) {
        TELEPHONY_LOGE("memcpy_s number failed!");
        return TELEPHONY_ERR_MEMCPY_FAIL;
    }
    return DelayedSingleton<ReportCallInfoHandler>::GetInstance()->UpdateCallReportInfo(callDetatilInfo);
}

std::vector<CallAttributeInfo> CallObjectManager::GetAllCallInfoList(bool isIncludeVoipCall)
{
    std::vector<CallAttributeInfo> callVec;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it) == nullptr || (*it)->GetCallType() == CallType::TYPE_VOIP) {
            continue;
        }
        CallAttributeInfo info;
        (*it)->GetCallAttributeInfo(info);
        callVec.emplace_back(info);
    }
    return callVec;
}

int32_t CallObjectManager::GetCallNumByRunningState(CallRunningState callState)
{
    int32_t count = 0;
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    std::list<sptr<CallBase>>::iterator it;
    for (it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallRunningState() == callState) {
            count++;
        }
    }
    return count;
}

sptr<CallBase> CallObjectManager::GetForegroundLiveCallByCallId(int32_t callId)
{
    std::lock_guard<ffrt::mutex> lock(listMutex_);
    sptr<CallBase> liveCall = nullptr;
    for (std::list<sptr<CallBase>>::iterator it = callObjectPtrList_.begin(); it != callObjectPtrList_.end(); ++it) {
        if ((*it)->GetCallID() != callId) {
            continue;
        }
        TelCallState telCallState = (*it)->GetTelCallState();
        if (telCallState == TelCallState::CALL_STATUS_ACTIVE || telCallState == TelCallState::CALL_STATUS_ALERTING ||
            telCallState == TelCallState::CALL_STATUS_DIALING) {
            liveCall = (*it);
            break;
        }
        if (telCallState == TelCallState::CALL_STATUS_WAITING || telCallState == TelCallState::CALL_STATUS_INCOMING) {
            liveCall = (*it);
        }
    }
    return liveCall;
}

bool CallObjectManager::IsNeedSilentInDoNotDisturbMode()
{
    sptr<CallBase> foregroundCall = CallObjectManager::GetForegroundCall(false);
    if (foregroundCall == nullptr) {
        return false;
    }
    if (foregroundCall->GetTelCallState() == TelCallState::CALL_STATUS_INCOMING) {
        return foregroundCall->GetParamsByKey("IsNeedSilentInDoNotDisturbMode", 0) == 1;
    }
    return false;
}

bool CallObjectManager::HasRttCall()
{
    return false;
}
} // namespace Telephony
} // namespace OHOS
