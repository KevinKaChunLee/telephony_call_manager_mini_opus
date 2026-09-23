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

// Only the in-scope reports are converted; the local ringback audio flag is cut with the ringback path.

#include "call_status_callback.h"

#include <securec.h>

#include "call_ability_report_proxy.h"
#include "call_manager_errors.h"
#include "report_call_info_handler.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
int32_t CallStatusCallback::UpdateCallReportInfo(const CallReportInfo &info)
{
    CallDetailInfo detailInfo;
    detailInfo.callType = info.callType;
    detailInfo.accountId = info.accountId;
    detailInfo.index = info.index;
    detailInfo.state = info.state;
    detailInfo.callMode = info.callMode;
    detailInfo.voiceDomain = info.voiceDomain;
    detailInfo.mpty = info.mpty;
    detailInfo.phoneIndex = info.phoneIndex;
    (void)memcpy_s(detailInfo.phoneNum, kMaxNumberLen, info.accountNum, kMaxNumberLen);
    (void)memset_s(detailInfo.bundleName, kMaxBundleNameLen, 0, kMaxBundleNameLen);
    int32_t ret = DelayedSingleton<ReportCallInfoHandler>::GetInstance()->UpdateCallReportInfo(detailInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallReportInfo failed! errCode:%{public}d", ret);
    }
    return ret;
}

int32_t CallStatusCallback::UpdateCallsReportInfo(const CallsReportInfo &info)
{
    CallDetailsInfo detailsInfo;
    CallDetailInfo detailInfo;
    detailInfo.index = 0;
    detailInfo.state = TelCallState::CALL_STATUS_UNKNOWN;
    for (const CallReportInfo &call : info.callVec) {
        detailInfo.callType = call.callType;
        detailInfo.accountId = call.accountId;
        detailInfo.index = call.index;
        detailInfo.state = call.state;
        detailInfo.callMode = call.callMode;
        detailInfo.voiceDomain = call.voiceDomain;
        detailInfo.mpty = call.mpty;
        detailInfo.crsType = call.crsType;
        detailInfo.originalCallType = call.originalCallType;
        (void)memcpy_s(detailInfo.phoneNum, kMaxNumberLen, call.accountNum, kMaxNumberLen);
        (void)memset_s(detailInfo.bundleName, kMaxBundleNameLen, 0, kMaxBundleNameLen);
        detailInfo.name = call.name;
        detailInfo.namePresentation = call.namePresentation;
        detailInfo.reason = call.reason;
        detailInfo.message = call.message;
        detailInfo.newCallUseBox = call.newCallUseBox;
        detailInfo.imsDomain = call.imsDomain;
        detailsInfo.callVec.push_back(detailInfo);
    }
    detailsInfo.slotId = info.slotId;
    (void)memset_s(detailsInfo.bundleName, kMaxBundleNameLen, 0, kMaxBundleNameLen);
    int32_t ret = DelayedSingleton<ReportCallInfoHandler>::GetInstance()->UpdateCallsReportInfo(detailsInfo);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateCallsReportInfo failed! errCode:%{public}d", ret);
    }
    return ret;
}

int32_t CallStatusCallback::UpdateDisconnectedCause(const DisconnectedDetails &details)
{
    int32_t ret = DelayedSingleton<ReportCallInfoHandler>::GetInstance()->UpdateDisconnectedCause(details);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateDisconnectedCause failed! errCode:%{public}d", ret);
    }
    return ret;
}

int32_t CallStatusCallback::UpdateEventResultInfo(const CellularCallEventInfo &info)
{
    int32_t ret = DelayedSingleton<ReportCallInfoHandler>::GetInstance()->UpdateEventResultInfo(info);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("UpdateEventResultInfo failed! errCode:%{public}d", ret);
    }
    return ret;
}

int32_t CallStatusCallback::UpdateRBTPlayInfo(const RBTPlayInfo info)
{
    if (info == RBTPlayInfo::LOCAL_ALERTING) {
        CallEventInfo eventInfo;
        eventInfo.eventId = CallAbilityEventId::EVENT_LOCAL_ALERTING;
        DelayedSingleton<CallAbilityReportProxy>::GetInstance()->CallEventUpdated(eventInfo);
    }
    return TELEPHONY_SUCCESS;
}
} // namespace Telephony
} // namespace OHOS
