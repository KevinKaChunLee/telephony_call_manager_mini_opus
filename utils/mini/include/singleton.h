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

#ifndef UTILS_BASE_SINGLETON_H
#define UTILS_BASE_SINGLETON_H

// Mini stand-in for c_utils base/include/singleton.h (c_utils has no mini build); keeps the
// upstream class and macro names. The upstream std::mutex and shared_ptr atomics would each take a
// kernel mutex on LiteOS-M, and thread-safe function statics are not guaranteed by every MCU
// toolchain, so construction is serialized by an atomic state instead: the losing tasks yield
// until the winner has published the instance.
//
// DestroyInstance may only run when no other task can call GetInstance any more (service exit).

#include "tel_mini_std_includes.h"
#include "nocopyable.h"
#include "tel_os_adapter.h"

namespace OHOS {
#define DECLARE_DELAYED_SINGLETON(MyClass) \
public:                                    \
    ~MyClass();                            \
                                           \
private:                                   \
    friend DelayedSingleton<MyClass>;      \
    MyClass();

#define DECLARE_DELAYED_REF_SINGLETON(MyClass) \
private:                                       \
    friend DelayedRefSingleton<MyClass>;       \
    ~MyClass();                                \
    MyClass();

#define DECLARE_SINGLETON(MyClass)                \
private:                                          \
    friend Singleton<MyClass>;                    \
    MyClass &operator=(const MyClass &) = delete; \
    MyClass(const MyClass &) = delete;            \
    MyClass();                                    \
    ~MyClass();

namespace SingletonState {
constexpr uint32_t EMPTY = 0;
constexpr uint32_t BUSY = 1;
constexpr uint32_t READY = 2;

// Returns true when the caller won the right to construct; false once the instance is READY.
inline bool AcquireOrWait(std::atomic<uint32_t> &state)
{
    uint32_t current = state.load(std::memory_order_acquire);
    while (current != READY) {
        uint32_t expected = EMPTY;
        if (current == EMPTY &&
            state.compare_exchange_strong(expected, BUSY, std::memory_order_acq_rel)) {
            return true;
        }
        Telephony::TelOsTaskYield();
        current = state.load(std::memory_order_acquire);
    }
    return false;
}
} // namespace SingletonState

template <typename T>
class DelayedSingleton : public NoCopyable {
public:
    static std::shared_ptr<T> GetInstance();
    static void DestroyInstance();

private:
    static std::shared_ptr<T> instance_;
    static std::atomic<uint32_t> state_;
};

template <typename T>
std::shared_ptr<T> DelayedSingleton<T>::instance_ = nullptr;

template <typename T>
std::atomic<uint32_t> DelayedSingleton<T>::state_ { SingletonState::EMPTY };

template <typename T>
std::shared_ptr<T> DelayedSingleton<T>::GetInstance()
{
    if (SingletonState::AcquireOrWait(state_)) {
        std::shared_ptr<T> created(new (std::nothrow) T);
        if (created == nullptr) {
            state_.store(SingletonState::EMPTY, std::memory_order_release);
            return nullptr;
        }
        instance_ = created;
        state_.store(SingletonState::READY, std::memory_order_release);
    }
    return instance_;
}

template <typename T>
void DelayedSingleton<T>::DestroyInstance()
{
    uint32_t expected = SingletonState::READY;
    if (state_.compare_exchange_strong(expected, SingletonState::BUSY, std::memory_order_acq_rel)) {
        instance_.reset();
        state_.store(SingletonState::EMPTY, std::memory_order_release);
    }
}

template <typename T>
class DelayedRefSingleton : public NoCopyable {
public:
    static T &GetInstance();

private:
    static T *instance_;
    static std::atomic<uint32_t> state_;
};

template <typename T>
T *DelayedRefSingleton<T>::instance_ = nullptr;

template <typename T>
std::atomic<uint32_t> DelayedRefSingleton<T>::state_ { SingletonState::EMPTY };

template <typename T>
T &DelayedRefSingleton<T>::GetInstance()
{
    if (SingletonState::AcquireOrWait(state_)) {
        instance_ = new T();
        state_.store(SingletonState::READY, std::memory_order_release);
    }
    return *instance_;
}

template <typename T>
class Singleton : public NoCopyable {
public:
    static T &GetInstance()
    {
        return instance_;
    }

private:
    static T instance_;
};

template <typename T>
T Singleton<T>::instance_;
} // namespace OHOS
#endif // UTILS_BASE_SINGLETON_H
