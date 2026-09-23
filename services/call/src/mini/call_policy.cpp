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

// Dial and call types the Mini build cuts (OTT, Bluetooth, satellite) fail the policy with
// CALL_ERR_FUNCTION_NOT_SUPPORTED, so they never create a call object or reach the stack. HasNormalCall
// keeps the SIM and network checks; the CT-card IMS rule is cut with IMS registration.

#include "call_policy.h"

#include <string_ex.h>

#include "call_manager_errors.h"
#include "call_number_utils.h"
#include "cellular_call_connection.h"
#include "core_service_client.h"
#include "telephony_errors.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
CallPolicy::CallPolicy() {}

CallPolicy::~CallPolicy() {}

int32_t CallPolicy::DialPolicy(std::u16string &number, AppExecFwk::PacMap &extras, bool isEcc)
{
    DialType dialType = (DialType)extras.GetIntValue("dialType");
    int32_t ret = CheckDialType(dialType);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    std::string phoneNum = Str16ToStr8(number);
    ret = CheckMdmPolicy(phoneNum);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    int32_t accountId = extras.GetIntValue("accountId");
    ret = SelectAccountIdForCarrier(accountId, dialType, extras);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    CallType callType = (CallType)extras.GetIntValue("callType");
    ret = ValidateCallType(callType);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    DialScene dialScene = (DialScene)extras.GetIntValue("dialScene");
    ret = ValidateDialScene(dialScene);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    VideoStateType videoState = (VideoStateType)extras.GetIntValue("videoState");
    ret = ValidateVideoState(videoState);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    ret = CheckCallLimit(isEcc, videoState);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    return SuperPrivacyMode(number, extras, isEcc);
}

int32_t CallPolicy::CheckDialType(DialType dialType)
{
    if (dialType == DialType::DIAL_CARRIER_TYPE || dialType == DialType::DIAL_VOICE_MAIL_TYPE) {
        return TELEPHONY_SUCCESS;
    }
    if (dialType == DialType::DIAL_OTT_TYPE || dialType == DialType::DIAL_BLUETOOTH_TYPE) {
        TELEPHONY_LOGE("dial type %{public}d is not supported on the mini system", dialType);
        return CALL_ERR_FUNCTION_NOT_SUPPORTED;
    }
    TELEPHONY_LOGE("dial type invalid! dialType=%{public}d", dialType);
    return TELEPHONY_ERR_ARGUMENT_INVALID;
}

int32_t CallPolicy::CheckMdmPolicy(const std::string &phoneNum)
{
    if (!IsDialingEnable(phoneNum)) {
        TELEPHONY_LOGE("MDM policy is disabled!");
        return TELEPHONY_ERR_POLICY_DISABLED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::SelectAccountIdForCarrier(int32_t accountId, DialType dialType, AppExecFwk::PacMap &extras)
{
    if (dialType == DialType::DIAL_CARRIER_TYPE) {
        if (!DelayedSingleton<CallNumberUtils>::GetInstance()->SelectAccountId(accountId, extras)) {
            extras.PutIntValue("accountId", 0);
            TELEPHONY_LOGE("invalid accountId, select accountId to 0");
        }
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::ValidateCallType(CallType callType)
{
    int32_t ret = IsValidCallType(callType);
    if (ret == CALL_ERR_FUNCTION_NOT_SUPPORTED) {
        return ret;
    }
    if (ret != TELEPHONY_SUCCESS) {
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::ValidateDialScene(DialScene dialScene)
{
    if (dialScene != DialScene::CALL_NORMAL && dialScene != DialScene::CALL_PRIVILEGED &&
        dialScene != DialScene::CALL_EMERGENCY) {
        TELEPHONY_LOGE("invalid dial scene!");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::ValidateVideoState(VideoStateType videoState)
{
    if (videoState != VideoStateType::TYPE_VOICE && videoState != VideoStateType::TYPE_VIDEO) {
        TELEPHONY_LOGE("invalid video state!");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::CheckCallLimit(bool isEcc, VideoStateType videoState)
{
    if (!isEcc) {
        bool hasEccCall = false;
        if (HasEmergencyCall(hasEccCall) == TELEPHONY_ERR_SUCCESS && hasEccCall) {
            TELEPHONY_LOGE("during emergency call, calling is prohibited");
            return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
        }
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::SuperPrivacyMode(std::u16string &number, AppExecFwk::PacMap &extras, bool isEcc)
{
    CallType callType = (CallType)extras.GetIntValue("callType");
    int32_t slotId = extras.GetIntValue("accountId");
    return HasNormalCall(isEcc, slotId, callType);
}

int32_t CallPolicy::HasNormalCall(bool isEcc, int32_t slotId, CallType callType)
{
    if (isEcc) {
        return TELEPHONY_SUCCESS;
    }
    bool hasSimCard = false;
    DelayedRefSingleton<CoreServiceClient>::GetInstance().HasSimCard(slotId, hasSimCard);
    if (!hasSimCard) {
        TELEPHONY_LOGE("Call failed due to no sim card");
        return TELEPHONY_ERR_NO_SIM_CARD;
    }
    bool isAirplaneModeOn = false;
    int32_t ret = GetAirplaneMode(isAirplaneModeOn);
    if (ret == TELEPHONY_SUCCESS && isAirplaneModeOn) {
        TELEPHONY_LOGE("Call failed due to isAirplaneModeOn is true");
        return TELEPHONY_ERR_AIRPLANE_MODE_ON;
    }
    bool inService = false;
    DelayedRefSingleton<CoreServiceClient>::GetInstance().IsNetworkInService(slotId, inService);
    if (!inService) {
        TELEPHONY_LOGE("Call failed due to no service");
        return TELEPHONY_ERR_NETWORK_NOT_IN_SERVICE;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::GetAirplaneMode(bool &isAirplaneModeOn)
{
    isAirplaneModeOn = false;
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t CallPolicy::IsVoiceCallValid(VideoStateType videoState)
{
    if (videoState == VideoStateType::TYPE_VOICE) {
        sptr<CallBase> ringCall = GetOneCallObject(CallRunningState::CALL_RUNNING_STATE_RINGING);
        if (ringCall != nullptr && ringCall->GetVideoStateType() == VideoStateType::TYPE_VOICE &&
            ringCall->GetCallType() != CallType::TYPE_VOIP) {
            TELEPHONY_LOGE("already has new call ringing!");
            return CALL_ERR_CALL_COUNTS_EXCEED_LIMIT;
        }
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::IsValidCallType(CallType callType)
{
    if (callType == CallType::TYPE_CS || callType == CallType::TYPE_IMS) {
        return TELEPHONY_SUCCESS;
    }
    if (callType == CallType::TYPE_OTT || callType == CallType::TYPE_SATELLITE ||
        callType == CallType::TYPE_BLUETOOTH) {
        TELEPHONY_LOGE("call type %{public}d is not supported on the mini system", callType);
        return CALL_ERR_FUNCTION_NOT_SUPPORTED;
    }
    TELEPHONY_LOGE("invalid call type!");
    return CALL_ERR_UNKNOW_CALL_TYPE;
}

int32_t CallPolicy::CanDialMulityCall(AppExecFwk::PacMap &extras, bool isEcc)
{
    VideoStateType videoState = (VideoStateType)extras.GetIntValue("videoState");
    int32_t slotId = extras.GetIntValue("accountId");
    bool enabled = false;
    DelayedSingleton<CellularCallConnection>::GetInstance()->GetVideoCallWaiting(slotId, enabled);
    if (!enabled) {
        if (videoState == VideoStateType::TYPE_VIDEO && HasCellularCallExist()) {
            TELEPHONY_LOGE("can not dial video call when any call exist!");
            return CALL_ERR_DIAL_IS_BUSY;
        }
        if (!isEcc && videoState == VideoStateType::TYPE_VOICE && HasVideoCall()) {
            TELEPHONY_LOGE("can not dial video call when any call exist!");
            return CALL_ERR_DIAL_IS_BUSY;
        }
    }
    return TELEPHONY_SUCCESS;
}

bool CallPolicy::IsSupportVideoCall(AppExecFwk::PacMap &extras)
{
    bool vtEnabled = false;
    DelayedSingleton<CallNumberUtils>::GetInstance()->IsCarrierVtConfig(extras.GetIntValue("accountId"), vtEnabled);
    if (!vtEnabled) {
        return false;
    }
    DialScene dialScene = (DialScene)extras.GetIntValue("dialScene");
    return dialScene == DialScene::CALL_NORMAL;
}

int32_t CallPolicy::AnswerCallPolicy(int32_t callId, int32_t videoState)
{
    if (videoState != static_cast<int32_t>(VideoStateType::TYPE_VOICE) &&
        videoState != static_cast<int32_t>(VideoStateType::TYPE_VIDEO)) {
        TELEPHONY_LOGE("videoState is invalid!");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (!IsCallExist(callId)) {
        TELEPHONY_LOGE("callId is invalid, callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    TelCallState state = GetCallState(callId);
    if (state != TelCallState::CALL_STATUS_INCOMING && state != TelCallState::CALL_STATUS_WAITING) {
        TELEPHONY_LOGE("current call state is:%{public}d, accept call not allowed", state);
        return CALL_ERR_ILLEGAL_CALL_OPERATION;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::RejectCallPolicy(int32_t callId)
{
    if (!IsCallExist(callId)) {
        TELEPHONY_LOGE("callId is invalid, callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    TelCallState state = GetCallState(callId);
    if (state != TelCallState::CALL_STATUS_INCOMING && state != TelCallState::CALL_STATUS_WAITING) {
        TELEPHONY_LOGE("current call state is:%{public}d, reject call not allowed", state);
        return CALL_ERR_ILLEGAL_CALL_OPERATION;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::HoldCallPolicy(int32_t callId)
{
    sptr<CallBase> call = GetOneCallObject(callId);
    if (call == nullptr) {
        TELEPHONY_LOGE("GetOneCallObject failed, this callId is invalid! callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (call->GetCallRunningState() != CallRunningState::CALL_RUNNING_STATE_ACTIVE) {
        TELEPHONY_LOGE("this call is not activated! callId:%{public}d", callId);
        return CALL_ERR_CALL_IS_NOT_ACTIVATED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::UnHoldCallPolicy(int32_t callId)
{
    sptr<CallBase> call = GetOneCallObject(callId);
    if (call == nullptr) {
        TELEPHONY_LOGE("GetOneCallObject failed, this callId is invalid! callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (call->GetCallRunningState() != CallRunningState::CALL_RUNNING_STATE_HOLD) {
        TELEPHONY_LOGE("this call is not on holding state! callId:%{public}d", callId);
        return CALL_ERR_CALL_IS_NOT_ON_HOLDING;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::HangUpPolicy(int32_t callId)
{
    if (!IsCallExist(callId)) {
        TELEPHONY_LOGE("callId is invalid, callId:%{public}d", callId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    TelCallState state = GetCallState(callId);
    if (state == TelCallState::CALL_STATUS_IDLE || state == TelCallState::CALL_STATUS_DISCONNECTING ||
        state == TelCallState::CALL_STATUS_DISCONNECTED) {
        TELEPHONY_LOGE("current call state is:%{public}d, hang up call not allowed", state);
        return CALL_ERR_ILLEGAL_CALL_OPERATION;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::SwitchCallPolicy(int32_t callId)
{
    std::list<int32_t> callIdList;
    if (!IsCallExist(callId)) {
        TELEPHONY_LOGE("callId is invalid");
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    GetCarrierCallList(callIdList);
    if (callIdList.size() < onlyTwoCall_) {
        return CALL_ERR_PHONE_CALLS_TOO_FEW;
    }
    if (GetCallState(callId) != TelCallState::CALL_STATUS_HOLDING ||
        IsCallExist(TelCallState::CALL_STATUS_DIALING) || IsCallExist(TelCallState::CALL_STATUS_ALERTING)) {
        TELEPHONY_LOGE("the call is not on hold, callId:%{public}d", callId);
        return CALL_ERR_ILLEGAL_CALL_OPERATION;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallPolicy::IsValidSlotId(int32_t slotId)
{
    bool result = DelayedSingleton<CallNumberUtils>::GetInstance()->IsValidSlotId(slotId);
    if (!result) {
        TELEPHONY_LOGE("invalid slotId!");
        return CALL_ERR_INVALID_SLOT_ID;
    }
    return TELEPHONY_SUCCESS;
}

bool CallPolicy::IsDialingEnable(const std::string &phoneNum)
{
    return true;
}

bool CallPolicy::IsIncomingEnable(const std::string &phoneNum)
{
    return true;
}
} // namespace Telephony
} // namespace OHOS
