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

// Voice-only IMS calls (see ims_call.h): conference control is cut as in cs_call.cpp and the video entry
// points fail with CALL_ERR_VIDEO_NOT_SUPPORTED.

#include "ims_call.h"

#include "call_manager_errors.h"
#include "ims_conference.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
IMSCall::IMSCall(DialParaInfo &info) : CarrierCall(info) {}

IMSCall::IMSCall(DialParaInfo &info, AppExecFwk::PacMap &extras) : CarrierCall(info, extras) {}

IMSCall::~IMSCall() {}

int32_t IMSCall::InitVideoCall()
{
    return CALL_ERR_VIDEO_NOT_SUPPORTED;
}

int32_t IMSCall::DialingProcess()
{
    return CarrierDialingProcess();
}

int32_t IMSCall::AnswerCall(int32_t videoState, bool isRTT)
{
    return CarrierAnswerCall(videoState, isRTT);
}

int32_t IMSCall::RejectCall()
{
    return CarrierRejectCall();
}

int32_t IMSCall::HangUpCall()
{
    return CarrierHangUpCall();
}

int32_t IMSCall::HoldCall()
{
    return CarrierHoldCall();
}

int32_t IMSCall::UnHoldCall()
{
    return CarrierUnHoldCall();
}

int32_t IMSCall::SwitchCall()
{
    return CarrierSwitchCall();
}

int32_t IMSCall::SetMute(int32_t mute, int32_t slotId)
{
    return CarrierSetMute(mute, slotId);
}

void IMSCall::GetCallAttributeInfo(CallAttributeInfo &info)
{
    GetCallAttributeCarrierInfo(info);
}

int32_t IMSCall::CombineConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

void IMSCall::HandleCombineConferenceFailEvent()
{
    std::set<std::int32_t> subCallIdList = DelayedSingleton<ImsConference>::GetInstance()->GetSubCallIdList();
    if (subCallIdList.empty()) {
        DelayedSingleton<ImsConference>::GetInstance()->SetMainCall(ERR_ID);
    } else {
        DelayedSingleton<ImsConference>::GetInstance()->SetMainCall(*subCallIdList.begin());
    }
    ConferenceState oldState = DelayedSingleton<ImsConference>::GetInstance()->GetOldConferenceState();
    DelayedSingleton<ImsConference>::GetInstance()->SetConferenceState(oldState);
}

int32_t IMSCall::SeparateConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t IMSCall::KickOutFromConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t IMSCall::CanCombineConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t IMSCall::CanSeparateConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t IMSCall::CanKickOutFromConference()
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t IMSCall::GetMainCallId(int32_t &mainCallId)
{
    mainCallId = DelayedSingleton<ImsConference>::GetInstance()->GetMainCall();
    return TELEPHONY_SUCCESS;
}

int32_t IMSCall::LaunchConference()
{
    int32_t ret = DelayedSingleton<ImsConference>::GetInstance()->JoinToConference(GetCallID());
    if (ret == TELEPHONY_SUCCESS) {
        SetTelConferenceState(TelConferenceState::TEL_CONFERENCE_ACTIVE);
    }
    return ret;
}

int32_t IMSCall::ExitConference()
{
    int32_t ret = DelayedSingleton<ImsConference>::GetInstance()->LeaveFromConference(GetCallID());
    if (ret == TELEPHONY_SUCCESS) {
        SetTelConferenceState(TelConferenceState::TEL_CONFERENCE_IDLE);
    }
    return ret;
}

int32_t IMSCall::HoldConference()
{
    int32_t ret = DelayedSingleton<ImsConference>::GetInstance()->HoldConference(GetCallID());
    if (ret == TELEPHONY_SUCCESS) {
        SetTelConferenceState(TelConferenceState::TEL_CONFERENCE_HOLDING);
    }
    return ret;
}

int32_t IMSCall::GetSubCallIdList(std::vector<std::u16string> &callIdList)
{
    return DelayedSingleton<ImsConference>::GetInstance()->GetSubCallIdList(GetCallID(), callIdList);
}

int32_t IMSCall::GetCallIdListForConference(std::vector<std::u16string> &callIdList)
{
    return DelayedSingleton<ImsConference>::GetInstance()->GetCallIdListForConference(GetCallID(), callIdList);
}

int32_t IMSCall::IsSupportConferenceable()
{
    return CarrierCall::IsSupportConferenceable();
}

int32_t IMSCall::UpdateImsCallMode(ImsCallMode mode)
{
    return CALL_ERR_VIDEO_NOT_SUPPORTED;
}

int32_t IMSCall::SendUpdateCallMediaModeRequest(ImsCallMode mode)
{
    return CALL_ERR_VIDEO_NOT_SUPPORTED;
}

int32_t IMSCall::RecieveUpdateCallMediaModeRequest(CallModeReportInfo &response)
{
    return CALL_ERR_VIDEO_NOT_SUPPORTED;
}

int32_t IMSCall::SendUpdateCallMediaModeResponse(ImsCallMode mode)
{
    return CALL_ERR_VIDEO_NOT_SUPPORTED;
}

int32_t IMSCall::ReceiveUpdateCallMediaModeResponse(CallModeReportInfo &response)
{
    return CALL_ERR_VIDEO_NOT_SUPPORTED;
}

int32_t IMSCall::ControlCamera(std::string &cameraId, int32_t callingUid, int32_t callingPid)
{
    return CALL_ERR_VIDEO_NOT_SUPPORTED;
}

int32_t IMSCall::SetPausePicture(std::string &path)
{
    return CALL_ERR_VIDEO_NOT_SUPPORTED;
}

int32_t IMSCall::SetDeviceDirection(int32_t rotation)
{
    return CALL_ERR_VIDEO_NOT_SUPPORTED;
}

int32_t IMSCall::CancelCallUpgrade()
{
    return CALL_ERR_VIDEO_NOT_SUPPORTED;
}

int32_t IMSCall::RequestCameraCapabilities()
{
    return CALL_ERR_VIDEO_NOT_SUPPORTED;
}

bool IMSCall::IsSupportVideoCall()
{
    return false;
}

bool IMSCall::IsVoiceModifyToVideo()
{
    return false;
}
} // namespace Telephony
} // namespace OHOS
