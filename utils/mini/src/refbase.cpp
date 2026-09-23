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

#include "refbase.h"

namespace OHOS {
void RefCounter::IncWeak()
{
    weak_.fetch_add(1, std::memory_order_relaxed);
}

void RefCounter::DecWeak()
{
    if (weak_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete this;
    }
}

void RefCounter::IncStrong()
{
    IncWeak();
    strong_.fetch_add(1, std::memory_order_relaxed);
}

bool RefCounter::DecStrong()
{
    return strong_.fetch_sub(1, std::memory_order_acq_rel) == 1;
}

bool RefCounter::AttemptIncStrong()
{
    int32_t current = strong_.load(std::memory_order_relaxed);
    while (current > 0) {
        if (strong_.compare_exchange_weak(current, current + 1, std::memory_order_acq_rel)) {
            IncWeak();
            return true;
        }
    }
    return false;
}

int32_t RefCounter::GetStrongCount() const
{
    return strong_.load(std::memory_order_relaxed);
}

int32_t RefCounter::GetWeakCount() const
{
    return weak_.load(std::memory_order_relaxed);
}

RefBase::RefBase() : refs_(new (std::nothrow) RefCounter()) {}

RefBase::RefBase(const RefBase &) : RefBase() {}

RefBase &RefBase::operator=(const RefBase &)
{
    return *this;
}

RefBase::~RefBase()
{
    if (refs_ != nullptr) {
        RefCounter *refs = refs_;
        refs_ = nullptr;
        refs->DecWeak();
    }
}

void RefBase::IncStrongRef(const void *objectId)
{
    if (refs_ == nullptr) {
        return;
    }
    bool first = (refs_->GetStrongCount() == 0);
    refs_->IncStrong();
    if (first) {
        OnFirstStrongRef(objectId);
    }
}

void RefBase::DecStrongRef(const void *objectId)
{
    RefCounter *refs = refs_;
    if (refs == nullptr) {
        return;
    }
    if (refs->DecStrong()) {
        OnLastStrongRef(objectId);
        delete this;
    }
    refs->DecWeak();
}

int32_t RefBase::GetSptrRefCount() const
{
    return (refs_ == nullptr) ? 0 : refs_->GetStrongCount();
}

void RefBase::IncWeakRef(const void *)
{
    if (refs_ != nullptr) {
        refs_->IncWeak();
    }
}

void RefBase::DecWeakRef(const void *)
{
    if (refs_ != nullptr) {
        refs_->DecWeak();
    }
}

int32_t RefBase::GetWptrRefCount() const
{
    return (refs_ == nullptr) ? 0 : refs_->GetWeakCount();
}

bool RefBase::AttemptIncStrongRef(const void *)
{
    return (refs_ != nullptr) && refs_->AttemptIncStrong();
}

RefCounter *RefBase::GetRefCounter() const
{
    return refs_;
}
} // namespace OHOS
