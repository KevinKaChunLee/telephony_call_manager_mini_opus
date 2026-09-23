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

#ifndef IMS_CALL_H
#define IMS_CALL_H

// Mini variant of services/call/include/ims_call.h. The standard class also derives from NetCallBase,
// whose video interface takes Surface and drags in the video state machine; Mini IMS calls are voice
// only, so the class keeps the carrier-call behaviour and the video entry points the ported code and
// callers may reach, all of which return CALL_ERR_VIDEO_NOT_SUPPORTED without touching call state.

#include "carrier_call.h"

namespace OHOS {
namespace Telephony {
class IMSCall : public CarrierCall {
public:
    IMSCall(DialParaInfo &info);
    IMSCall(DialParaInfo &info, AppExecFwk::PacMap &extras);
    ~IMSCall();
    int32_t InitVideoCall();
    int32_t DialingProcess() override;
    int32_t AnswerCall(int32_t videoState, bool isRTT = false) override;
    int32_t RejectCall() override;
    int32_t HangUpCall() override;
    int32_t HoldCall() override;
    int32_t UnHoldCall() override;
    int32_t SwitchCall() override;
    void GetCallAttributeInfo(CallAttributeInfo &info) override;
    int32_t CombineConference() override;
    void HandleCombineConferenceFailEvent() override;
    int32_t SeparateConference() override;
    int32_t KickOutFromConference() override;
    int32_t CanCombineConference() override;
    int32_t CanSeparateConference() override;
    int32_t CanKickOutFromConference() override;
    int32_t LaunchConference() override;
    int32_t ExitConference() override;
    int32_t HoldConference() override;
    int32_t GetMainCallId(int32_t &mainCallId) override;
    int32_t GetSubCallIdList(std::vector<std::u16string> &callIdList) override;
    int32_t GetCallIdListForConference(std::vector<std::u16string> &callIdList) override;
    int32_t IsSupportConferenceable() override;
    int32_t SetMute(int32_t mute, int32_t slotId) override;
    int32_t UpdateImsCallMode(ImsCallMode mode);
    int32_t SendUpdateCallMediaModeRequest(ImsCallMode mode);
    int32_t RecieveUpdateCallMediaModeRequest(CallModeReportInfo &response);
    int32_t SendUpdateCallMediaModeResponse(ImsCallMode mode);
    int32_t ReceiveUpdateCallMediaModeResponse(CallModeReportInfo &response);
    int32_t ControlCamera(std::string &cameraId, int32_t callingUid, int32_t callingPid);
    int32_t SetPausePicture(std::string &path);
    int32_t SetDeviceDirection(int32_t rotation);
    int32_t CancelCallUpgrade();
    int32_t RequestCameraCapabilities();
    bool IsSupportVideoCall();
    bool IsVoiceModifyToVideo();
};
} // namespace Telephony
} // namespace OHOS

#endif // IMS_CALL_H
