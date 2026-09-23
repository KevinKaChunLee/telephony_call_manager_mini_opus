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

#include "call_permission_adapter.h"

#include <cstring>

#include "telephony_log_wrapper.h"

#ifndef CALL_MANAGER_MINI_GRANTED_PERMISSIONS
#define CALL_MANAGER_MINI_GRANTED_PERMISSIONS ""
#endif
#ifndef CALL_MANAGER_MINI_CALLER_BUNDLE_NAME
#define CALL_MANAGER_MINI_CALLER_BUNDLE_NAME ""
#endif

namespace OHOS {
namespace Telephony {
namespace {
constexpr char PERMISSION_SEPARATOR = ';';

#ifdef CALL_MANAGER_MINI_CALLER_IS_SYSTEM_APP
constexpr bool BUILD_CALLER_IS_SYSTEM_APP = true;
#else
constexpr bool BUILD_CALLER_IS_SYSTEM_APP = false;
#endif

struct TrustModel {
    const char *granted = CALL_MANAGER_MINI_GRANTED_PERMISSIONS;
    bool isSystemApp = BUILD_CALLER_IS_SYSTEM_APP;
    const char *bundleName = CALL_MANAGER_MINI_CALLER_BUNDLE_NAME;
};

TrustModel g_trustModel;

bool ListContains(const char *list, const std::string &item)
{
    if (list == nullptr || item.empty()) {
        return false;
    }
    const char *begin = list;
    while (*begin != '\0') {
        const char *end = std::strchr(begin, PERMISSION_SEPARATOR);
        size_t length = (end == nullptr) ? std::strlen(begin) : static_cast<size_t>(end - begin);
        if (length == item.size() && std::strncmp(begin, item.c_str(), length) == 0) {
            return true;
        }
        if (end == nullptr) {
            break;
        }
        begin = end + 1;
    }
    return false;
}
} // namespace

bool CallPermissionAdapter::CheckPermission(const std::string &permission)
{
    bool granted = ListContains(g_trustModel.granted, permission);
    if (!granted) {
        TELEPHONY_LOGE("permission %{public}s is not granted to in-image callers", permission.c_str());
    }
    return granted;
}

bool CallPermissionAdapter::IsSystemCaller()
{
    return g_trustModel.isSystemApp;
}

std::string CallPermissionAdapter::GetCallerBundleName()
{
    return (g_trustModel.bundleName == nullptr) ? std::string() : std::string(g_trustModel.bundleName);
}

#ifdef TELEPHONY_MINI_HOST_TEST
void CallPermissionAdapter::OverrideForTest(const char *grantedPermissions, bool isSystemApp, const char *bundleName)
{
    if (grantedPermissions == nullptr) {
        g_trustModel = TrustModel();
        return;
    }
    g_trustModel.granted = grantedPermissions;
    g_trustModel.isSystemApp = isSystemApp;
    g_trustModel.bundleName = bundleName;
}
#endif
} // namespace Telephony
} // namespace OHOS
