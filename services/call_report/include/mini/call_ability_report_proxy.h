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

#ifndef CALL_ABILITY_REPORT_PROXY_H
#define CALL_ABILITY_REPORT_PROXY_H

// Mini variant of services/call_report/include/call_ability_report_proxy.h (design D6/D7): one
// application callback, changed on the main loop only. Each report is copied by value and posted to
// the main loop as its own task, so the callback runs with no lock held and may call back into the
// call manager; a failing callback never affects the state machine.

#include "singleton.h"

#include "call_manager_inner_type.h"
#include "call_state_listener_base.h"
#include "i_call_ability_callback.h"

namespace OHOS {
namespace Telephony {
class CallAbilityReportProxy : public CallStateListenerBase,
                               public std::enable_shared_from_this<CallAbilityReportProxy> {
    DECLARE_DELAYED_SINGLETON(CallAbilityReportProxy)

public:
    int32_t RegisterCallBack(sptr<ICallAbilityCallback> callback, const std::string &bundleInfo);
    int32_t UnRegisterCallBack(const std::string &bundleInfo);
    // The standard death-recipient path: drops exactly this callback, without a permission check.
    int32_t UnRegisterCallBack(const sptr<ICallAbilityCallback> &callback);
    void CallStateUpdated(sptr<CallBase> &callObjectPtr, TelCallState priorState, TelCallState nextState) override;
    void CallEventUpdated(CallEventInfo &info) override;
    void CallDestroyed(const DisconnectedDetails &details) override;
    int32_t ReportCallStateInfo(const CallAttributeInfo &info);
    int32_t ReportCallStateInfo(const CallAttributeInfo &info, std::string bundleInfo);

private:
    int32_t ReportCallEvent(const CallEventInfo &info);

    sptr<ICallAbilityCallback> callbackPtr_;
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_ABILITY_REPORT_PROXY_H
