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

#include "conference_base.h"

#include <string_ex.h>

#include "call_manager_errors.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
ConferenceBase::ConferenceBase()
    : mainCallId_(ERR_ID), state_(CONFERENCE_STATE_IDLE), oldState_(CONFERENCE_STATE_IDLE), beginTime_(0),
      conferenceType_(CallType::TYPE_CS)
{
    subCallIdSet_.clear();
}

ConferenceBase::~ConferenceBase()
{
    subCallIdSet_.clear();
}

int32_t ConferenceBase::GetMainCall()
{
    std::lock_guard<ffrt::mutex> lock(conferenceMutex_);
    return mainCallId_;
}

int32_t ConferenceBase::SetMainCall(int32_t callId)
{
    if (callId < ERR_ID) {
        TELEPHONY_LOGE("callId is invalid:%{public}d", callId);
        return CALL_ERR_INVALID_CALLID;
    }
    std::lock_guard<ffrt::mutex> lock(conferenceMutex_);
    mainCallId_ = callId;
    return TELEPHONY_SUCCESS;
}

ConferenceState ConferenceBase::GetConferenceState()
{
    std::lock_guard<ffrt::mutex> lock(conferenceMutex_);
    return state_;
}

void ConferenceBase::SetConferenceState(ConferenceState state)
{
    std::lock_guard<ffrt::mutex> lock(conferenceMutex_);
    state_ = state;
}

void ConferenceBase::SetOldConferenceState(ConferenceState state)
{
    std::lock_guard<ffrt::mutex> lock(conferenceMutex_);
    oldState_ = state;
}

ConferenceState ConferenceBase::GetOldConferenceState()
{
    std::lock_guard<ffrt::mutex> lock(conferenceMutex_);
    return oldState_;
}

int32_t ConferenceBase::GetSubCallIdList(int32_t callId, std::vector<std::u16string> &callIdList)
{
    bool flag = false;
    callIdList.clear();
    std::lock_guard<ffrt::mutex> lock(conferenceMutex_);
    for (auto it = subCallIdSet_.begin(); it != subCallIdSet_.end(); ++it) {
        if (*it == callId) {
            flag = true;
        }
        callIdList.push_back(Str8ToStr16(std::to_string(*it)));
    }
    if (!flag) {
        callIdList.clear();
        return CALL_ERR_THE_CALL_IS_NOT_IN_THE_CONFERENCE;
    }
    return TELEPHONY_SUCCESS;
}

std::set<int32_t> ConferenceBase::GetSubCallIdList()
{
    std::lock_guard<ffrt::mutex> lock(conferenceMutex_);
    return subCallIdSet_;
}

int32_t ConferenceBase::GetCallIdListForConference(int32_t callId, std::vector<std::u16string> &callIdList)
{
    bool flag = false;
    callIdList.clear();
    std::lock_guard<ffrt::mutex> lock(conferenceMutex_);
    for (auto it = subCallIdSet_.begin(); it != subCallIdSet_.end(); ++it) {
        if (*it == callId) {
            flag = true;
        }
        callIdList.push_back(Str8ToStr16(std::to_string(*it)));
    }
    if (mainCallId_ == callId) {
        flag = true;
    }
    callIdList.push_back(Str8ToStr16(std::to_string(mainCallId_)));
    if (!flag) {
        callIdList.clear();
        return CALL_ERR_THE_CALL_IS_NOT_IN_THE_CONFERENCE;
    }
    return TELEPHONY_SUCCESS;
}
} // namespace Telephony
} // namespace OHOS
