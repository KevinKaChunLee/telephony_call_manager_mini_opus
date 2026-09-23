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

#ifndef OHOS_ABILITY_BASE_WANT_PARAMS_WRAPPER_H
#define OHOS_ABILITY_BASE_WANT_PARAMS_WRAPPER_H

// Mini stand-in for ability_base's want_params_wrapper.h. The reused common_type.h declares a
// WantParams member; the Mini call manager accepts the "extraParams" dial field without parsing it,
// so the type never carries data and every read yields the caller's default.

#include "tel_mini_std_includes.h"

namespace OHOS {
namespace AAFwk {
class WantParams {
public:
    bool IsEmpty() const
    {
        return true;
    }
    int GetIntParam(const std::string &key, int defaultValue) const
    {
        return defaultValue;
    }
    std::string GetStringParam(const std::string &key) const
    {
        return std::string();
    }
};

class WantParamWrapper {
public:
    explicit WantParamWrapper(const WantParams &params) {}
    std::string ToString() const
    {
        return "{}";
    }
    static WantParams ParseWantParamsWithBrackets(const std::string &text)
    {
        return WantParams();
    }
};
} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_ABILITY_BASE_WANT_PARAMS_WRAPPER_H
