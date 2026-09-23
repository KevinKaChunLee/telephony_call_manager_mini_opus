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

#ifndef OHOS_OS_APPEXECFWK_PAC_MAP_H
#define OHOS_OS_APPEXECFWK_PAC_MAP_H

// Mini stand-in for ability_base's AppExecFwk::PacMap, limited to the value kinds the call manager
// passes around. Keys are bounded by CALL_MANAGER_MINI_PACMAP_MAX_ENTRIES because dial extras come
// from callers: a Put that would add a key beyond the bound is dropped and logged. Reading a key
// with a different type than it was stored with yields the default, as upstream does.

#include "tel_mini_std_includes.h"

namespace OHOS {
namespace AppExecFwk {
class PacMap {
public:
    void PutIntValue(const std::string &key, int value);
    void PutLongValue(const std::string &key, long value);
    void PutBooleanValue(const std::string &key, bool value);
    void PutStringValue(const std::string &key, const std::string &value);

    int GetIntValue(const std::string &key, int defaultValue = 0) const;
    long GetLongValue(const std::string &key, long defaultValue = 0) const;
    bool GetBooleanValue(const std::string &key, bool defaultValue = false) const;
    std::string GetStringValue(const std::string &key, const std::string &defaultValue = "") const;

    bool HasKey(const std::string &key) const;
    void Remove(const std::string &key);
    void Clear();
    bool IsEmpty() const;
    int GetSize() const;

private:
    enum class ValueType : uint8_t {
        INT,
        LONG,
        BOOLEAN,
        STRING,
    };
    struct Value {
        ValueType type = ValueType::INT;
        int64_t number = 0;
        std::string text;
    };
    bool Put(const std::string &key, Value &&value);
    const Value *Find(const std::string &key, ValueType type) const;

    std::map<std::string, Value> values_;
};
} // namespace AppExecFwk
} // namespace OHOS
#endif // OHOS_OS_APPEXECFWK_PAC_MAP_H
