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

// Conference control is cut: merging, splitting and kicking out fail before any state changes, while the
// bookkeeping that follows the network's reports is kept.

#include "cs_call.h"

#include "call_manager_errors.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
CSCall::CSCall(DialParaInfo &info) : CarrierCall(info) {}

CSCall::CSCall(DialParaInfo &info, AppExecFwk::PacMap &extras) : CarrierCall(info, extras) {}

CSCall::~CSCall() {}

int32_t CSCall::DialingProcess()
{
    return CarrierDialingProcess();
}

int32_t CSCall::AnswerCall(int32_t videoState, bool isRTT)
{
    return CarrierAnswerCall(videoState);
}

int32_t CSCall::RejectCall()
{
    return CarrierRejectCall();
}

int32_t CSCall::HangUpCall()
{
    return CarrierHangUpCall();
}

int32_t CSCall::HoldCall()
{
    return CarrierHoldCall();
}

int32_t CSCall::UnHoldCall()
{
    return CarrierUnHoldCall();
}

int32_t CSCall::SwitchCall()
{
    return CarrierSwitchCall();
}

void CSCall::GetCallAttributeInfo(CallAttributeInfo &info)
{
    GetCallAttributeCarrierInfo(info);
}

int32_t CSCall::SetMute(int32_t mute, int32_t slotId)
{
    return CarrierSetMute(mute, slotId);
}

int32_t CSCall::CombineConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

void CSCall::HandleCombineConferenceFailEvent()
{
    std::set<std::int32_t> subCallIdList = DelayedSingleton<CsConference>::GetInstance()->GetSubCallIdList();
    if (subCallIdList.empty()) {
        DelayedSingleton<CsConference>::GetInstance()->SetMainCall(ERR_ID);
    } else {
        DelayedSingleton<CsConference>::GetInstance()->SetMainCall(*subCallIdList.begin());
    }
    ConferenceState oldState = DelayedSingleton<CsConference>::GetInstance()->GetOldConferenceState();
    DelayedSingleton<CsConference>::GetInstance()->SetConferenceState(oldState);
}

int32_t CSCall::SeparateConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t CSCall::KickOutFromConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t CSCall::CanCombineConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t CSCall::CanSeparateConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t CSCall::CanKickOutFromConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t CSCall::LaunchConference()
{
    DelayedSingleton<CsConference>::GetInstance()->SetConferenceState(ConferenceState::CONFERENCE_STATE_IDLE);
    int32_t ret = DelayedSingleton<CsConference>::GetInstance()->JoinToConference(GetCallID());
    if (ret == TELEPHONY_SUCCESS) {
        SetTelConferenceState(TelConferenceState::TEL_CONFERENCE_ACTIVE);
    }
    return ret;
}

int32_t CSCall::ExitConference()
{
    int32_t ret = DelayedSingleton<CsConference>::GetInstance()->LeaveFromConference(GetCallID());
    if (ret == TELEPHONY_SUCCESS) {
        SetTelConferenceState(TelConferenceState::TEL_CONFERENCE_IDLE);
    }
    return ret;
}

int32_t CSCall::HoldConference()
{
    int32_t ret = DelayedSingleton<CsConference>::GetInstance()->HoldConference(GetCallID());
    if (ret == TELEPHONY_SUCCESS) {
        SetTelConferenceState(TelConferenceState::TEL_CONFERENCE_HOLDING);
    }
    return ret;
}

int32_t CSCall::GetMainCallId(int32_t &mainCallId)
{
    mainCallId = DelayedSingleton<CsConference>::GetInstance()->GetMainCall();
    return TELEPHONY_SUCCESS;
}

int32_t CSCall::GetSubCallIdList(std::vector<std::u16string> &callIdList)
{
    return DelayedSingleton<CsConference>::GetInstance()->GetSubCallIdList(GetCallID(), callIdList);
}

int32_t CSCall::GetCallIdListForConference(std::vector<std::u16string> &callIdList)
{
    return DelayedSingleton<CsConference>::GetInstance()->GetCallIdListForConference(GetCallID(), callIdList);
}

int32_t CSCall::IsSupportConferenceable()
{
    return CarrierCall::IsSupportConferenceable();
}

void CSCall::UpdateConferenceId(int32_t callId)
{
    DelayedSingleton<CsConference>::GetInstance()->UpdateConferenceId(callId);
}
} // namespace Telephony
} // namespace OHOS
