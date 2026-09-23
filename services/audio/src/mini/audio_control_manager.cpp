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

#include "audio_control_manager.h"

#include "call_audio_lite_interface.h"
#include "call_manager_errors.h"
#include "call_object_manager.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
AudioControlManager::AudioControlManager() {}

AudioControlManager::~AudioControlManager() {}

void AudioControlManager::Init()
{
    ReleaseAll();
    unavailableLogged_ = false;
}

void AudioControlManager::UnInit()
{
    if (callAudioActive_ || ringtonePlaying_ || waitingTonePlaying_) {
        StopRingtone();
        StopWaitingTone();
        SetCallAudioActive(false);
    }
    ReleaseAll();
}

void AudioControlManager::CallStateUpdated(
    sptr<CallBase> &callObjectPtr, TelCallState priorState, TelCallState nextState)
{
    switch (nextState) {
        case TelCallState::CALL_STATUS_INCOMING:
            StartRingtone();
            break;
        case TelCallState::CALL_STATUS_WAITING:
            PlayWaitingTone();
            break;
        case TelCallState::CALL_STATUS_ANSWERED:
        case TelCallState::CALL_STATUS_ACTIVE:
            StopRingtone();
            StopWaitingTone();
            SetCallAudioActive(true);
            break;
        case TelCallState::CALL_STATUS_DISCONNECTED:
            if (priorState == TelCallState::CALL_STATUS_INCOMING) {
                StopRingtone();
            } else if (priorState == TelCallState::CALL_STATUS_WAITING) {
                StopWaitingTone();
            }
            if (!CallObjectManager::HasCellularCallExist()) {
                StopRingtone();
                StopWaitingTone();
                SetCallAudioActive(false);
                ReleaseAll();
            }
            break;
        default:
            break;
    }
}

void AudioControlManager::IncomingCallHungUp(sptr<CallBase> &callObjectPtr, bool isSendSms, std::string content)
{
    StopRingtone();
}

int32_t AudioControlManager::SetMute(bool isMute)
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

int32_t AudioControlManager::SetAudioDevice(const AudioDevice &device)
{
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}

bool AudioControlManager::IsRingtonePlaying() const
{
    return ringtonePlaying_;
}

bool AudioControlManager::IsWaitingTonePlaying() const
{
    return waitingTonePlaying_;
}

bool AudioControlManager::IsCallAudioActive() const
{
    return callAudioActive_;
}

int32_t AudioControlManager::StartRingtone()
{
    if (ringtonePlaying_) {
        return TELEPHONY_SUCCESS;
    }
    sptr<CallAudioLiteInterface> audio = CallAudioLiteRegistry::Get();
    if (audio == nullptr) {
        return Unavailable();
    }
    int32_t ret = audio->StartRingtone();
    ringtonePlaying_ = (ret == TELEPHONY_SUCCESS);
    return ret;
}

int32_t AudioControlManager::StopRingtone()
{
    if (!ringtonePlaying_) {
        return TELEPHONY_SUCCESS;
    }
    ringtonePlaying_ = false;
    sptr<CallAudioLiteInterface> audio = CallAudioLiteRegistry::Get();
    return (audio == nullptr) ? Unavailable() : audio->StopRingtone();
}

int32_t AudioControlManager::PlayWaitingTone()
{
    if (waitingTonePlaying_) {
        return TELEPHONY_SUCCESS;
    }
    sptr<CallAudioLiteInterface> audio = CallAudioLiteRegistry::Get();
    if (audio == nullptr) {
        return Unavailable();
    }
    int32_t ret = audio->PlayWaitingTone();
    waitingTonePlaying_ = (ret == TELEPHONY_SUCCESS);
    return ret;
}

int32_t AudioControlManager::StopWaitingTone()
{
    if (!waitingTonePlaying_) {
        return TELEPHONY_SUCCESS;
    }
    waitingTonePlaying_ = false;
    sptr<CallAudioLiteInterface> audio = CallAudioLiteRegistry::Get();
    return (audio == nullptr) ? Unavailable() : audio->StopWaitingTone();
}

int32_t AudioControlManager::SetCallAudioActive(bool active)
{
    if (callAudioActive_ == active) {
        return TELEPHONY_SUCCESS;
    }
    sptr<CallAudioLiteInterface> audio = CallAudioLiteRegistry::Get();
    if (audio == nullptr) {
        return Unavailable();
    }
    int32_t ret = audio->SetCallAudioActive(active);
    if (ret == TELEPHONY_SUCCESS) {
        callAudioActive_ = active;
    } else {
        TELEPHONY_LOGE("call audio %{public}s failed: %{public}d", active ? "activation" : "release", ret);
    }
    return ret;
}

void AudioControlManager::ReleaseAll()
{
    ringtonePlaying_ = false;
    waitingTonePlaying_ = false;
    callAudioActive_ = false;
}

int32_t AudioControlManager::Unavailable()
{
    if (!unavailableLogged_) {
        unavailableLogged_ = true;
        TELEPHONY_LOGW("no call audio implementation registered; call audio is not available");
    }
    return CALL_ERR_FUNCTION_NOT_SUPPORTED;
}
} // namespace Telephony
} // namespace OHOS
