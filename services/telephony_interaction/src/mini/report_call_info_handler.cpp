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

// No OOBE or device-provisioning gate: the Mini system has no setup wizard. Bluetooth and transfer-call
// filtering and the anti-fraud wiring are cut.

#include "report_call_info_handler.h"

#include <securec.h>

#include "call_manager_errors.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
ReportCallInfoHandler::ReportCallInfoHandler() {}

ReportCallInfoHandler::~ReportCallInfoHandler() {}

void ReportCallInfoHandler::Init()
{
    if (callStatusManagerPtr_ != nullptr) {
        return;
    }
    callStatusManagerPtr_ = std::make_shared<CallStatusManager>();
    callStatusManagerPtr_->Init();
}

void ReportCallInfoHandler::UnInit()
{
    callStatusManagerPtr_ = nullptr;
}

int32_t ReportCallInfoHandler::UpdateCallReportInfo(const CallDetailInfo &info)
{
    if (callStatusManagerPtr_ == nullptr) {
        TELEPHONY_LOGE("callStatusManagerPtr_ is null");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    int32_t ret = callStatusManagerPtr_->HandleCallReportInfo(info);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("HandleCallReportInfo failed! ret:%{public}d", ret);
    }
    return ret;
}

int32_t ReportCallInfoHandler::UpdateCallsReportInfo(CallDetailsInfo &info)
{
    if (callStatusManagerPtr_ == nullptr) {
        TELEPHONY_LOGE("callStatusManagerPtr_ is null");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    CallDetailsInfo callDetailsInfo;
    callDetailsInfo.slotId = info.slotId;
    if (memcpy_s(callDetailsInfo.bundleName, kMaxBundleNameLen + 1, info.bundleName, kMaxBundleNameLen + 1) != EOK) {
        TELEPHONY_LOGE("memcpy_s bundleName failed");
        return TELEPHONY_ERR_MEMCPY_FAIL;
    }
    for (const CallDetailInfo &detail : info.callVec) {
        callDetailsInfo.callVec.push_back(detail);
    }
    int32_t ret = callStatusManagerPtr_->HandleCallsReportInfo(callDetailsInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("HandleCallsReportInfo failed! ret:%{public}d", ret);
    }
    return ret;
}

int32_t ReportCallInfoHandler::UpdateDisconnectedCause(const DisconnectedDetails &details)
{
    if (callStatusManagerPtr_ == nullptr) {
        TELEPHONY_LOGE("callStatusManagerPtr_ is null");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    DisconnectedDetails disconnectedDetails = details;
    if (static_cast<RilDisconnectedReason>(details.reason) == RilDisconnectedReason::DISCONNECTED_REASON_NORMAL) {
        disconnectedDetails.reason = DisconnectedReason::NORMAL_CALL_CLEARING;
    }
    return callStatusManagerPtr_->HandleDisconnectedCause(disconnectedDetails);
}

int32_t ReportCallInfoHandler::UpdateEventResultInfo(const CellularCallEventInfo &info)
{
    if (callStatusManagerPtr_ == nullptr) {
        TELEPHONY_LOGE("callStatusManagerPtr_ is null");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
    return callStatusManagerPtr_->HandleEventResultReportInfo(info);
}
} // namespace Telephony
} // namespace OHOS
