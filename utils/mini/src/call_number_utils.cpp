/*
 * Copyright (C) 2021-2026 Huawei Device Co., Ltd.
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

#include "call_number_utils.h"

#include "call_manager_errors.h"
#include "call_manager_mini_config.h"
#include "cellular_call_connection.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
CallNumberUtils::CallNumberUtils() {}

CallNumberUtils::~CallNumberUtils() {}

int32_t CallNumberUtils::FormatPhoneNumber(
    const std::string &phoneNumber, const std::string &countryCode, std::string &formatNumber)
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t CallNumberUtils::FormatPhoneNumberToE164(
    const std::string phoneNumber, const std::string countryCode, std::string &formatNumber)
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t CallNumberUtils::CheckNumberIsEmergency(const std::string &phoneNumber, const int32_t slotId, bool &enabled)
{
    return DelayedSingleton<CellularCallConnection>::GetInstance()->IsEmergencyPhoneNumber(
        phoneNumber, slotId, enabled);
}

int32_t CallNumberUtils::IsCarrierVtConfig(const int32_t slotId, bool &enabled)
{
    return DelayedSingleton<CellularCallConnection>::GetInstance()->GetCarrierVtConfig(slotId, enabled);
}

bool CallNumberUtils::IsValidSlotId(int32_t slotId) const
{
    return slotId >= 0 && slotId < MINI_SLOT_COUNT;
}

std::string CallNumberUtils::RemoveSeparatorsPhoneNumber(const std::string &phoneString)
{
    std::string newString;
    if (phoneString.empty()) {
        TELEPHONY_LOGE("RemoveSeparatorsPhoneNumber return, phoneStr is empty.");
        return newString;
    }
    for (char c : phoneString) {
        if ((c >= '0' && c <= '9') || c == '*' || c == '#' || c == '+' || c == 'N' || c == ',' || c == ';') {
            newString += c;
        }
    }
    return newString;
}

std::string CallNumberUtils::RemovePostDialPhoneNumber(const std::string &phoneString)
{
    std::string newString = "";
    if (phoneString.empty()) {
        TELEPHONY_LOGE("RemovePostDialPhoneNumber return, phoneStr is empty.");
        return newString;
    }
    for (char c : phoneString) {
        if ((c >= '0' && c <= '9') || c == '*' || c == '#' || c == '+' || c == 'N') {
            newString += c;
        } else if (c == ',' || c == ';') {
            break;
        }
    }
    return newString;
}

bool CallNumberUtils::SelectAccountId(int32_t slotId, AppExecFwk::PacMap &extras)
{
    return IsValidSlotId(slotId);
}
} // namespace Telephony
} // namespace OHOS
