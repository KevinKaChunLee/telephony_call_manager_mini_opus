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

#ifndef CALL_REQUEST_EVENT_HANDLER_HELPER
#define CALL_REQUEST_EVENT_HANDLER_HELPER

// Mini variant of services/call/include/call_request_event_handler_helper.h: the dialing flag's
// 3-second restore task runs as a delayed task on the main loop instead of an AppExecFwk event runner.

#include "singleton.h"
#include "los_event_handler.h"

namespace OHOS {
namespace Telephony {
class CallRequestEventHandlerHelper {
    DECLARE_DELAYED_SINGLETON(CallRequestEventHandlerHelper)

public:
    int32_t SetDialingCallProcessing();
    void RemoveEventHandlerTask();
    void RestoreDialingFlag(bool isDialingCallProcessing);
    bool IsDialingCallProcessing();
    void SetPendingMo(bool pendingMo, int32_t callId);
    bool HasPendingMo(int32_t callId);
    void SetPendingHangup(bool pendingHangup, int32_t callId);
    bool HasPendingHangup(int32_t callId);
    bool IsPendingHangup();
    int32_t GetPendingHangupCallId();

private:
    TelMiniTaskId restoreTaskId_ = TEL_MINI_INVALID_TASK_ID;
    bool isDialingCallProcessing_ = false;
    bool pendingMo_ = false;
    bool pendingHangup_ = false;
    int32_t pendingMoCallId_ = -1;
    int32_t pendingHangupCallId_ = -1;
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_REQUEST_EVENT_HANDLER_HELPER
