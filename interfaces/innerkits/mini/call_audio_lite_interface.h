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

#ifndef CALL_AUDIO_LITE_INTERFACE_H
#define CALL_AUDIO_LITE_INTERFACE_H

// Product audio for calls (design D10). The Mini AudioControlManager drives it from the call state on
// the main loop; failures are logged and never block the call state machine. Calls must not block.

#include "tel_mini_std_includes.h"

#include "refbase.h"

namespace OHOS {
namespace Telephony {
class CallAudioLiteInterface : public virtual RefBase {
public:
    virtual ~CallAudioLiteInterface() = default;

    virtual int32_t SetCallAudioActive(bool active) = 0;
    virtual int32_t StartRingtone() = 0;
    virtual int32_t StopRingtone() = 0;
    virtual int32_t PlayWaitingTone() = 0;
    virtual int32_t StopWaitingTone() = 0;
};

class CallAudioLiteRegistry {
public:
    // Replaces any earlier instance; nullptr unregisters.
    static void Register(const sptr<CallAudioLiteInterface> &instance);
    static sptr<CallAudioLiteInterface> Get();
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_AUDIO_LITE_INTERFACE_H
