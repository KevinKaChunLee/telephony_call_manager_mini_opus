/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef CARRIER_CALL_H
#define CARRIER_CALL_H

#include "call_base.h"
#include "cellular_call_connection.h"
#include "ffrt.h"

namespace OHOS {
namespace Telephony {
class CarrierCall : public CallBase {
public:
    explicit CarrierCall(DialParaInfo &info);
    CarrierCall(DialParaInfo &info, AppExecFwk::PacMap &extras);
    ~CarrierCall();
    int32_t CarrierDialingProcess();
    int32_t CarrierAnswerCall(int32_t videoState, bool isRTT = false);
    int32_t CarrierRejectCall();
    int32_t CarrierHangUpCall();
    int32_t CarrierHoldCall();
    int32_t CarrierUnHoldCall();
    int32_t CarrierSetMute(int32_t mute, int32_t slotId);
    void GetCallAttributeCarrierInfo(CallAttributeInfo &info);
    bool GetEmergencyState() override;
    int32_t CarrierSwitchCall();
    int32_t StartDtmf(char str) override;
    int32_t StopDtmf() override;
    int32_t PostDialProceed(bool proceed) override;
    int32_t GetSlotId() override;
    int32_t CarrierCombineConference();
    int32_t CarrierSeparateConference();
    int32_t CarrierKickOutFromConference();
    int32_t IsSupportConferenceable() override;
    void SetSlotId(int32_t slotId) override;
    void SetCallIndex(int32_t index) override;
    int32_t GetCallIndex() override;

protected:
    int32_t PackCellularCallInfo(CellularCallInfo &callInfo);

private:
    DialScene dialScene_;
    int32_t slotId_;
    int32_t index_;
    bool isEcc_;
    ffrt::mutex mutex_;
    std::shared_ptr<CellularCallConnection> cellularCallConnectionPtr_;
};
} // namespace Telephony
} // namespace OHOS

#endif // CARRIER_CALL_H
