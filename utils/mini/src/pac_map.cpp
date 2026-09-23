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

#include "pac_map.h"

#include "call_manager_mini_config.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace AppExecFwk {
bool PacMap::Put(const std::string &key, Value &&value)
{
    auto it = values_.find(key);
    if (it != values_.end()) {
        it->second = std::move(value);
        return true;
    }
    if (values_.size() >= Telephony::MINI_PACMAP_MAX_ENTRIES) {
        TELEPHONY_LOGE("PacMap full (%{public}u entries), key dropped", Telephony::MINI_PACMAP_MAX_ENTRIES);
        return false;
    }
    values_.emplace(key, std::move(value));
    return true;
}

const PacMap::Value *PacMap::Find(const std::string &key, ValueType type) const
{
    auto it = values_.find(key);
    if (it == values_.end() || it->second.type != type) {
        return nullptr;
    }
    return &it->second;
}

void PacMap::PutIntValue(const std::string &key, int value)
{
    Value entry;
    entry.type = ValueType::INT;
    entry.number = value;
    Put(key, std::move(entry));
}

void PacMap::PutLongValue(const std::string &key, long value)
{
    Value entry;
    entry.type = ValueType::LONG;
    entry.number = value;
    Put(key, std::move(entry));
}

void PacMap::PutBooleanValue(const std::string &key, bool value)
{
    Value entry;
    entry.type = ValueType::BOOLEAN;
    entry.number = value ? 1 : 0;
    Put(key, std::move(entry));
}

void PacMap::PutStringValue(const std::string &key, const std::string &value)
{
    Value entry;
    entry.type = ValueType::STRING;
    entry.text = value;
    Put(key, std::move(entry));
}

int PacMap::GetIntValue(const std::string &key, int defaultValue) const
{
    const Value *entry = Find(key, ValueType::INT);
    return (entry == nullptr) ? defaultValue : static_cast<int>(entry->number);
}

long PacMap::GetLongValue(const std::string &key, long defaultValue) const
{
    const Value *entry = Find(key, ValueType::LONG);
    return (entry == nullptr) ? defaultValue : static_cast<long>(entry->number);
}

bool PacMap::GetBooleanValue(const std::string &key, bool defaultValue) const
{
    const Value *entry = Find(key, ValueType::BOOLEAN);
    return (entry == nullptr) ? defaultValue : (entry->number != 0);
}

std::string PacMap::GetStringValue(const std::string &key, const std::string &defaultValue) const
{
    const Value *entry = Find(key, ValueType::STRING);
    return (entry == nullptr) ? defaultValue : entry->text;
}

bool PacMap::HasKey(const std::string &key) const
{
    return values_.find(key) != values_.end();
}

void PacMap::Remove(const std::string &key)
{
    values_.erase(key);
}

void PacMap::Clear()
{
    values_.clear();
}

bool PacMap::IsEmpty() const
{
    return values_.empty();
}

int PacMap::GetSize() const
{
    return static_cast<int>(values_.size());
}
} // namespace AppExecFwk
} // namespace OHOS
