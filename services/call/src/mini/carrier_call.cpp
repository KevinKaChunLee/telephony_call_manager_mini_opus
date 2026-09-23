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

#include "carrier_call.h"

#include <securec.h>

#include "call_manager_errors.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
CarrierCall::CarrierCall(DialParaInfo &info)
    : CallBase(info), dialScene_(DialScene::CALL_NORMAL), slotId_(info.accountId), index_(info.index),
    isEcc_(info.isEcc), cellularCallConnectionPtr_(DelayedSingleton<CellularCallConnection>::GetInstance())
{}

CarrierCall::CarrierCall(DialParaInfo &info, AppExecFwk::PacMap &extras)
    : CallBase(info, extras), dialScene_((DialScene)extras.GetIntValue("dialScene")), slotId_(info.accountId),
    index_(info.index), isEcc_(info.isEcc),
    cellularCallConnectionPtr_(DelayedSingleton<CellularCallConnection>::GetInstance())
{}

CarrierCall::~CarrierCall() {}

int32_t CarrierCall::CarrierDialingProcess()
{
    int32_t ret = DialCallBase();
    if (ret != TELEPHONY_SUCCESS) {
        CarrierHangUpCall();
    }
    return ret;
}

int32_t CarrierCall::CarrierAnswerCall(int32_t videoState, bool isRTT)
{
    CellularCallInfo callInfo;
    int32_t ret = AnswerCallBase();
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("answer call failed!");
        return CALL_ERR_ANSWER_FAILED;
    }
    ret = PackCellularCallInfo(callInfo);
    callInfo.videoState = videoState;
    callInfo.isRTT = isRTT;
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    ret = cellularCallConnectionPtr_->Answer(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("answer call failed!");
        return CALL_ERR_ANSWER_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CarrierCall::CarrierRejectCall()
{
    CellularCallInfo callInfo;
    int32_t ret = RejectCallBase();
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    ret = PackCellularCallInfo(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    ret = cellularCallConnectionPtr_->HangUp(callInfo, CallSupplementType::TYPE_DEFAULT);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("Reject failed!");
        return CALL_ERR_REJECT_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CarrierCall::CarrierHangUpCall()
{
    int32_t ret = CALL_ERR_HANGUP_FAILED;
    CellularCallInfo callInfo;
    ret = PackCellularCallInfo(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    uint64_t policyFlag = GetPolicyFlag();
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    if (policyFlag & POLICY_FLAG_HANG_UP_ACTIVE) {
        ret = cellularCallConnectionPtr_->HangUp(callInfo, CallSupplementType::TYPE_HANG_UP_ACTIVE);
    } else if (policyFlag & POLICY_FLAG_HANG_UP_HOLD_WAIT) {
        ret = cellularCallConnectionPtr_->HangUp(callInfo, CallSupplementType::TYPE_HANG_UP_HOLD_WAIT);
    } else {
        ret = cellularCallConnectionPtr_->HangUp(callInfo, CallSupplementType::TYPE_DEFAULT);
    }
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("End failed!");
        return CALL_ERR_HANGUP_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CarrierCall::CarrierHoldCall()
{
    CellularCallInfo callInfo;
    int32_t ret = PackCellularCallInfo(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    ret = cellularCallConnectionPtr_->HoldCall(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("Hold failed!");
        return CALL_ERR_HOLD_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CarrierCall::CarrierUnHoldCall()
{
    CellularCallInfo callInfo;
    int32_t ret = PackCellularCallInfo(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    ret = cellularCallConnectionPtr_->UnHoldCall(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UnHold failed!");
        return CALL_ERR_UNHOLD_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CarrierCall::CarrierSetMute(int32_t mute, int32_t slotId)
{
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return cellularCallConnectionPtr_->SetMute(mute, slotId);
}

void CarrierCall::GetCallAttributeCarrierInfo(CallAttributeInfo &info)
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    info.isEcc = isEcc_;
    info.accountId = slotId_;
    info.index = index_;
    GetCallAttributeBaseInfo(info);
}

bool CarrierCall::GetEmergencyState()
{
    std::lock_guard<ffrt::mutex> lock(mutex_);
    return isEcc_;
}

int32_t CarrierCall::CarrierSwitchCall()
{
    CellularCallInfo callInfo;
    int32_t ret = PackCellularCallInfo(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    ret = cellularCallConnectionPtr_->SwitchCall(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("SwitchCall failed!");
        return CALL_ERR_UNHOLD_FAILED;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CarrierCall::GetSlotId()
{
    return slotId_;
}

int32_t CarrierCall::CarrierCombineConference()
{
    CellularCallInfo callInfo;
    int32_t ret = PackCellularCallInfo(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return cellularCallConnectionPtr_->CombineConference(callInfo);
}

int32_t CarrierCall::CarrierSeparateConference()
{
    CellularCallInfo callInfo;
    int32_t ret = PackCellularCallInfo(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return cellularCallConnectionPtr_->SeparateConference(callInfo);
}

int32_t CarrierCall::CarrierKickOutFromConference()
{
    CellularCallInfo callInfo;
    int32_t ret = PackCellularCallInfo(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return cellularCallConnectionPtr_->KickOutFromConference(callInfo);
}

int32_t CarrierCall::IsSupportConferenceable()
{
    // emergency call not allowed to join conference
    return GetEmergencyState() != true ? TELEPHONY_SUCCESS : CALL_ERR_EMERGENCY_UNSUPPORT_CONFERENCEABLE;
}

int32_t CarrierCall::PackCellularCallInfo(CellularCallInfo &callInfo)
{
    callInfo.callId = callId_;
    callInfo.callType = callType_;
    callInfo.videoState = static_cast<int32_t>(videoState_);
    callInfo.index = index_;
    callInfo.slotId = slotId_;
    callInfo.accountId = slotId_;
    if (memset_s(callInfo.phoneNum, kMaxNumberLen, 0, kMaxNumberLen) != EOK) {
        TELEPHONY_LOGW("memset_s failed!");
        return TELEPHONY_ERR_MEMSET_FAIL;
    }
    if (accountNumber_.length() > static_cast<size_t>(kMaxNumberLen)) {
        TELEPHONY_LOGE("Number out of limit!");
        return CALL_ERR_NUMBER_OUT_OF_RANGE;
    }
    if (memcpy_s(callInfo.phoneNum, kMaxNumberLen, accountNumber_.c_str(), accountNumber_.length()) != EOK) {
        TELEPHONY_LOGW("memcpy_s failed!");
        return TELEPHONY_ERR_MEMCPY_FAIL;
    }
    return TELEPHONY_SUCCESS;
}

int32_t CarrierCall::StartDtmf(char str)
{
    CellularCallInfo callInfo;
    int32_t ret = PackCellularCallInfo(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return cellularCallConnectionPtr_->StartDtmf(str, callInfo);
}

int32_t CarrierCall::StopDtmf()
{
    CellularCallInfo callInfo;
    int32_t ret = PackCellularCallInfo(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return cellularCallConnectionPtr_->StopDtmf(callInfo);
}

int32_t CarrierCall::PostDialProceed(bool proceed)
{
    CellularCallInfo callInfo;
    int32_t ret = PackCellularCallInfo(callInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGW("PackCellularCallInfo failed!");
    }
    if (cellularCallConnectionPtr_ == nullptr) {
        TELEPHONY_LOGE("cellularCallConnectionPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return cellularCallConnectionPtr_->PostDialProceed(callInfo, proceed);
}

void CarrierCall::SetSlotId(int32_t slotId)
{
    slotId_ = slotId;
}

void CarrierCall::SetCallIndex(int32_t index)
{
    index_ = index;
}

int32_t CarrierCall::GetCallIndex()
{
    return index_;
}
} // namespace Telephony
} // namespace OHOS
