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

#ifndef TELEPHONY_MINI_CALL_PERMISSION_ADAPTER_H
#define TELEPHONY_MINI_CALL_PERMISSION_ADAPTER_H

#include <string>

// A single LiteOS-M image has no process or application identity, so every caller inside it gets the
// trust the product declares at build time (callmanager_mini.gni):
//   CALL_MANAGER_MINI_GRANTED_PERMISSIONS  ';'-separated permission names, matched exactly
//   CALL_MANAGER_MINI_CALLER_IS_SYSTEM_APP defined when in-image callers count as system apps
//   CALL_MANAGER_MINI_CALLER_BUNDLE_NAME   the bundle name reported for them
// Nothing is granted by default. There is deliberately no "allow all" setting.

namespace OHOS {
namespace Telephony {
class CallPermissionAdapter {
public:
    static bool CheckPermission(const std::string &permission);
    static bool IsSystemCaller();
    static std::string GetCallerBundleName();

#ifdef TELEPHONY_MINI_HOST_TEST
    // Replaces the build-time trust model for one self-test; nullptr restores it.
    static void OverrideForTest(const char *grantedPermissions, bool isSystemApp, const char *bundleName);
#endif
};
} // namespace Telephony
} // namespace OHOS
#endif // TELEPHONY_MINI_CALL_PERMISSION_ADAPTER_H
