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

#ifndef CALL_STATUS_CALLBACK_GUARD_H
#define CALL_STATUS_CALLBACK_GUARD_H

// The uplink the Mini call manager hands to cellular_call lite (design D8). It takes over the input
// checks CallStatusCallbackStub performs on parcels: slot id, enum ranges, NUL-terminated numbers and
// the batch size. Accepted reports are copied by value and posted to the main loop, where
// CallStatusCallback handles them; nothing here touches call state. Rejected reports return
// TELEPHONY_ERR_ARGUMENT_INVALID and leave no trace but a log line without the number.
// Reports the Mini call manager does not handle return CALL_ERR_FUNCTION_NOT_SUPPORTED.

#include "i_call_status_callback.h"

namespace OHOS {
namespace Telephony {
class CallStatusCallbackGuard : public ICallStatusCallback {
public:
    CallStatusCallbackGuard() = default;
    ~CallStatusCallbackGuard() override = default;

    int32_t UpdateCallReportInfo(const CallReportInfo &info) override;
    int32_t UpdateCallsReportInfo(const CallsReportInfo &info) override;
    int32_t UpdateDisconnectedCause(const DisconnectedDetails &details) override;
    int32_t UpdateEventResultInfo(const CellularCallEventInfo &info) override;
    int32_t UpdateRBTPlayInfo(const RBTPlayInfo info) override;

    int32_t ReportCallProcedureEvents(const std::string &callId, const std::string &procedureJsonStr) override;
    int32_t UpdateGetWaitingResult(const CallWaitResponse &callWaitResponse) override;
    int32_t UpdateSetWaitingResult(const int32_t result) override;
    int32_t UpdateGetRestrictionResult(const CallRestrictionResponse &callRestrictionResult) override;
    int32_t UpdateSetRestrictionResult(int32_t result) override;
    int32_t UpdateSetRestrictionPasswordResult(int32_t result) override;
    int32_t UpdateGetTransferResult(const CallTransferResponse &callTransferResponse) override;
    int32_t UpdateSetTransferResult(const int32_t result) override;
    int32_t UpdateGetCallClipResult(const ClipResponse &clipResponse) override;
    int32_t UpdateGetCallClirResult(const ClirResponse &clirResponse) override;
    int32_t UpdateSetCallClirResult(const int32_t result) override;
    int32_t StartRttResult(const int32_t result) override;
    int32_t StopRttResult(const int32_t result) override;
    int32_t GetImsConfigResult(const GetImsConfigResponse &response) override;
    int32_t SetImsConfigResult(const int32_t result) override;
    int32_t GetImsFeatureValueResult(const GetImsFeatureValueResponse &response) override;
    int32_t SetImsFeatureValueResult(const int32_t result) override;
    int32_t ReceiveUpdateCallMediaModeRequest(const CallModeReportInfo &response) override;
    int32_t ReceiveUpdateCallMediaModeResponse(const CallModeReportInfo &response) override;
    int32_t InviteToConferenceResult(const int32_t result) override;
    int32_t StartDtmfResult(const int32_t result) override;
    int32_t StopDtmfResult(const int32_t result) override;
    int32_t SendUssdResult(const int32_t result) override;
    int32_t GetImsCallDataResult(const int32_t result) override;
    int32_t SendMmiCodeResult(const MmiCodeInfo &info) override;
    int32_t CloseUnFinishedUssdResult(const int32_t result) override;
    int32_t ReportPostDialChar(const std::string &c) override;
    int32_t ReportPostDialDelay(const std::string &str) override;
    int32_t HandleCallSessionEventChanged(const CallSessionReportInfo &reportInfo) override;
    int32_t HandlePeerDimensionsChanged(const PeerDimensionsReportInfo &dimensionsDetail) override;
    int32_t HandleCallDataUsageChanged(const int64_t result) override;
    int32_t HandleCameraCapabilitiesChanged(const CameraCapabilitiesReportInfo &cameraCapabilities) override;
    int32_t HandleImsSuppExtChanged(const ImsSuppExtReportInfo &suppExtInfo) override;
    int32_t UpdateVoipEventInfo(const VoipCallEventInfo &info) override;
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_STATUS_CALLBACK_GUARD_H
