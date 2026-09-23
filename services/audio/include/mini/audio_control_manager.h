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

#ifndef AUDIO_CONTROL_MANAGER_H
#define AUDIO_CONTROL_MANAGER_H

// Mini variant of services/audio/include/audio_control_manager.h, degraded per design D10: the call
// state drives the product's CallAudioLiteInterface (ringtone, waiting tone, call audio path). Audio
// results never block the state machine. Without a registered implementation every request returns
// CALL_ERR_FUNCTION_NOT_SUPPORTED and is logged once. Device routing, mute and DTMF are cut.

#include "singleton.h"

#include "call_manager_inner_type.h"
#include "call_state_listener_base.h"

namespace OHOS {
namespace Telephony {
class AudioControlManager : public CallStateListenerBase, public std::enable_shared_from_this<AudioControlManager> {
    DECLARE_DELAYED_SINGLETON(AudioControlManager)

public:
    void Init();
    void UnInit();
    void CallStateUpdated(sptr<CallBase> &callObjectPtr, TelCallState priorState, TelCallState nextState) override;
    void IncomingCallHungUp(sptr<CallBase> &callObjectPtr, bool isSendSms, std::string content) override;
    int32_t SetMute(bool isMute);
    int32_t SetAudioDevice(const AudioDevice &device);
    bool IsRingtonePlaying() const;
    bool IsWaitingTonePlaying() const;
    bool IsCallAudioActive() const;

private:
    int32_t StartRingtone();
    int32_t StopRingtone();
    int32_t PlayWaitingTone();
    int32_t StopWaitingTone();
    int32_t SetCallAudioActive(bool active);
    void ReleaseAll();
    int32_t Unavailable();

    bool ringtonePlaying_ = false;
    bool waitingTonePlaying_ = false;
    bool callAudioActive_ = false;
    bool unavailableLogged_ = false;
};
} // namespace Telephony
} // namespace OHOS
#endif // AUDIO_CONTROL_MANAGER_H
