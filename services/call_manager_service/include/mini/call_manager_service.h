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

#ifndef CALL_MANAGER_SERVICE_H
#define CALL_MANAGER_SERVICE_H

// Mini variant of services/call_manager_service/include/call_manager_service.h (design D6). Not a
// SystemAbility: the in-scope entries keep the standard system-app and permission decision points
// (through CallPermissionAdapter) and take over the parcel-level duties of CallManagerServiceStub:
// the dial-extras whitelist, the number length check and the caller's bundle name. Every method runs
// on the main loop; IsReady() may be read from any task.

#include <atomic>

#include "singleton.h"
#include "pac_map.h"

#include "call_control_manager.h"
#include "i_call_ability_callback.h"

namespace OHOS {
namespace Telephony {
class CallManagerService {
    DECLARE_DELAYED_SINGLETON(CallManagerService)

public:
    // Idempotent. Steps run in the standard order and are undone in reverse on failure; the service
    // becomes ready only after all of them succeeded.
    int32_t Init();
    void UnInit();
    bool IsReady() const;

    int32_t RegisterCallBack(const sptr<ICallAbilityCallback> &callback);
    int32_t UnRegisterCallBack();
    int32_t ObserverOnCallDetailsChange();
    int32_t DialCall(std::u16string number, AppExecFwk::PacMap &extras);
    int32_t AnswerCall(int32_t callId, int32_t videoState, bool isRTT);
    int32_t RejectCall(int32_t callId, bool rejectWithMessage, std::u16string textMessage);
    int32_t HangUpCall(int32_t callId);
    int32_t GetCallState();
    int32_t HoldCall(int32_t callId);
    int32_t UnHoldCall(int32_t callId);
    int32_t SwitchCall(int32_t callId);
    bool HasCall(const bool isInCludeVoipCall);
    int32_t IsNewCallAllowed(bool &enabled);
    int32_t IsRinging(bool &enabled);
    int32_t IsEmergencyPhoneNumber(std::u16string &number, int32_t slotId, bool &enabled);
    bool EndCall();
    int32_t AnswerCall();
    int32_t RejectCall();
    int32_t HangUpCall();

#ifdef TELEPHONY_MINI_HOST_TEST
    // Makes Init fail at the given step (1-based, 0 disables) to exercise the rollback.
    static void FailInitStepForTest(int32_t step);
#endif

private:
    bool CheckCallerIsSystemApp();
    bool CheckSetTelephonyStatePermission();
    std::string GetBundleInfo();

    std::shared_ptr<CallControlManager> callControlManagerPtr_ = nullptr;
    std::atomic<bool> ready_ { false };
};

// Brings the service up on TelMiniMainLoop() in self-driven mode (host self-tests, products without
// samgr_lite) and takes it down again. Start is idempotent and returns the first failure.
int32_t CallManagerServiceMiniStart();
void CallManagerServiceMiniStop();
} // namespace Telephony
} // namespace OHOS
#endif // CALL_MANAGER_SERVICE_H
