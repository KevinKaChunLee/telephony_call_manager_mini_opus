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

#include "call_status_callback_guard.h"

#include <cstring>

#include "call_manager_errors.h"
#include "call_manager_mini_config.h"
#include "call_status_callback.h"
#include "los_event_handler.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
namespace {
bool IsValidSlot(int32_t slotId)
{
    return slotId >= 0 && slotId < MINI_SLOT_COUNT;
}

bool IsReportedState(TelCallState state)
{
    return state >= TelCallState::CALL_STATUS_ACTIVE && state <= TelCallState::CALL_STATUS_DISCONNECTING;
}

bool IsCarrierCallType(CallType type)
{
    return type == CallType::TYPE_CS || type == CallType::TYPE_IMS;
}

bool IsVideoState(VideoStateType mode)
{
    return mode >= VideoStateType::TYPE_VOICE && mode <= VideoStateType::TYPE_VIDEO;
}

bool IsTerminated(const char *text, size_t capacity)
{
    return std::memchr(text, '\0', capacity) != nullptr;
}

bool IsValidReport(const CallReportInfo &info)
{
    if (!IsValidSlot(info.accountId)) {
        TELEPHONY_LOGE("report rejected: slot %{public}d", info.accountId);
        return false;
    }
    if (!IsReportedState(info.state) || !IsCarrierCallType(info.callType) || !IsVideoState(info.callMode)) {
        TELEPHONY_LOGE("report rejected: state %{public}d type %{public}d mode %{public}d",
            static_cast<int32_t>(info.state), static_cast<int32_t>(info.callType), static_cast<int32_t>(info.callMode));
        return false;
    }
    if (!IsTerminated(info.accountNum, sizeof(info.accountNum))) {
        TELEPHONY_LOGE("report rejected: number is not terminated within %{public}d bytes", kMaxNumberLen);
        return false;
    }
    return true;
}

int32_t PostToLoop(TelMiniTask task)
{
    int32_t ret = TelMiniMainLoop().PostAsyncTask(std::move(task));
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("report dropped: main loop post failed %{public}d", ret);
    }
    return ret;
}

int32_t NotSupported(const char *method)
{
    TELEPHONY_LOGW("%{public}s is not handled on the mini system", method);
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}
} // namespace

int32_t CallStatusCallbackGuard::UpdateCallReportInfo(const CallReportInfo &info)
{
    if (!IsValidReport(info)) {
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    return PostToLoop([info]() { CallStatusCallback().UpdateCallReportInfo(info); });
}

int32_t CallStatusCallbackGuard::UpdateCallsReportInfo(const CallsReportInfo &info)
{
    if (info.callVec.size() > MINI_MAX_REPORTED_CALLS) {
        TELEPHONY_LOGE("report rejected: %{public}zu calls in one batch", info.callVec.size());
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    if (!IsValidSlot(info.slotId)) {
        TELEPHONY_LOGE("report rejected: slot %{public}d", info.slotId);
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    for (const CallReportInfo &call : info.callVec) {
        if (!IsValidReport(call) || call.accountId != info.slotId) {
            return TELEPHONY_ERR_ARGUMENT_INVALID;
        }
    }
    return PostToLoop([info]() { CallStatusCallback().UpdateCallsReportInfo(info); });
}

int32_t CallStatusCallbackGuard::UpdateDisconnectedCause(const DisconnectedDetails &details)
{
    return PostToLoop([details]() { CallStatusCallback().UpdateDisconnectedCause(details); });
}

int32_t CallStatusCallbackGuard::UpdateEventResultInfo(const CellularCallEventInfo &info)
{
    if (info.eventType != CellularCallEventType::EVENT_REQUEST_RESULT_TYPE) {
        TELEPHONY_LOGE("event rejected: type %{public}d", static_cast<int32_t>(info.eventType));
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    return PostToLoop([info]() { CallStatusCallback().UpdateEventResultInfo(info); });
}

int32_t CallStatusCallbackGuard::UpdateRBTPlayInfo(const RBTPlayInfo info)
{
    if (info != RBTPlayInfo::NETWORK_ALERTING && info != RBTPlayInfo::LOCAL_ALERTING) {
        return TELEPHONY_ERR_ARGUMENT_INVALID;
    }
    return PostToLoop([info]() { CallStatusCallback().UpdateRBTPlayInfo(info); });
}

int32_t CallStatusCallbackGuard::ReportCallProcedureEvents(const std::string &, const std::string &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::UpdateGetWaitingResult(const CallWaitResponse &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::UpdateSetWaitingResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::UpdateGetRestrictionResult(const CallRestrictionResponse &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::UpdateSetRestrictionResult(int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::UpdateSetRestrictionPasswordResult(int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::UpdateGetTransferResult(const CallTransferResponse &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::UpdateSetTransferResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::UpdateGetCallClipResult(const ClipResponse &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::UpdateGetCallClirResult(const ClirResponse &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::UpdateSetCallClirResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::StartRttResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::StopRttResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::GetImsConfigResult(const GetImsConfigResponse &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::SetImsConfigResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::GetImsFeatureValueResult(const GetImsFeatureValueResponse &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::SetImsFeatureValueResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::ReceiveUpdateCallMediaModeRequest(const CallModeReportInfo &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::ReceiveUpdateCallMediaModeResponse(const CallModeReportInfo &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::InviteToConferenceResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::StartDtmfResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::StopDtmfResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::SendUssdResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::GetImsCallDataResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::SendMmiCodeResult(const MmiCodeInfo &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::CloseUnFinishedUssdResult(const int32_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::ReportPostDialChar(const std::string &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::ReportPostDialDelay(const std::string &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::HandleCallSessionEventChanged(const CallSessionReportInfo &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::HandlePeerDimensionsChanged(const PeerDimensionsReportInfo &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::HandleCallDataUsageChanged(const int64_t)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::HandleCameraCapabilitiesChanged(const CameraCapabilitiesReportInfo &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::HandleImsSuppExtChanged(const ImsSuppExtReportInfo &)
{
    return NotSupported(__func__);
}

int32_t CallStatusCallbackGuard::UpdateVoipEventInfo(const VoipCallEventInfo &)
{
    return NotSupported(__func__);
}
} // namespace Telephony
} // namespace OHOS
