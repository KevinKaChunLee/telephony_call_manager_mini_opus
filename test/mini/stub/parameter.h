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

#ifndef TELEPHONY_MINI_TEST_STUB_PARAMETER_H
#define TELEPHONY_MINI_TEST_STUB_PARAMETER_H

// Host-only stand-in for startup_init syspara (interfaces/innerkits/include/syspara/parameter.h),
// which product images provide. Return values follow upstream GetParameter: the value length on
// success, -9 for an invalid argument, -1 otherwise.

#include <cstdint>
#include <cstring>
#include <map>
#include <string>

namespace MiniTestParameterStore {
inline std::map<std::string, std::string> &Values()
{
    static std::map<std::string, std::string> values;
    return values;
}
} // namespace MiniTestParameterStore

inline void MiniTestSetParameter(const char *key, const char *value)
{
    MiniTestParameterStore::Values()[key] = value;
}

inline void MiniTestClearParameters()
{
    MiniTestParameterStore::Values().clear();
}

inline int GetParameter(const char *key, const char *def, char *value, uint32_t len)
{
    constexpr int paramInvalid = -9;
    constexpr int paramFail = -1;
    if (key == nullptr || value == nullptr || len == 0) {
        return paramInvalid;
    }
    const char *result = def;
    auto it = MiniTestParameterStore::Values().find(key);
    if (it != MiniTestParameterStore::Values().end()) {
        result = it->second.c_str();
    }
    if (result == nullptr) {
        return paramFail;
    }
    size_t length = std::strlen(result);
    if (length >= len) {
        return paramFail;
    }
    std::memcpy(value, result, length + 1);
    return static_cast<int>(length);
}

#endif // TELEPHONY_MINI_TEST_STUB_PARAMETER_H
