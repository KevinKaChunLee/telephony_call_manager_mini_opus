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

#ifndef TELEPHONY_MINI_FFRT_H
#define TELEPHONY_MINI_FFRT_H

// Mini stand-in for the ffrt types that reused standard headers declare as members. Business state
// is only touched on the call manager's main loop, so these locks guard nothing and own no kernel
// object: LiteOS-M's default pool of six mutexes could not back one per call object anyway.
// In host self-tests every lock() asks the registered check whether the caller is on the main loop
// and counts the calls that are not.
//
// condition_variable never blocks: waiting for another task of the same loop would deadlock it,
// so the Mini code must not wait at all, and the blocking wait() overload is deliberately absent.

#include "tel_mini_std_includes.h"

#include <mutex>

namespace OHOS {
namespace Telephony {
using TelMiniConfinementCheck = bool (*)();

void TelMiniSetConfinementCheck(TelMiniConfinementCheck check);
uint32_t TelMiniConfinementViolations();

#ifdef TELEPHONY_MINI_HOST_TEST
void TelMiniNoteConfinedAccess();
#else
inline void TelMiniNoteConfinedAccess() {}
#endif
} // namespace Telephony
} // namespace OHOS

namespace ffrt {
class mutex {
public:
    mutex() = default;
    mutex(const mutex &) = delete;
    mutex &operator=(const mutex &) = delete;

    void lock()
    {
        OHOS::Telephony::TelMiniNoteConfinedAccess();
    }
    bool try_lock()
    {
        lock();
        return true;
    }
    void unlock() {}
};

class recursive_mutex {
public:
    recursive_mutex() = default;
    recursive_mutex(const recursive_mutex &) = delete;
    recursive_mutex &operator=(const recursive_mutex &) = delete;

    void lock()
    {
        OHOS::Telephony::TelMiniNoteConfinedAccess();
    }
    bool try_lock()
    {
        lock();
        return true;
    }
    void unlock() {}
};

class shared_mutex {
public:
    shared_mutex() = default;
    shared_mutex(const shared_mutex &) = delete;
    shared_mutex &operator=(const shared_mutex &) = delete;

    void lock()
    {
        OHOS::Telephony::TelMiniNoteConfinedAccess();
    }
    bool try_lock()
    {
        lock();
        return true;
    }
    void unlock() {}
    void lock_shared()
    {
        OHOS::Telephony::TelMiniNoteConfinedAccess();
    }
    bool try_lock_shared()
    {
        lock_shared();
        return true;
    }
    void unlock_shared() {}
};

enum class cv_status {
    no_timeout,
    timeout,
};

class condition_variable {
public:
    condition_variable() = default;
    condition_variable(const condition_variable &) = delete;
    condition_variable &operator=(const condition_variable &) = delete;

    void notify_one() noexcept {}
    void notify_all() noexcept {}

    template <typename Lock, typename Rep, typename Period>
    cv_status wait_for(Lock &, const std::chrono::duration<Rep, Period> &)
    {
        return cv_status::timeout;
    }
    template <typename Lock, typename Rep, typename Period, typename Predicate>
    bool wait_for(Lock &, const std::chrono::duration<Rep, Period> &, Predicate pred)
    {
        return pred();
    }
};
} // namespace ffrt
#endif // TELEPHONY_MINI_FFRT_H
