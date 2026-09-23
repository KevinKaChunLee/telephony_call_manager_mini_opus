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

#ifndef CS_CONFERENCE_H
#define CS_CONFERENCE_H

#include <cstdio>
#include <cstdlib>

#include "singleton.h"
#include "conference_base.h"

namespace OHOS {
namespace Telephony {
class CsConference : public ConferenceBase {
    DECLARE_DELAYED_SINGLETON(CsConference)
public:
    int32_t JoinToConference(int32_t callId) override;
    int32_t LeaveFromConference(int32_t callId) override;
    int32_t HoldConference(int32_t callId) override;
    int32_t CanCombineConference() override;
    int32_t CanSeparateConference() override;
    int32_t CanKickOutFromConference() override;
    void UpdateConferenceId(int32_t callId);

private:
    uint32_t maxSubCallLimits_;
};
} // namespace Telephony
} // namespace OHOS

#endif // CS_CONFERENCE_BASE_H