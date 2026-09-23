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

#ifndef CALL_REQUEST_HANDLER_H
#define CALL_REQUEST_HANDLER_H

// Mini variant of services/call/include/call_request_handler.h. Requests the standard handler submits
// to ffrt are posted to the main loop; a failed post returns TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL
// without side effects. Dialing stays synchronous, as in the standard handler.

#include <memory>

#include "call_request_process.h"
#include "common_type.h"

namespace OHOS {
namespace Telephony {
class CallRequestHandler {
public:
    CallRequestHandler();
    ~CallRequestHandler();
    void Init();
    int32_t DialCall();
    int32_t AnswerCall(int32_t callId, int32_t videoState, bool isRTT = false);
    int32_t RejectCall(int32_t callId, bool isSendSms, std::string &content);
    int32_t HangUpCall(int32_t callId);
    int32_t HoldCall(int32_t callId);
    int32_t UnHoldCall(int32_t callId);
    int32_t SwitchCall(int32_t callId);

private:
    std::shared_ptr<CallRequestProcess> callRequestProcessPtr_;
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_REQUEST_HANDLER_H
