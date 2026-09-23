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

#ifndef CALL_STATUS_CALLBACK_H
#define CALL_STATUS_CALLBACK_H

// Mini variant of services/telephony_interaction/include/call_status_callback.h. The standard class
// is the stub the stack calls over IPC; here CallStatusCallbackGuard receives the stack's reports,
// validates and copies them and calls these methods on the main loop. Only the in-scope reports are
// kept; their conversion to CallDetailInfo is ported from the standard implementation.

#include "tel_mini_std_includes.h"

#include "call_manager_info.h"
#include "call_manager_inner_type.h"
#include "call_manager_disconnected_details.h"

namespace OHOS {
namespace Telephony {
class CallStatusCallback {
public:
    CallStatusCallback() = default;
    ~CallStatusCallback() = default;
    int32_t UpdateCallReportInfo(const CallReportInfo &info);
    int32_t UpdateCallsReportInfo(const CallsReportInfo &info);
    int32_t UpdateDisconnectedCause(const DisconnectedDetails &details);
    int32_t UpdateEventResultInfo(const CellularCallEventInfo &info);
    int32_t UpdateRBTPlayInfo(const RBTPlayInfo info);
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_STATUS_CALLBACK_H
