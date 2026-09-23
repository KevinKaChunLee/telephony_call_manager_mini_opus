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

#ifndef REPORT_CALL_INFO_HANDLER_H
#define REPORT_CALL_INFO_HANDLER_H

// Mini variant of services/telephony_interaction/include/report_call_info_handler.h. The standard
// handler hops onto reportCallInfoQueue to leave the IPC thread; Mini reports already run on the main
// loop (CallStatusCallbackGuard posts them, dialing reports its own state there), so they are handled
// in place and the handler returns CallStatusManager's result. This is what lets a failed dial clean
// up without waiting for a later task (design D4).

#include <memory>

#include "singleton.h"

#include "call_manager_disconnected_details.h"
#include "call_manager_info.h"
#include "call_status_manager.h"

namespace OHOS {
namespace Telephony {
class ReportCallInfoHandler {
    DECLARE_DELAYED_SINGLETON(ReportCallInfoHandler)

public:
    void Init();
    void UnInit();
    int32_t UpdateCallReportInfo(const CallDetailInfo &info);
    int32_t UpdateCallsReportInfo(CallDetailsInfo &info);
    int32_t UpdateDisconnectedCause(const DisconnectedDetails &details);
    int32_t UpdateEventResultInfo(const CellularCallEventInfo &info);

private:
    std::shared_ptr<CallStatusManager> callStatusManagerPtr_ = nullptr;
};
} // namespace Telephony
} // namespace OHOS
#endif // REPORT_CALL_INFO_HANDLER_H
