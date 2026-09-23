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

#ifndef CONFERENCE_BASE_H
#define CONFERENCE_BASE_H

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <memory>
#include "ffrt.h"

#include "call_manager_inner_type.h"

namespace OHOS {
namespace Telephony {
constexpr uint16_t CS_CONFERENCE_MIN_CALLS_CNT = 2;
constexpr uint16_t CS_CONFERENCE_MAX_CALLS_CNT = 5;

enum ConferenceState {
    CONFERENCE_STATE_IDLE = 0,
    CONFERENCE_STATE_CREATING,
    CONFERENCE_STATE_HOLDING,
    CONFERENCE_STATE_ACTIVE,
    CONFERENCE_STATE_LEAVING,
};

class ConferenceBase {
public:
    ConferenceBase();
    virtual ~ConferenceBase();
    int32_t GetMainCall(); // get host call id
    int32_t SetMainCall(int32_t callId);
    ConferenceState GetConferenceState();
    void SetConferenceState(ConferenceState state);
    virtual int32_t JoinToConference(int32_t callId) = 0;
    virtual int32_t LeaveFromConference(int32_t callId) = 0;
    virtual int32_t HoldConference(int32_t callId) = 0;
    virtual int32_t CanCombineConference() = 0;
    virtual int32_t CanSeparateConference() = 0;
    virtual int32_t CanKickOutFromConference() = 0;
    int32_t GetSubCallIdList(
        int32_t callId, std::vector<std::u16string> &callIdList); // get participant list except host
    std::set<int32_t> GetSubCallIdList();
    int32_t GetCallIdListForConference(
        int32_t callId, std::vector<std::u16string> &callIdList); // get participant list besides host
    ConferenceState GetOldConferenceState();
    void SetOldConferenceState(ConferenceState state);
    int32_t IsConferenceCallForMutiSim(int32_t callId);

protected:
    int32_t mainCallId_;
    ConferenceState state_;
    ConferenceState oldState_;
    std::set<int32_t> subCallIdSet_;
    ffrt::mutex conferenceMutex_;
    time_t beginTime_;
    CallType conferenceType_;
};
} // namespace Telephony
} // namespace OHOS

#endif
