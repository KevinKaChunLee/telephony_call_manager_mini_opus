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

#ifndef CALL_POLICY_H
#define CALL_POLICY_H

// Mini variant of services/call/include/call_policy.h with the policies of the in-scope actions. The
// EDM, super-privacy and airplane-mode checks read services the Mini system does not have and count as
// "not enabled"; emergency calls stay exempt from every check exactly where the standard exempts them.

#include <string>
#include <memory>

#include "pac_map.h"
#include "call_object_manager.h"

namespace OHOS {
namespace Telephony {
class CallPolicy : public CallObjectManager {
public:
    CallPolicy();
    ~CallPolicy();

    int32_t DialPolicy(std::u16string &number, AppExecFwk::PacMap &extras, bool isEcc);
    int32_t AnswerCallPolicy(int32_t callId, int32_t videoState);
    int32_t RejectCallPolicy(int32_t callId);
    int32_t HoldCallPolicy(int32_t callId);
    int32_t UnHoldCallPolicy(int32_t callId);
    int32_t HangUpPolicy(int32_t callId);
    int32_t SwitchCallPolicy(int32_t callId);
    int32_t IsValidSlotId(int32_t slotId);
    bool IsSupportVideoCall(AppExecFwk::PacMap &extras);
    int32_t CanDialMulityCall(AppExecFwk::PacMap &extras, bool isEcc);
    int32_t IsValidCallType(CallType callType);
    int32_t IsVoiceCallValid(VideoStateType videoState);
    int32_t HasNormalCall(bool isEcc, int32_t slotId, CallType callType);
    int32_t GetAirplaneMode(bool &isAirplaneModeOn);
    int32_t SuperPrivacyMode(std::u16string &number, AppExecFwk::PacMap &extras, bool isEcc);
    bool IsDialingEnable(const std::string &phoneNum);
    bool IsIncomingEnable(const std::string &phoneNum);

private:
    int32_t CheckDialType(DialType dialType);
    int32_t CheckMdmPolicy(const std::string &phoneNum);
    int32_t SelectAccountIdForCarrier(int32_t accountId, DialType dialType, AppExecFwk::PacMap &extras);
    int32_t ValidateCallType(CallType callType);
    int32_t ValidateDialScene(DialScene dialScene);
    int32_t ValidateVideoState(VideoStateType videoState);
    int32_t CheckCallLimit(bool isEcc, VideoStateType videoState);

private:
    uint16_t onlyTwoCall_ = 2;
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_POLICY_H
