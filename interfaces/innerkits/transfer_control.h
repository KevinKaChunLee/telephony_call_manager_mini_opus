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

#ifndef TRANSFER_CONTROL_H
#define TRANSFER_CONTROL_H

#include <cstdio>

#include "iremote_broker.h"
#include "call_manager_info.h"

namespace OHOS {
namespace Telephony {
class TransferControl {
public:
    /**
     * @brief Construct of TransferControl
     */
    TransferControl() {}

    /**
     * @brief Destroy the TransferControl object
     */
    virtual ~TransferControl() {}

    /**
     * @brief check transfer call can be allowed
     *
     * @param info[in] call detail information
     * @param resultInfo[out] remote device name, and policy
     *
     * @return Returns true:tranfer call allowed, false: tranfer call not allowed
     */
    virtual bool IsCallNeedTransfer(const TransferCallInfo &info, TransferResultInfo &resultInfo) = 0;

    /**
     * @brief notify transfer call detail information
     *
     * @param info[in] call detail information
     *
     * @return Returns true:notify success, false:notify fail
     */
    virtual bool NotifyCallState(const TransferCallInfo &info) = 0;

    /**
     * @brief notify disconnect sco
     *
     * @param devMac[in] the remote device bluetooth mac address
     *
     * @return Returns true:notify success, false:notify fail
     */
    virtual bool DisconnectSco(const std::string &devMac) = 0;
};
} // namespace Telephony
} // namespace OHOS
#endif // TRANSFER_CONTROL_H
