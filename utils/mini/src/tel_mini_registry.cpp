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

#include "call_audio_lite_interface.h"
#include "cellular_call_lite_interface.h"
#include "core_service_lite_interface.h"
#include "tel_mini_spin_lock.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
namespace {
template <typename T>
class Slot {
public:
    // The replaced instance is returned so that its last reference drops outside the lock.
    sptr<T> Exchange(const sptr<T> &instance)
    {
        std::lock_guard<TelMiniSpinLock> guard(lock_);
        sptr<T> previous = instance_;
        instance_ = instance;
        return previous;
    }
    sptr<T> Get()
    {
        std::lock_guard<TelMiniSpinLock> guard(lock_);
        return instance_;
    }

private:
    TelMiniSpinLock lock_;
    sptr<T> instance_;
};

Slot<CellularCallLiteInterface> g_cellularCall;
Slot<CoreServiceLiteInterface> g_coreService;
Slot<CallAudioLiteInterface> g_callAudio;

TelMiniSpinLock g_listenerLock;
CellularCallLiteReadyListener g_readyListeners[CellularCallLiteRegistry::MAX_LISTENERS] = {};

void NotifyReady(const CellularCallLiteReadyListener (&listeners)[CellularCallLiteRegistry::MAX_LISTENERS])
{
    for (CellularCallLiteReadyListener listener : listeners) {
        if (listener != nullptr) {
            listener();
        }
    }
}
} // namespace

void CellularCallLiteRegistry::Register(const sptr<CellularCallLiteInterface> &instance)
{
    sptr<CellularCallLiteInterface> previous = g_cellularCall.Exchange(instance);
    previous = nullptr;
    if (instance == nullptr) {
        return;
    }
    CellularCallLiteReadyListener snapshot[MAX_LISTENERS] = {};
    {
        std::lock_guard<TelMiniSpinLock> guard(g_listenerLock);
        for (uint32_t i = 0; i < MAX_LISTENERS; i++) {
            snapshot[i] = g_readyListeners[i];
        }
    }
    NotifyReady(snapshot);
}

sptr<CellularCallLiteInterface> CellularCallLiteRegistry::Get()
{
    return g_cellularCall.Get();
}

bool CellularCallLiteRegistry::AddReadyListener(CellularCallLiteReadyListener listener)
{
    if (listener == nullptr) {
        return false;
    }
    bool added = false;
    {
        std::lock_guard<TelMiniSpinLock> guard(g_listenerLock);
        for (CellularCallLiteReadyListener &slot : g_readyListeners) {
            if (slot == listener) {
                added = true;
                break;
            }
        }
        for (uint32_t i = 0; !added && i < MAX_LISTENERS; i++) {
            if (g_readyListeners[i] == nullptr) {
                g_readyListeners[i] = listener;
                added = true;
            }
        }
    }
    if (!added) {
        TELEPHONY_LOGE("cellular call ready listener table full");
        return false;
    }
    if (Get() != nullptr) {
        listener();
    }
    return true;
}

void CellularCallLiteRegistry::RemoveReadyListener(CellularCallLiteReadyListener listener)
{
    std::lock_guard<TelMiniSpinLock> guard(g_listenerLock);
    for (CellularCallLiteReadyListener &slot : g_readyListeners) {
        if (slot == listener) {
            slot = nullptr;
        }
    }
}

void CoreServiceLiteRegistry::Register(const sptr<CoreServiceLiteInterface> &instance)
{
    sptr<CoreServiceLiteInterface> previous = g_coreService.Exchange(instance);
}

sptr<CoreServiceLiteInterface> CoreServiceLiteRegistry::Get()
{
    return g_coreService.Get();
}

void CallAudioLiteRegistry::Register(const sptr<CallAudioLiteInterface> &instance)
{
    sptr<CallAudioLiteInterface> previous = g_callAudio.Exchange(instance);
}

sptr<CallAudioLiteInterface> CallAudioLiteRegistry::Get()
{
    return g_callAudio.Get();
}
} // namespace Telephony
} // namespace OHOS
