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

#ifndef CELLULAR_CALL_CONNECTION_H
#define CELLULAR_CALL_CONNECTION_H

// Mini variant of services/telephony_interaction/include/cellular_call_connection.h. The standard
// class holds a cellular_call SA proxy and re-registers its callback when the SA comes back; here the
// stack is the in-image CellularCallLiteInterface from CellularCallLiteRegistry. In-scope commands are
// forwarded with the stack's error code unchanged and fail with TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL
// while no stack is registered; out-of-scope ones return CALL_ERR_FUNCTION_NOT_SUPPORTED.
// All methods run on the main loop.

#include "singleton.h"

#include "call_manager_info.h"
#include "call_manager_inner_type.h"
#include "i_call_status_callback.h"

namespace OHOS {
namespace Telephony {
class CellularCallConnection {
    DECLARE_DELAYED_SINGLETON(CellularCallConnection)

public:
    // Subscribes to stack readiness; the uplink is registered on the main loop once the stack is there.
    void Init(int32_t systemAbilityId);
    // Unregisters the uplink before dropping it.
    void UnInit();
    bool IsCallbackRegistered() const;

    int32_t Dial(const CellularCallInfo &callInfo);
    int32_t HangUp(const CellularCallInfo &callInfo, CallSupplementType type);
    int32_t Reject(const CellularCallInfo &callInfo);
    int32_t Answer(const CellularCallInfo &callInfo);
    int32_t HoldCall(const CellularCallInfo &callInfo);
    int32_t UnHoldCall(const CellularCallInfo &callInfo);
    int32_t SwitchCall(const CellularCallInfo &callInfo);
    int32_t IsEmergencyPhoneNumber(const std::string &phoneNum, int32_t slotId, bool &enabled);
    int32_t HangUpAllConnection();

    int32_t CombineConference(const CellularCallInfo &callInfo);
    int32_t SeparateConference(const CellularCallInfo &callInfo);
    int32_t KickOutFromConference(const CellularCallInfo &callInfo);
    int32_t StartDtmf(char cDTMFCode, const CellularCallInfo &callInfo);
    int32_t StopDtmf(const CellularCallInfo &callInfo);
    int32_t PostDialProceed(const CellularCallInfo &callInfo, const bool proceed);
    int32_t SetMute(int32_t mute, int32_t slotId);
    int32_t GetVideoCallWaiting(int32_t slotId, bool &enabled);
    int32_t GetCarrierVtConfig(int32_t slotId, bool &enabled);
    bool IsMmiCode(int32_t slotId, std::string &number);

private:
    static void OnStackReady();
    void RegisterCallbackOnLoop();

    bool initialized_ = false;
    sptr<ICallStatusCallback> callback_;
    bool callbackRegistered_ = false;
};
} // namespace Telephony
} // namespace OHOS
#endif // CELLULAR_CALL_CONNECTION_H
