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

#ifndef CALL_NUMBER_UTILS_H
#define CALL_NUMBER_UTILS_H

// Mini variant of utils/include/call_number_utils.h. Kept: separator removal, the emergency check
// through the protocol stack adapter and the slot check. Formatting, number location and yellow-page
// lookups need i18n and data-share services and fail with CALL_ERR_FUNCTION_NOT_SUPPORTED.

#include "singleton.h"
#include "pac_map.h"

namespace OHOS {
namespace Telephony {
class CallNumberUtils {
    DECLARE_DELAYED_SINGLETON(CallNumberUtils)

public:
    int32_t FormatPhoneNumber(const std::string &phoneNumber, const std::string &countryCode,
        std::string &formatNumber);
    int32_t FormatPhoneNumberToE164(const std::string phoneNumber, const std::string countryCode,
        std::string &formatNumber);
    int32_t CheckNumberIsEmergency(const std::string &phoneNumber, const int32_t slotId, bool &enabled);
    int32_t IsCarrierVtConfig(const int32_t slotId, bool &enabled);
    bool IsValidSlotId(int32_t slotId) const;
    std::string RemoveSeparatorsPhoneNumber(const std::string &phoneString);
    std::string RemovePostDialPhoneNumber(const std::string &phoneString);
    bool SelectAccountId(int32_t slotId, AppExecFwk::PacMap &extras);
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_NUMBER_UTILS_H
