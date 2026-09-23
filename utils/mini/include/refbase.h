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

#ifndef UTILS_BASE_REFBASE_H
#define UTILS_BASE_REFBASE_H

// Mini stand-in for c_utils base/include/refbase.h (c_utils has no mini build). Only the subset
// the Mini tree uses is provided, with the upstream names: an object is destroyed when its last
// sptr goes away, and a wptr can be promoted only while some sptr still holds the object.

#include "tel_mini_std_includes.h"

namespace OHOS {
class RefBase;

class RefCounter {
public:
    void IncWeak();
    void DecWeak();
    void IncStrong();
    // Returns true when this call released the last strong reference.
    bool DecStrong();
    bool AttemptIncStrong();
    int32_t GetStrongCount() const;
    int32_t GetWeakCount() const;

private:
    std::atomic<int32_t> strong_ { 0 };
    // One for the living object, one per wptr and one per strong reference.
    std::atomic<int32_t> weak_ { 1 };
};

class RefBase {
public:
    RefBase();
    RefBase(const RefBase &other);
    RefBase &operator=(const RefBase &other);
    RefBase(RefBase &&other) = delete;
    RefBase &operator=(RefBase &&other) = delete;
    virtual ~RefBase();

    void IncStrongRef(const void *objectId = nullptr);
    void DecStrongRef(const void *objectId = nullptr);
    int32_t GetSptrRefCount() const;
    void IncWeakRef(const void *objectId = nullptr);
    void DecWeakRef(const void *objectId = nullptr);
    int32_t GetWptrRefCount() const;
    bool AttemptIncStrongRef(const void *objectId = nullptr);
    RefCounter *GetRefCounter() const;

    virtual void OnFirstStrongRef(const void *objectId) {}
    virtual void OnLastStrongRef(const void *objectId) {}

private:
    RefCounter *refs_ = nullptr;
};

template <typename T>
class wptr;

template <typename T>
class sptr {
public:
    sptr() = default;
    sptr(std::nullptr_t) {}
    sptr(T *other) : refs_(other)
    {
        if (refs_ != nullptr) {
            refs_->IncStrongRef(this);
        }
    }
    sptr(const sptr<T> &other) : sptr(other.GetRefPtr()) {}
    sptr(sptr<T> &&other) noexcept : refs_(other.refs_)
    {
        other.refs_ = nullptr;
    }
    template <typename O>
    sptr(const sptr<O> &other) : sptr(static_cast<T *>(other.GetRefPtr()))
    {
    }
    ~sptr()
    {
        if (refs_ != nullptr) {
            refs_->DecStrongRef(this);
        }
    }

    template <typename... Args>
    static sptr<T> MakeSptr(Args &&... args)
    {
        return sptr<T>(new (std::nothrow) T(std::forward<Args>(args)...));
    }

    sptr<T> &operator=(T *other)
    {
        if (other != nullptr) {
            other->IncStrongRef(this);
        }
        T *old = refs_;
        refs_ = other;
        if (old != nullptr) {
            old->DecStrongRef(this);
        }
        return *this;
    }
    sptr<T> &operator=(const sptr<T> &other)
    {
        return operator=(other.GetRefPtr());
    }
    sptr<T> &operator=(sptr<T> &&other) noexcept
    {
        if (this != &other) {
            T *old = refs_;
            refs_ = other.refs_;
            other.refs_ = nullptr;
            if (old != nullptr) {
                old->DecStrongRef(this);
            }
        }
        return *this;
    }
    template <typename O>
    sptr<T> &operator=(const sptr<O> &other)
    {
        return operator=(static_cast<T *>(other.GetRefPtr()));
    }
    sptr<T> &operator=(std::nullptr_t)
    {
        clear();
        return *this;
    }

    void clear()
    {
        if (refs_ != nullptr) {
            T *old = refs_;
            refs_ = nullptr;
            old->DecStrongRef(this);
        }
    }
    T *GetRefPtr() const
    {
        return refs_;
    }
    operator T *() const
    {
        return refs_;
    }
    T &operator*() const
    {
        return *refs_;
    }
    T *operator->() const
    {
        return refs_;
    }
    bool operator!() const
    {
        return refs_ == nullptr;
    }
    bool operator==(const T *other) const
    {
        return refs_ == other;
    }
    bool operator!=(const T *other) const
    {
        return refs_ != other;
    }
    bool operator==(const sptr<T> &other) const
    {
        return refs_ == other.refs_;
    }
    bool operator!=(const sptr<T> &other) const
    {
        return refs_ != other.refs_;
    }

private:
    T *refs_ = nullptr;
};

template <typename T>
class wptr {
public:
    wptr() = default;
    wptr(T *other) : ptr_(other)
    {
        Attach();
    }
    wptr(const sptr<T> &other) : wptr(other.GetRefPtr()) {}
    wptr(const wptr<T> &other) : ptr_(other.ptr_), counter_(other.counter_)
    {
        if (counter_ != nullptr) {
            counter_->IncWeak();
        }
    }
    ~wptr()
    {
        Detach();
    }
    wptr<T> &operator=(const wptr<T> &other)
    {
        if (this != &other) {
            if (other.counter_ != nullptr) {
                other.counter_->IncWeak();
            }
            Detach();
            ptr_ = other.ptr_;
            counter_ = other.counter_;
        }
        return *this;
    }
    wptr<T> &operator=(const sptr<T> &other)
    {
        return operator=(wptr<T>(other));
    }

    // Empty when the object has already been released.
    sptr<T> promote() const
    {
        if (counter_ == nullptr || !counter_->AttemptIncStrong()) {
            return nullptr;
        }
        sptr<T> strong(ptr_);
        ptr_->DecStrongRef(this);
        return strong;
    }
    T *GetRefPtr() const
    {
        return ptr_;
    }
    bool operator==(const wptr<T> &other) const
    {
        return ptr_ == other.ptr_;
    }
    bool operator!=(const wptr<T> &other) const
    {
        return ptr_ != other.ptr_;
    }

private:
    void Attach()
    {
        if (ptr_ != nullptr) {
            counter_ = ptr_->GetRefCounter();
            counter_->IncWeak();
        }
    }
    void Detach()
    {
        if (counter_ != nullptr) {
            counter_->DecWeak();
            counter_ = nullptr;
        }
        ptr_ = nullptr;
    }

    T *ptr_ = nullptr;
    RefCounter *counter_ = nullptr;
};
} // namespace OHOS
#endif // UTILS_BASE_REFBASE_H
