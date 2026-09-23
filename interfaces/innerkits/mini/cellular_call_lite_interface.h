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

#ifndef CELLULAR_CALL_LITE_INTERFACE_H
#define CELLULAR_CALL_LITE_INTERFACE_H

// The in-image protocol stack seen by the Mini call manager: the in-scope subset of
// CellularCallInterface with the same signatures, minus IRemoteBroker and Surface. The cellular_call
// lite component implements it and registers the instance with CellularCallLiteRegistry, from any
// task, before or after the call manager starts. Every method may be called on the call manager's
// main loop and must not block on it. Error codes are passed up unchanged.
//
// RegisterCallManagerCallBack hands over the uplink: the stack reports call state through that
// ICallStatusCallback from its own task and must stop using it once UnRegisterCallManagerCallBack
// has returned.

#include "tel_mini_std_includes.h"

#include "refbase.h"
#include "call_manager_info.h"
#include "call_manager_inner_type.h"
#include "i_call_status_callback.h"

namespace OHOS {
namespace Telephony {
class CellularCallLiteInterface : public virtual RefBase {
public:
    virtual ~CellularCallLiteInterface() = default;

    virtual int32_t Dial(const CellularCallInfo &callInfo) = 0;
    virtual int32_t HangUp(const CellularCallInfo &callInfo, CallSupplementType type) = 0;
    virtual int32_t Answer(const CellularCallInfo &callInfo) = 0;
    virtual int32_t Reject(const CellularCallInfo &callInfo) = 0;
    virtual int32_t HoldCall(const CellularCallInfo &callInfo) = 0;
    virtual int32_t UnHoldCall(const CellularCallInfo &callInfo) = 0;
    virtual int32_t SwitchCall(const CellularCallInfo &callInfo) = 0;
    virtual int32_t IsEmergencyPhoneNumber(int32_t slotId, const std::string &phoneNum, bool &enabled) = 0;
    virtual int32_t HangUpAllConnection() = 0;
    virtual int32_t RegisterCallManagerCallBack(const sptr<ICallStatusCallback> &callback) = 0;
    virtual int32_t UnRegisterCallManagerCallBack() = 0;
};

// Called without any registry lock held, on the task that registered the stack.
using CellularCallLiteReadyListener = void (*)();

class CellularCallLiteRegistry {
public:
    // Replaces any earlier instance; nullptr unregisters. Listeners hear about every non-null
    // registration.
    static void Register(const sptr<CellularCallLiteInterface> &instance);
    static sptr<CellularCallLiteInterface> Get();
    // At most MAX_LISTENERS; returns false when full. A listener added while an instance is already
    // registered is notified at once.
    static bool AddReadyListener(CellularCallLiteReadyListener listener);
    static void RemoveReadyListener(CellularCallLiteReadyListener listener);

    static constexpr uint32_t MAX_LISTENERS = 2;
};
} // namespace Telephony
} // namespace OHOS
#endif // CELLULAR_CALL_LITE_INTERFACE_H
